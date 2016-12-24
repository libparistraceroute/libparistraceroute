#ifndef OS_NETINET_IP
#define OS_NETINET_IP

#include "../os.h"

#ifdef LINUX
#  include <netinet/ip.h>
#endif

#ifdef FREEBSD
#  include <sys/types.h>    // u_char, u_int*_t
#  include "in.h"           // struct in_addr
#  include <netinet/ip.h>

    // "struct iphdr" does not exist under FreeBSD.
    // This is not "struct ip".
    // From <netinet/ip.h> Linux header
    // Mixed with FreeBSD #define (see <netinet/ip.h> FreeBSD header)

    struct iphdr
    {
#  if BYTE_ORDER == LITTLE_ENDIAN
        unsigned int ihl:4;
        unsigned int version:4;
#  elif BYTE_ORDER == BIG_ENDIAN
        unsigned int version:4;
        unsigned int ihl:4;
#  endif
        u_int8_t  tos;
        u_int16_t tot_len;
        u_int16_t id;
        u_int16_t frag_off;
        u_int8_t  ttl;
        u_int8_t  protocol;
        u_int16_t check;
        u_int32_t saddr;
        u_int32_t daddr;
        /* The options start here. */
    };

#endif

#endif // OS_NETINET_IP
