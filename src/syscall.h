#pragma once

#include <sys/syscall.h>

// Support ubuntu-10
#ifndef SYS_recvmmsg
#define SYS_recvmmsg 299
#endif

#ifndef SYS_fanotify_init
#define SYS_fanotify_init 300
#endif

#ifndef SYS_fanotify_mark
#define SYS_fanotify_mark 301
#endif

#ifndef SYS_prlimit64
#define SYS_prlimit64 302
#endif

#ifndef SYS_name_to_handle_at
#define SYS_name_to_handle_at 303
#endif

#ifndef SYS_open_by_handle_at
#define SYS_open_by_handle_at 304
#endif

#ifndef SYS_clock_adjtime
#define SYS_clock_adjtime 305
#endif

#ifndef SYS_syncfs
#define SYS_syncfs 306
#endif

#ifndef SYS_sendmmsg
#define SYS_sendmmsg 307
#endif

#ifndef SYS_setns
#define SYS_setns 308
#endif

#ifndef SYS_getcpu
#define SYS_getcpu 309
#endif

#ifndef SYS_process_vm_readv
#define SYS_process_vm_readv 310
#endif

#ifndef SYS_process_vm_writev
#define SYS_process_vm_writev 311
#endif

#ifndef SYS_kcmp
#define SYS_kcmp 312
#endif

#ifndef SYS_finit_module
#define SYS_finit_module 313
#endif

#ifndef SYS_sched_setattr
#define SYS_sched_setattr 314
#endif

#ifndef SYS_sched_getattr
#define SYS_sched_getattr 315
#endif

#ifndef SYS_renameat2
#define SYS_renameat2 316
#endif

#ifndef SYS_seccomp
#define SYS_seccomp 317
#endif

#ifndef SYS_getrandom
#define SYS_getrandom 318
#endif

#ifndef SYS_memfd_create
#define SYS_memfd_create 319
#endif

#ifndef SYS_kexec_file_load
#define SYS_kexec_file_load 320
#endif

#ifndef SYS_bpf
#define SYS_bpf 321
#endif

#ifndef SYS_execveat
#define SYS_execveat 322
#endif

#ifndef SYS_userfaultfd
#define SYS_userfaultfd 323
#endif

#ifndef SYS_membarrier
#define SYS_membarrier 324
#endif

#ifndef SYS_mlock2
#define SYS_mlock2 325
#endif

#ifndef SYS_copy_file_range
#define SYS_copy_file_range 326
#endif

#ifndef SYS_preadv2
#define SYS_preadv2 327
#endif

#ifndef SYS_pwritev2
#define SYS_pwritev2 328
#endif

#ifndef SYS_pkey_mprotect
#define SYS_pkey_mprotect 329
#endif

#ifndef SYS_pkey_alloc
#define SYS_pkey_alloc 330
#endif

#ifndef SYS_pkey_free
#define SYS_pkey_free 331
#endif

#ifndef SYS_statx
#define SYS_statx 332
#endif

namespace NOPTrace {
    const char* StrSyscallName(int syscall);
}
