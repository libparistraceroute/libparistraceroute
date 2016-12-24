#include "epoll.h"

#ifdef FREEBSD

int epoll_create1(int flags) {
    fprintf(stderr, "os: epoll_create1: NOT IMPLEMENTED");
    return -1;
}

int epoll_ctl(int epfd, int op, int fd, struct epoll_event *event) {
    fprintf(stderr, "os: epoll_ctl: NOT IMPLEMENTED");
    return -1;
}

int epoll_wait(int epfd, struct epoll_event *events, int maxevents, int timeout) {
    fprintf(stderr, "os: epoll_wait: NOT IMPLEMENTED");
    return -1;
}

#endif

