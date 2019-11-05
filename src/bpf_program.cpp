#include "bpf_program.h"
#include "syscall.h"

#include <cstdio>
#include <cstdlib>
#include <vector>

#include <fcntl.h>
#include <linux/filter.h>
#include <sys/prctl.h>
#include <sys/socket.h>

// Support ubuntu-10
#ifndef PR_SET_NO_NEW_PRIVS
#define PR_SET_NO_NEW_PRIVS 0x26
#endif

// Partly from linux/seccomp.h
#ifndef SECCOMP_MODE_FILTER
#define SECCOMP_MODE_FILTER 0x2
#endif

#ifndef SECCOMP_RET_ALLOW
#define SECCOMP_RET_ALLOW 0x7fff0000
#define SECCOMP_RET_TRACE 0x7ff00000

struct seccomp_data {
    int nr;
    __u32 arch;
    __u64 instruction_pointer;
    __u64 args[6];
};
#endif

namespace NOPTrace {
    void InstallBpfProgram() noexcept {
        std::vector<struct sock_filter> filter = {
            BPF_STMT(BPF_LD | BPF_W | BPF_ABS, (offsetof(struct seccomp_data, nr))),

            // int open(const char *pathname, int flags);
            BPF_JUMP(BPF_JMP | BPF_JEQ | BPF_K, SYS_open, 0, 4),
            BPF_STMT(BPF_LD | BPF_W | BPF_ABS, (offsetof(struct seccomp_data, args[1]))),
            // trace only open syscalls with O_WRONLY and O_RDWR access modes (flags arg.)
            BPF_JUMP(BPF_JMP | BPF_JSET | BPF_K, O_WRONLY, 1, 0),
            BPF_JUMP(BPF_JMP | BPF_JSET | BPF_K, O_RDWR, 0, 1),
            BPF_STMT(BPF_RET | BPF_K, SECCOMP_RET_TRACE),

            // int openat(int dirfd, const char *pathname, int flags);
            BPF_JUMP(BPF_JMP | BPF_JEQ | BPF_K, SYS_openat, 0, 4),
            BPF_STMT(BPF_LD | BPF_W | BPF_ABS, (offsetof(struct seccomp_data, args[2]))),
            // trace only openat syscalls with O_WRONLY and O_RDWR access modes (flags arg.)
            BPF_JUMP(BPF_JMP | BPF_JSET | BPF_K, O_WRONLY, 1, 0),
            BPF_JUMP(BPF_JMP | BPF_JSET | BPF_K, O_RDWR, 0, 1),
            BPF_STMT(BPF_RET | BPF_K, SECCOMP_RET_TRACE),

            // int fcntl(int fd, int cmd, ... /* arg */ );
            BPF_JUMP(BPF_JMP | BPF_JEQ | BPF_K, SYS_fcntl, 0, 6),
            BPF_STMT(BPF_LD | BPF_W | BPF_ABS, (offsetof(struct seccomp_data, args[1]))),
            // trace only specific set of command
            BPF_JUMP(BPF_JMP | BPF_JSET | BPF_K, F_DUPFD, 3, 0),
            BPF_JUMP(BPF_JMP | BPF_JSET | BPF_K, F_DUPFD_CLOEXEC, 2, 0),
            BPF_JUMP(BPF_JMP | BPF_JSET | BPF_K, F_SETFL, 1, 0),
            BPF_JUMP(BPF_JMP | BPF_JSET | BPF_K, F_SETFD, 0, 1),
            BPF_STMT(BPF_RET | BPF_K, SECCOMP_RET_TRACE),
        };

        const std::vector<unsigned> tracingSyscalls = {
            SYS_write,
            SYS_writev,
            SYS_pwritev,
            SYS_pwrite64,
            SYS_pwritev2,
            SYS_close,
            SYS_creat,
            SYS_dup,
            SYS_dup2,
            SYS_dup3,
            SYS_fallocate,
            SYS_ftruncate,
            SYS_lseek,
        };

        for (auto syscall : tracingSyscalls) {
            filter.push_back(BPF_JUMP(BPF_JMP | BPF_JEQ | BPF_K, syscall, 0, 1));
            filter.push_back(BPF_STMT(BPF_RET | BPF_K, SECCOMP_RET_TRACE));
        }
        // Allow all other syscalls
        filter.push_back(BPF_STMT(BPF_RET | BPF_K, SECCOMP_RET_ALLOW));

        struct sock_fprog prog;
        prog.filter = &filter.front();
        prog.len = (unsigned short)filter.size();

        if (prctl(PR_SET_NO_NEW_PRIVS, 1, 0, 0, 0) == -1) {
            perror("prctl(PR_SET_NO_NEW_PRIVS, 1, ...) failed:");
            exit(2);
        }

        if (prctl(PR_SET_SECCOMP, SECCOMP_MODE_FILTER, &prog) == -1) {
            perror("prctl(PR_SET_SECCOMP, SECCOMP_MODE_FILTER, ...) failed:");
            exit(2);
        }
    }
}
