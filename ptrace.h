#pragma once

#include <sys/ptrace.h>
#include <sys/user.h>
#include <unistd.h>

// Support ubuntu-10
#ifndef PTRACE_O_EXITKILL
#define PTRACE_O_EXITKILL 0x100000
#endif

#ifndef PTRACE_EVENT_SECCOMP
#define PTRACE_EVENT_SECCOMP 0x7
#endif

#ifndef PTRACE_O_TRACESECCOMP
#define PTRACE_O_TRACESECCOMP 0x80
#endif

namespace NOPTrace {
    void PtraceTraceMe() noexcept;
    void PtraceSetOptions(pid_t pid, long opts) noexcept;
    long PtraceRestartSyscall(pid_t pid, int signal) noexcept;
    long PtraceContinueSyscall(pid_t pid, int signal) noexcept;
    long PtraceGetEventMsg(pid_t pid) noexcept;
    long PtracePeekUser(pid_t pid, size_t offset) noexcept;
    long PtraceGetRegs(pid_t pid, struct user_regs_struct& registers) noexcept;
    const char* StrPtraceEventName(int event) noexcept;
}
