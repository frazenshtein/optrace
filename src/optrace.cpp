#include "optrace.h"
#include "bpf_program.h"
#include "context.h"
#include "ptrace.h"
#include "regs.h"
#include "utils.h"
#include "syscall.h"

#include <cassert>
#include <cerrno>
#include <cstring>
#include <iostream>
#include <unordered_map>

#include <sched.h>
#include <sys/user.h>
#include <sys/wait.h>

namespace {
    pid_t TraceePid;
}

namespace NOPTrace {
    const unsigned SEC_COMP_V1 = 1;
    const unsigned SEC_COMP_V2 = 2;
    const int EXIT_CODE_UNKNOWN = -1;
    const int SYSCALL_UNDEFINED = -1;

    int TraceMe(const struct TOptions opts) {
        return TraceProgram(nullptr, opts);
    }

    void SetupTracee(bool useSecComp) {
        if (useSecComp) {
            InstallBpfProgram();
        }

        PtraceTraceMe();

        if (kill(getpid(), SIGTRAP) < 0) {
            std::cerr << "kill(" << getpid() << ", SIGTRAP) failed: " << strerror(errno) << std::endl;
            exit(2);
        }
    }

    void RunTracee(char** argv, bool useSecComp) {
        SetupTracee(useSecComp);

        if (argv) {
            execvp(argv[0], argv);
            std::cerr << "execvp(" << argv[0] << ", ...) failed: " << strerror(errno) << std::endl;
            exit(2);
        }
    }

    void ForwardSignal(int sig) {
        if (TraceePid) {
            kill(TraceePid, sig);
        }
    }

    void SetupSignalForwarding(const std::vector<int>& signums) {
        struct sigaction sa;
        memset(&sa, 0, sizeof(sa));
        sa.sa_flags = SA_RESTART;
        sa.sa_handler = ForwardSignal;

        for (auto signum : signums) {
            assert(sigaction(signum, &sa, nullptr) == 0);
        }
    }

    void SetupTracer(pid_t pid, const struct TOptions& opts, bool useSecComp) {
        int status, res;
        while ((res = waitpid(pid, &status, __WALL)) < 0 && errno == EINTR) {
        }
        if (res < 0) {
            std::cerr << "waitpid(" << pid << ", ...) failed: " << strerror(errno) << std::endl;
            exit(2);
        }

        if ((WIFSTOPPED(status) != true) && (WSTOPSIG(status) != SIGTRAP)) {
            std::cerr << "Unexpected child status: " << status << std::endl;
            exit(2);
        }

        long ptraceOpts = PTRACE_O_TRACESYSGOOD | PTRACE_O_TRACEEXEC | PTRACE_O_TRACEEXIT;
        if (opts.FollowForks) {
            ptraceOpts |= PTRACE_O_TRACECLONE | PTRACE_O_TRACEFORK | PTRACE_O_TRACEVFORK;
        }
        if (opts.JailForks) {
            ptraceOpts |= PTRACE_O_EXITKILL;
        }
        if (useSecComp) {
            ptraceOpts |= PTRACE_O_TRACESECCOMP;
        }

        PtraceSetOptions(pid, ptraceOpts);
    }

