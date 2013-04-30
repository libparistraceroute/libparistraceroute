#include "address.h"

#include <stdlib.h>
#include <errno.h>      // errno, ENOMEM, EINVAL
#include <netdb.h>
#include <string.h>     // memcpy
#include <sys/types.h>  // getaddrinfo etc.
#include <sys/socket.h> // sockaddr_*
#include <netinet/in.h> // INET_ADDRSTRLEN, INET6_ADDRSTRLEN
#include <arpa/inet.h>  // inet_pton

#define AI_IDN        0x0040
#define SIZEOF_INET   32
#define SIZEOF_INET6  128

/******************************************************************************
 * Helper functions
 ******************************************************************************/

// CPPFLAGS += -D_GNU_SOURCE

int address_guess_family(const char * str_ip) {
    return (strchr(str_ip, '.') != NULL) ?
        AF_INET :
        AF_INET6;
}

int address_from_string(const char * hostname, address_t * address)
{
    struct addrinfo   hints,
                    * ai,
                    * res = NULL;
    int               ret;
    int               af = address_guess_family(hostname);

    // Initialize hints
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = af;
    hints.ai_flags = AI_IDN;

    // Convert string hostname / IP into a sequence of addrinfo instances
    if ((ret = getaddrinfo(hostname, NULL, &hints, &res)) == 0) {
        // Find the first addrinfo with the appropriate family
        for (ai = res; ai; ai = ai->ai_next) {
            if (ai->ai_family == af) break;
        }

        // Not found! Pass the last addrinfo
        if (!ai) ai = res;

        memcpy(&address->address, ai->ai_addr, ai->ai_addrlen);
        address->family = af;
        freeaddrinfo(res);
    }

    return ret;
}

int address_to_string(const address_t * address, char ** pbuffer)
{
    struct sockaddr     * sa;
    struct sockaddr_in    sin4;
    struct sockaddr_in6   sin6;
    socklen_t             salen;
    size_t                hostlen;
    int ret;

    switch (address->family) {
        case AF_INET:
            sa = (struct sockaddr *) &sin4;
            sin4.sin_family = address->family;
            sin4.sin_port   = 0;
            sin4.sin_addr   = address->address.sin;
            salen = SIZEOF_INET;
            hostlen = INET_ADDRSTRLEN;
            break;
        case AF_INET6:
            sa = (struct sockaddr *) &sin6;
            sin6.sin6_family = address->family;
            sin6.sin6_port   = 0;
            sin6.sin6_addr   = address->address.sin6;
            salen = SIZEOF_INET6;
            hostlen = INET6_ADDRSTRLEN;
            break;
        default:
            *pbuffer = NULL;
            return EINVAL;
    }

    if (!(*pbuffer = malloc(hostlen))) {
        return ENOMEM;
    }

    ret = getnameinfo(sa, salen, *pbuffer, hostlen, NULL, 0, NI_NUMERICHOST);
    return ret;
}

char * address_resolv(const char * str_ip) {
    sin_union_t      address;
    struct hostent * hp;
    int              af_type;
    size_t           len;
    
    if (strchr(str_ip, '.') != NULL) {
        len = SIZEOF_INET;
        af_type = AF_INET;
    } else {
        len = SIZEOF_INET6;
        af_type = AF_INET6;
    }

    if (!inet_pton(af_type, str_ip, &address)) {
        // Can't parse address
        errno = EINVAL;
        return NULL;
    }

    hp = gethostbyaddr(&address, len, af_type);
    return hp ? hp->h_name : NULL;
}
