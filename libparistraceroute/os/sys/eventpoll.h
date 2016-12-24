#ifndef OS_EVENTPOLL
#define OS_EVENTPOLL

#include "../os.h"

#ifdef LINUX
#  include <sys/eventpoll.h>
#endif

#ifdef FREEBSD
#  define EPOLL_CTL_ADD 1
#  define EPOLL_CTL_DEL 2
#  define EPOLL_CTL_MOD 3
#endif

#endif // OS_SYS_EVENTPOLL
