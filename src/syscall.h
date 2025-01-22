#pragma once

#include <sys/syscall.h>
#include "ptrace.h"
#include "regs.h"
#include "utils.h"

#ifdef __x86_64__
    #include "syscall_x86_64.h"

    #define SYSCALL_NR(REGISTERS) REGISTERS.orig_rax
    #define SYSCALL_RETDATA(REGISTERS) REGISTERS.rax
    #define SYSCALL_ARG0(REGISTERS) REGISTERS.rdi
    #define SYSCALL_ARG1(REGISTERS) REGISTERS.rsi
    #define SYSCALL_ARG2(REGISTERS) REGISTERS.rdx
    #define SYSCALL_ARG3(REGISTERS) REGISTERS.r10

#endif

#ifdef __aarch64__
    #include "syscall_aarch64.h"

    #define SYSCALL_NR(REGISTERS) REGISTERS.regs[8]
    #define SYSCALL_RETDATA(REGISTERS) REGISTERS.regs[0]
    #define SYSCALL_ARG0(REGISTERS) REGISTERS.regs[9] // We saved ARG0 here in SyscallEnter
    #define SYSCALL_ARG1(REGISTERS) REGISTERS.regs[1]
    #define SYSCALL_ARG2(REGISTERS) REGISTERS.regs[2]
    #define SYSCALL_ARG3(REGISTERS) REGISTERS.regs[3]
#endif

namespace NOPTrace {
    long GetCloneFlags(pid_t pid);
    long GetSyscallNumber(const struct user_regs_struct& registers);
    const char* StrSyscallName(int syscall);
}
