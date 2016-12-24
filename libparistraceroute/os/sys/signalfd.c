#include "signalfd.h"

#ifdef FREEBSD

int signalfd(int fd, const sigset_t *mask, int flags) {
    fprintf(stderr, "os: signalfd: NOT IMPLEMENTED");
    return 0;
}

#endif

