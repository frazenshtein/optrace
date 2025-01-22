#pragma once

#ifndef SYS_recvmmsg
    #define SYS_recvmmsg 243
#endif

#ifndef SYS_fanotify_init
    #define SYS_fanotify_init 262
#endif

#ifndef SYS_fanotify_mark
    #define SYS_fanotify_mark 263
#endif

#ifndef SYS_prlimit64
    #define SYS_prlimit64 261
#endif

#ifndef SYS_name_to_handle_at
    #define SYS_name_to_handle_at 264
#endif

#ifndef SYS_open_by_handle_at
    #define SYS_open_by_handle_at 265
#endif

#ifndef SYS_clock_adjtime
    #define SYS_clock_adjtime 266
#endif

#ifndef SYS_syncfs
    #define SYS_syncfs 267
#endif

#ifndef SYS_sendmmsg
    #define SYS_sendmmsg 269
#endif

#ifndef SYS_setns
    #define SYS_setns 268
#endif

#ifndef SYS_getcpu
    #define SYS_getcpu 168
#endif

#ifndef SYS_process_vm_readv
    #define SYS_process_vm_readv 270
#endif

#ifndef SYS_process_vm_writev
    #define SYS_process_vm_writev 271
#endif

#ifndef SYS_kcmp
    #define SYS_kcmp 272
#endif

#ifndef SYS_finit_module
    #define SYS_finit_module 273
#endif

#ifndef SYS_sched_setattr
    #define SYS_sched_setattr 274
#endif

#ifndef SYS_sched_getattr
    #define SYS_sched_getattr 275
#endif

#ifndef SYS_renameat2
    #define SYS_renameat2 275
#endif

#ifndef SYS_seccomp
    #define SYS_seccomp 277
#endif

#ifndef SYS_getrandom
    #define SYS_getrandom 278
#endif

#ifndef SYS_memfd_create
    #define SYS_memfd_create 279
#endif

#ifndef SYS_bpf
    #define SYS_bpf 280
#endif

#ifndef SYS_execveat
    #define SYS_execveat 281
#endif

#ifndef SYS_userfaultfd
    #define SYS_userfaultfd 282
#endif

#ifndef SYS_membarrier
    #define SYS_membarrier 283
#endif

#ifndef SYS_mlock2
    #define SYS_mlock2 284
#endif

#ifndef SYS_copy_file_range
    #define SYS_copy_file_range 285
#endif

#ifndef SYS_preadv2
    #define SYS_preadv2 286
#endif

#ifndef SYS_pwritev2
    #define SYS_pwritev2 287
#endif

#ifndef SYS_pkey_mprotect
    #define SYS_pkey_mprotect 288
#endif

#ifndef SYS_pkey_alloc
    #define SYS_pkey_alloc 289
#endif

#ifndef SYS_pkey_free
    #define SYS_pkey_free 290
#endif

#ifndef SYS_statx
    #define SYS_statx 291
#endif
