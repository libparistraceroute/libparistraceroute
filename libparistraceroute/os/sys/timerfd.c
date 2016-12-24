#include "timerfd.h"

#ifdef FREEBSD

int timerfd_settime(
    int fd, int flags,
    //const struct itimerspec *new_value,
    //struct itimerspec *old_value
    void *new_value,
    void *old_value
) {
    fprintf(stderr, "os: timerfd_settime: NOT IMPLEMENTED");
    return -1;
}

int timerfd_create(int clockid, int flags){
    fprintf(stderr, "os: timerfd_create: NOT IMPLEMENTED");
    return -1;
}

#endif