    int RunTracer(TContext& context, pid_t traceePid, bool followForks, bool waitDaemons, bool useSecComp) {
        // Restart tracee signal-delivery-stop
        if (useSecComp) {
            assert(!PtraceContinueSyscall(traceePid, 0));
        } else {
            assert(!PtraceRestartSyscall(traceePid, 0));
        }

        unsigned secCompVer = SEC_COMP_V1;
        if (KernelVerGreaterOrEqual("4.8.0-0")) {
            secCompVer = SEC_COMP_V2;
        }

        int status, pid, traceeExitCode = EXIT_CODE_UNKNOWN;
        struct user_regs_struct registers;

        std::unordered_map<pid_t, int> suspendedThreads;
        std::unordered_map<pid_t, int> syscallStateMap = {{traceePid, SYSCALL_UNDEFINED}};

        // Thread might be vanished in case of exit/death
        // and sudden death (when execve is called by thread which is not a group leader).
        auto vanishThread = [&](int pid, bool notify) {
            assert(syscallStateMap.find(pid) != syscallStateMap.end());
            syscallStateMap.erase(pid);

            if (notify) {
                context.VanishProcess(pid);
            }
            // There might be case when group leader thread is dead (a zombie),
            // but other threads are not dead and can generate an event in time.
            // Just restart syscall stop last time.
            PtraceRestartSyscall(pid, 0);
        };

        while (1) {
            pid = wait3(&status, __WALL, 0);
            if (pid < 0) {
                switch (errno) {
                    case EINTR:
                        continue;
                    // No child alive left
                    case ECHILD:
                        if (traceeExitCode == EXIT_CODE_UNKNOWN) {
                            // By default we assume that all children we killed using SIGKILL
                            // and we do not know real exitCode.
                            return 128 + SIGKILL;
                        }
                        return traceeExitCode;
                    default:
                        std::cerr << "wait3 failed: " << strerror(errno) << std::endl;
                        exit(2);
                }
            }

            int exitCode = EXIT_CODE_UNKNOWN;
            int transmittedSignal = 0;
            bool syscallStop = false;
            const unsigned int event = (unsigned int)status >> 16;

            if (WIFSTOPPED(status)) {
                if (WSTOPSIG(status) == (SIGTRAP | 0x80)) {
                    syscallStop = true;
                } else {
                    // signal-delivery-stop
                    transmittedSignal = WSTOPSIG(status);
                }
            } else if (WIFSIGNALED(status)) {
                int termSig = WTERMSIG(status);
                exitCode = 128 + termSig;
                if (WCOREDUMP(status)) {
                    context.RegisterCoreDump(pid, termSig);
                }
            } else if (WIFEXITED(status)) {
                exitCode = WEXITSTATUS(status);
            } else {
                std::cerr << pid << " got unknown status: "<< status << " (event:" << event << ")" << std::endl;
                exit(2);
            }

            if (exitCode != EXIT_CODE_UNKNOWN) {
                vanishThread(pid, true);
                if (pid == traceePid) {
                    // Exit immediately if daemon processes waiting wasn't requested
                    if (followForks && waitDaemons) {
                        if (traceeExitCode == EXIT_CODE_UNKNOWN) {
                            traceeExitCode = exitCode;
                        }
                    } else {
                        return exitCode;
                    }
                }
                // Process is done - do not try to communicate with it.
                continue;
            }

            if (syscallStateMap.find(pid) == syscallStateMap.end()) {
                // This is a new thread, we do not know who created it.
                // Suspend it until proper event occurs.
                suspendedThreads[pid] = status;
                continue;
            }

            if (event) {
                long reportedPid = PtraceGetEventMsg(pid);
                if (reportedPid < 0) {
                    // Looks like process is dead or refusing ptrace requests
                    PtraceRestartSyscall(pid, 0);
                    continue;
                }

                if (event == PTRACE_EVENT_CLONE || event == PTRACE_EVENT_FORK || event == PTRACE_EVENT_VFORK) {
                    bool isThread = false;
                    if (event == PTRACE_EVENT_CLONE) {
                        long cloneFlags = GetCloneFlags(pid);
                        if (cloneFlags < 0) {
                            // Looks like process is dead or refusing ptrace requests
                            PtraceRestartSyscall(pid, 0);
                            continue;
                        }
                        isThread = bool(cloneFlags & CLONE_THREAD);
                    }

                    if (isThread) {
                        context.RegisterThread(pid, reportedPid);
                    } else {
                        context.RegisterProcess(pid, reportedPid);
                    }

                    syscallStateMap[reportedPid] = SYSCALL_UNDEFINED;
                    if (suspendedThreads.find(reportedPid) != suspendedThreads.end()) {
                        suspendedThreads.erase(reportedPid);
                        // Resuming previously suspended thread - now it's properly registered.
                        if (useSecComp) {
                            PtraceContinueSyscall(reportedPid, 0);
                        } else {
                            PtraceRestartSyscall(reportedPid, 0);
                        }
                        syscallStateMap[reportedPid] = SYSCALL_UNDEFINED;
                    }
                } else if (event == PTRACE_EVENT_EXEC) {
                    context.RegisterExec(pid, reportedPid);
                    if (reportedPid != pid) {
                        // execve is called by thread which is not a group leader - this thread is torn down.
                        vanishThread(reportedPid, false);
                    }
                } else if (event == PTRACE_EVENT_SECCOMP) {
                    // Since Linux 3.5 to 4.7: restarts from this stop will behave as
                    // if the stop had occurred right before the system call in question.
                    if (secCompVer == SEC_COMP_V1) {
                        PtraceRestartSyscall(pid, 0);
                        continue;
                        // Since Linux 4.8: a PTRACE_EVENT_SECCOMP stop functions comparably to a syscall-entry-stop.
                    } else {
                        syscallStop = true;
                        syscallStateMap[pid] = SYSCALL_UNDEFINED;
                    }
                }
            }

            if (syscallStop) {
                if (PtraceGetRegs(pid, registers) < 0) {
                    // Looks like process is dead or refusing ptrace requests.
                    PtraceRestartSyscall(pid, 0);
                    continue;
                }

                // Process must be already registered
                assert(syscallStateMap.find(pid) != syscallStateMap.end());
                int& threadPrevSyscall = syscallStateMap[pid];

                if (threadPrevSyscall == SYSCALL_UNDEFINED) {
                    threadPrevSyscall = GetSyscallNumber(registers);
                    // rax stores return data from syscall.
                    // However, before syscall-exit-stop it's unknown and kernel set -ENOSYS.
                    // So this is kind of a sanity check,
                    // that we don't misinterpret syscall-entry-stop as syscall-exit-stop
                    if ((int)registers.rax != -2) {
                        return -2;
                    }
                    context.SyscallEnter(pid, registers);
#if defined(__aarch64__)
                    // We need to save first parameter of syscall as it is replaced by retdata after syscall processing.
                    // We are storing data at r9, which is one of temporary data registers.
                    registers.regs[9] = registers.regs[0];
                    PtraceSetRegs(pid, registers);
#endif
                } else {
                    threadPrevSyscall = SYSCALL_UNDEFINED;
                    context.SyscallExit(pid, registers);
                }
            }

            if (useSecComp) {
                if (syscallStop && syscallStateMap[pid] != SYSCALL_UNDEFINED) {
                    PtraceRestartSyscall(pid, transmittedSignal);
                } else {
                    PtraceContinueSyscall(pid, transmittedSignal);
                }

            } else {
                PtraceRestartSyscall(pid, transmittedSignal);
            }
        }
        return 0;
    }

