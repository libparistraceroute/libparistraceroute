#ifndef OS_NETINET_TCP
#define OS_NETINET_TCP

#include "../os.h"

#ifdef LINUX
#  include <netinet/udp.h>
#  define SRC_PORT source
#  define DST_PORT dest
#  define LENGTH   len
#  define CHECKSUM check
#endif

#ifdef FREEBSD
#  include <sys/types.h>    // u_char, u_int*_t
#  include <netinet/udp.h>
#  define SRC_PORT uh_sport
#  define DST_PORT uh_dport
#  define LENGTH   uh_ulen
#  define CHECKSUM uh_sum
#endif

#endif // OS_NETINET_TCP
