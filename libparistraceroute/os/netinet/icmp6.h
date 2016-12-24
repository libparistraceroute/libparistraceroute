#ifndef OS_NETINET_ICMP6
#define OS_NETINET_ICMP6

#include "../os.h"

#ifdef LINUX
#  include <netinet/icmp6.h>
#endif

#ifdef FREEBSD
#  include "in.h"
#  include <sys/types.h>        // u_int_*_t
#  include <netinet/icmp6.h>
#endif

#endif // OS_NETINET_ICMP6
