#ifndef OS_SYS_SIGNALFD
#define OS_SYS_SIGNALFD

#include "../os.h"

#ifdef LINUX
#  include <sys/signalfd.h>
#endif

#ifdef FREEBSD
#  include <stdio.h>
#  include <signal.h>

typedef __signed__ char __s8;
typedef unsigned char __u8;

typedef __signed__ short __s16;
typedef unsigned short __u16;

typedef __signed__ int __s32;
typedef unsigned int __u32;

typedef __signed__ long __s64;
typedef unsigned long __u64;

struct signalfd_siginfo {
    __u32 ssi_signo;
    __s32 ssi_errno;
    __s32 ssi_code;
    __u32 ssi_pid;
    __u32 ssi_uid;
    __s32 ssi_fd;
    __u32 ssi_tid;
    __u32 ssi_band;
    __u32 ssi_overrun;
    __u32 ssi_trapno;
    __s32 ssi_status;
    __s32 ssi_int;
    __u64 ssi_ptr;
    __u64 ssi_utime;
    __u64 ssi_stime;
    __u64 ssi_addr;
    __u16 ssi_addr_lsb;

    /*
     * Pad strcture to 128 bytes. Remember to update the
     * pad size when you add new members. We use a fixed
     * size structure to avoid compatibility problems with
     * future versions, and we leave extra space for additional
     * members. We use fixed size members because this strcture
     * comes out of a read(2) and we really don't want to have
     * a compat on read(2).
     */
    __u8 __pad[46];
};

int signalfd(int fd, const sigset_t *mask, int flags);

#endif

#endif // OS_SYS_EPOLL
