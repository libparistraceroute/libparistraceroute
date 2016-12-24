#ifndef OS_SYS_EVENTFD
#define OS_SYS_EVENTFD

#include "../os.h"

#ifdef LINUX
#  include <sys/eventfd.h>
#endif

#ifdef FREEBSD
#  include <stdio.h>
#  include <stdint.h>

/* Type for event counter.  */
typedef uint64_t eventfd_t;

/* Flags for eventfd.  */
enum
{
    EFD_SEMAPHORE = 00000001,
#define EFD_SEMAPHORE EFD_SEMAPHORE
    EFD_CLOEXEC = 02000000,
#define EFD_CLOEXEC EFD_CLOEXEC
    EFD_NONBLOCK = 00004000
#define EFD_NONBLOCK EFD_NONBLOCK
};

int eventfd(unsigned int initval, int flags);
int eventfd_write(unsigned int initval, int flags);

#endif

#endif // OS_SYS_EVENTFD
