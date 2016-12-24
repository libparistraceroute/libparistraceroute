#ifndef OS_NETINET_IN
#define OS_NETINET_IN

#include "../os.h"

#ifdef LINUX
#  include <netinet/in.h>
#endif

#ifdef FREEBSD
#  include <sys/_types.h> // __sa_family_t
#  include <netinet/in.h>

// Under FreeBSD, struct in6-addr is implemented in <netinet6/in6.h>
// Some macros are disabled if _KERNEL is not defined, and we use
// them in libparistraceroute. With the following we ensure they are set.
#  ifndef _KERNEL
#    define s6_addr16 __u6_addr.__u6_addr16
#    define s6_addr32 __u6_addr.__u6_addr32
#  endif

#endif

#endif // OS_NETINET_IN
