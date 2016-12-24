#ifndef OS_SYS_TIMERFD
#define OS_SYS_TIMERFD

#include "../os.h"

#ifdef LINUX
#  include <sys/timerfd.h>
#endif

#ifdef FREEBSD
#  include <stdio.h>

int timerfd_settime(
    int fd, int flags,
    //const struct itimerspec *new_value,
    //struct itimerspec *old_value
    void *new_value,
    void *old_value
);

#  define CLOCK_REALTIME 0 // from <linux/time.h>

int timerfd_create(int clockid, int flags);

#endif

#endif // OS_SYS_TIMERFD
