#ifndef OS_NETINET_IP_ICMP
#define OS_NETINET_IP_ICMP

#include "../os.h"

#ifdef LINUX
#  include <netinet/ip_icmp.h>

// FreeBSD and Linux headers are not consistent
#  define ICMPV4_TYPE       type
#  define ICMPV4_CODE       code
#  define ICMPV4_CHECKSUM   checksum

#endif

#ifdef FREEBSD
#  include <sys/types.h>        // u_int_*_t
#  include "in.h"               // struct in_addr
#  include "ip.h"               // struct ip
#  include <netinet/ip_icmp.h>

// FreeBSD and Linux headers are not consistent
#  define ICMPV4_TYPE       icmp_type
#  define ICMPV4_CODE       icmp_code
#  define ICMPV4_CHECKSUM   icmp_cksum

// Some #define are missing in the FreeBSD header
#  define ICMP_DEST_UNREACH 3   // From <netinet/ip_icmp.h>
#  define ICMP_TIME_EXCEEDED 11  // From <netinet/ip_icmp.h>

#endif

#endif // OS_NETINET_IP_ICMP
