#include "eventfd.h"

#ifdef FREEBSD

int eventfd(unsigned int initval, int flags) {
    fprintf(stderr, "os: eventfd: NOT IMPLEMENTED");
    return -1;
}

int eventfd_write(unsigned int initval, int flags) {
    fprintf(stderr, "os: eventfd_write: NOT IMPLEMENTED");
    return -1;
}

#endif
