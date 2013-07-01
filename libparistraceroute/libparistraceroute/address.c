#include "address.h"

#include <stdio.h>      // perror
#include <stdlib.h>     // malloc
#include <errno.h>      // errno, ENOMEM, EINVAL
#include <string.h>     // memcpy
#include <netdb.h>      // getnameinfo, getaddrinfo
#include <sys/socket.h> // getnameinfo, getaddrinfo, sockaddr_*
#include <netinet/in.h> // INET_ADDRSTRLEN, INET6_ADDRSTRLEN
#include <arpa/inet.h>  // inet_pton

#define AI_IDN        0x0040

// CPPFLAGS += -D_GNU_SOURCE

static void ip_dump(int family, const void * ip, char * buffer, size_t buffer_len) {
    if (inet_ntop(family, ip, buffer, buffer_len)) {
        printf(buffer);
    } else {
        printf("???");
    }
}

int ip_from_string(int family, const char * hostname, ip_t * ip)
{
    struct addrinfo   hints,
                    * ai,
                    * res = NULL;
    int               ret;
    void            * addr;
    size_t            addr_len;

    // Initialize hints
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = family;
    hints.ai_flags = AI_IDN;

    // Convert string hostname / IP into a sequence of addrinfo instances
    if ((ret = getaddrinfo(hostname, NULL, &hints, &res)) != 0) {
        goto ERROR_GETADDRINFO;
    }

    // Find the first addrinfo with the appropriate family
    for (ai = res; ai; ai = ai->ai_next) {
        if (ai->ai_family == family) break;
    }

    // Not found! Pass the last addrinfo
    if (!ai) ai = res;

    // Extract from sockaddr the address where is stored the IP
    switch (family) {
        case AF_INET:
            addr = &(((struct sockaddr_in  *) ai->ai_addr)->sin_addr);
            addr_len = sizeof(ipv4_t);
            break;
        case AF_INET6:
            addr = &(((struct sockaddr_in6 *) ai->ai_addr)->sin6_addr);
            addr_len = sizeof(ipv6_t);
            break;
        default:
            ret = EINVAL;
            goto ERROR_FAMILY;
    }

    // Fill the address_t structure
    memcpy(ip, addr, addr_len);
ERROR_FAMILY:
    freeaddrinfo(res);
ERROR_GETADDRINFO:
    return ret;
}

void ipv4_dump(const ipv4_t * ipv4) {
    char buffer[INET_ADDRSTRLEN];
    ip_dump(AF_INET, ipv4, buffer, INET_ADDRSTRLEN);
}

void ipv6_dump(const ipv6_t * ipv6) {
    char buffer[INET6_ADDRSTRLEN];
    ip_dump(AF_INET6, ipv6, buffer, INET6_ADDRSTRLEN);
}

void address_dump(const address_t * address) {
    char buffer[INET6_ADDRSTRLEN];
    switch (address->family) {
        case AF_INET : printf("IPv4: "); break;
        case AF_INET6: printf("IPv6: "); break;
    }
    ip_dump(address->family, &address->ip, buffer, INET6_ADDRSTRLEN);
}

bool address_guess_family(const char * str_ip, int * pfamily) {
    struct addrinfo   hints,
                    * result;
    int               err ;

    memset(&hints, 0, sizeof(struct addrinfo));
    hints.ai_family   = AF_UNSPEC;    // Allow IPv4 or IPv6
    hints.ai_socktype = SOCK_DGRAM;
    hints.ai_flags    = AI_PASSIVE;

    if ((err = getaddrinfo(str_ip, NULL, &hints, &result)) != 0) {
        fprintf(stderr, gai_strerror(err));
        goto ERR_GETADDRINFO;
    }

    if (!result) goto ERR_NO_RESULT;

    /*
    printf("==>\n");
    for (rp = result; rp != NULL; rp = rp->ai_next) {
        printf("family = %d\n", rp->ai_family);
    }
    */

    // We retrieve family from the first result
    *pfamily = result->ai_family;
    return true;

ERR_NO_RESULT:
ERR_GETADDRINFO:
    fprintf(stderr, "Invalid address (%s): %s\n", str_ip, gai_strerror(err));
    return false;
}
int address_from_string(int family, const char * hostname, address_t * address) {
    address->family = family;
    return ip_from_string(family, hostname, &address->ip);
}

int address_to_string(const address_t * address, char ** pbuffer)
{
    struct sockaddr     * sa;
    struct sockaddr_in    sa4;
    struct sockaddr_in6   sa6;
    socklen_t             sa_len;
    int                   ret;

    switch (address->family) {
        case AF_INET:
            sa = (struct sockaddr *) &sa4;
            sa4.sin_family = address->family;
            sa4.sin_port   = 0;
            sa4.sin_addr   = address->ip.ipv4;
            sa_len         = sizeof(struct sockaddr_in);
            break;
        case AF_INET6:
            sa = (struct sockaddr *) &sa6;
            sa6.sin6_family = address->family;
            sa6.sin6_port   = 0;
            sa6.sin6_addr   = address->ip.ipv6;
            sa_len          = sizeof(struct sockaddr_in6);
            break;
        default:
            *pbuffer = NULL;
            return EINVAL;
    }

    if (!(*pbuffer = malloc(NI_MAXHOST))) {
        return -1;
    }

    if ((ret = getnameinfo(sa, sa_len, *pbuffer, NI_MAXHOST, NULL, 0, NI_NUMERICHOST))) {
        fprintf("address_to_string: %s", gai_strerror(ret));
    }

    return ret;
}

bool address_resolv(const char * str_ip, char ** phostname)
{
    ip_t             ip;
    struct hostent * hp;
    int              family;
    size_t           ip_len;
    bool             ret;

    if (!str_ip) goto ERR_INVALID_PARAMETER;

    if (!(address_guess_family(str_ip, &family))) {
        goto ERR_ADDRESS_GUESS_FAMILY;
    }

    switch (family) {
        case AF_INET:  ip_len = sizeof(ipv4_t); break;
        case AF_INET6: ip_len = sizeof(ipv6_t); break;
        default:
            perror("address_resolv: Invalid family");
            goto ERR_INVALID_FAMILY;
    }

    // Can't parse str_ip
    if (!inet_pton(family, str_ip, &ip)) {
        perror("address_resolv: Can't parse IP address");
        goto ERR_INET_PTON;
    }

    if ((ret = (hp = gethostbyaddr(&ip, ip_len, family)) != NULL)) {
        *phostname = strdup(hp->h_name);
    }
    return ret;

ERR_INET_PTON:
ERR_INVALID_FAMILY:
ERR_ADDRESS_GUESS_FAMILY:
ERR_INVALID_PARAMETER:
    errno = EINVAL;
    return false;
}
