#include "ptrace.h"

#include <cerrno>
#include <cstdlib>
#include <cstring>
#include <iostream>

#include <sys/uio.h>

namespace NOPTrace {
    long PtraceSafeCall(decltype(PTRACE_SYSCALL) request, pid_t pid, void* addr, void* data) noexcept {
        long res = ptrace(request, pid, addr, data);
        if (res == -1) {
            if (errno != ESRCH) {
                std::cerr << "ptrace(" << request << ", " << pid << ", ...) failed: " << strerror(errno) << std::endl;
                exit(2);
            }
            return -1;
        }
        return res;
    }

    long PtraceRestartSyscall(pid_t pid, int signal) noexcept {
        return PtraceSafeCall(PTRACE_SYSCALL, pid, 0, reinterpret_cast<void*>(signal));
    }

    long PtraceContinueSyscall(pid_t pid, int signal) noexcept {
        return PtraceSafeCall(PTRACE_CONT, pid, 0, reinterpret_cast<void*>(signal));
    }

    long PtraceGetEventMsg(pid_t pid) noexcept {
        long data = 0;
        if (PtraceSafeCall(PTRACE_GETEVENTMSG, pid, 0, reinterpret_cast<void*>(&data)) < 0) {
            return -1;
        }
        return data;
    }

    long PtracePeekUser(pid_t pid, size_t offset) noexcept {
        return PtraceSafeCall(PTRACE_PEEKUSER, pid, reinterpret_cast<void*>(offset), 0);
    }

    long PtraceGetRegs(pid_t pid, struct user_regs_struct& registers) noexcept {
#if defined(__x86_64__)
        return PtraceSafeCall(PTRACE_GETREGS, pid, reinterpret_cast<void*>(0), reinterpret_cast<void*>(&registers));
#else
        struct iovec iov;
        iov.iov_base = &registers;
        iov.iov_len = sizeof(registers);
        return PtraceSafeCall(PTRACE_GETREGSET, pid, reinterpret_cast<void*>(1), reinterpret_cast<void*>(&iov));
#endif
    }

    long PtraceSetRegs(pid_t pid, struct user_regs_struct registers) noexcept {
        struct iovec iov;
        iov.iov_base = &registers;
        iov.iov_len = sizeof(registers);
        return PtraceSafeCall(PTRACE_SETREGSET, pid, reinterpret_cast<void*>(1), reinterpret_cast<void*>(&iov));
    }

    void PtraceTraceMe() noexcept {
        if (ptrace(PTRACE_TRACEME, 0, 0, 0) < 0) {
            perror("ptrace(PTRACE_TRACEME, ...) failed:");
            exit(2);
        }
    }

    void PtraceSetOptions(pid_t pid, long opts) noexcept {
        if (ptrace(PTRACE_SETOPTIONS, pid, 0, opts) < 0) {
            std::cerr << "ptrace(PTRACE_SETOPTIONS, " << pid << ", 0, " << opts << ") failed: " << strerror(errno) << std::endl;
            exit(2);
        }
    }

#define PTRACE_EVENT_CASE(x) \
    case x:                  \
        return #x;

    const char* StrPtraceEventName(int event) noexcept {
        switch (event) {
            PTRACE_EVENT_CASE(PTRACE_EVENT_CLONE);
            PTRACE_EVENT_CASE(PTRACE_EVENT_EXEC);
            PTRACE_EVENT_CASE(PTRACE_EVENT_EXIT);
            PTRACE_EVENT_CASE(PTRACE_EVENT_FORK);
            PTRACE_EVENT_CASE(PTRACE_EVENT_SECCOMP);
            PTRACE_EVENT_CASE(PTRACE_EVENT_VFORK);
            PTRACE_EVENT_CASE(PTRACE_EVENT_VFORK_DONE);
            default:
                return "PTRACE_EVENT_UNKNOWN";
        }
    }
}