    int TraceProgram(char** argv, const struct TOptions opts) {
        bool useSecComp = false;
        if (opts.UseSecComp && KernelVerGreaterOrEqual("3.5.0-0")) {
            useSecComp = true;
        }

        sigset_t oldmask, newmask;
        sigfillset(&newmask);
        // block all signals
        assert(sigprocmask(SIG_SETMASK, &newmask, &oldmask) == 0);

        TraceePid = fork();
        if (TraceePid < 0) {
            std::cerr << "fork failed: " << strerror(errno) << std::endl;
            exit(2);
        } else if (TraceePid == 0) {
            // restore signal mask in the child
            assert(sigprocmask(SIG_SETMASK, &oldmask, nullptr) == 0);
            RunTracee(argv, useSecComp);
            return 0;
        }

        SetupTracer(TraceePid, opts, useSecComp);

        if (opts.ForwardAllSignals) {
            std::vector<int> signals {SIGHUP, SIGINT, SIGQUIT, SIGILL, SIGABRT, SIGFPE, SIGSEGV, SIGPIPE, SIGALRM, SIGTERM, SIGUSR1, SIGUSR2};
            SetupSignalForwarding(signals);
        } else if (!opts.ForwardingSignals.empty()) {
            SetupSignalForwarding(opts.ForwardingSignals);
        }
        // restore signal mask in the parent
        assert(sigprocmask(SIG_SETMASK, &oldmask, nullptr) == 0);

        TContext context(opts);
        context.RegisterTracee(TraceePid);

        int rc = RunTracer(context, TraceePid, opts.FollowForks, opts.WaitDaemons, useSecComp);
        rc = context.PostProcess(rc);

        if (argv) {
            return rc;
        } else {
            _exit(rc);
        }
    }
}
