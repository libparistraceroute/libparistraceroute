#include "address.h"

#include <stdio.h>
#include <stdlib.h>     // malloc
#include <errno.h>      // errno, ENOMEM, EINVAL
#include <string.h>     // memcpy
#include <netdb.h>      // getnameinfo, getaddrinfo
#include <sys/socket.h> // getnameinfo, getaddrinfo, sockaddr_*
#include <netinet/in.h> // INET_ADDRSTRLEN, INET6_ADDRSTRLEN
#include <arpa/inet.h>  // inet_pton

#define AI_IDN        0x0040

// CPPFLAGS += -D_GNU_SOURCE

int address_guess_family(const char * str_ip)
{
    return (strchr(str_ip, '.') != NULL) ? AF_INET : AF_INET6;
}

void address_dump(const address_t * address)
{
    char buffer[INET6_ADDRSTRLEN];
    inet_ntop(address->family, &address->ip, buffer, INET6_ADDRSTRLEN);
    printf("%s", buffer);
}

int address_from_string(const char * hostname, address_t * address)
{
    struct addrinfo   hints,
                    * ai,
                    * res = NULL;
    int               ret;
    void            * addr;

    // This is not wonderful because "www.google.fr" will be considered as IPv4
    int               family = address_guess_family(hostname);

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
            addr = &(((struct sockaddr_in  *) ai->ai_addr)->sin_addr );
            break;
        case AF_INET6:
            addr = &(((struct sockaddr_in6 *) ai->ai_addr)->sin6_addr);
            break;
        default:
            ret = EINVAL;
            goto ERROR_FAMILY;
    }

    // Fill the address_t structure
    memcpy(&address->ip, addr, ai->ai_addrlen);
    address->family = family;
ERROR_FAMILY:
    freeaddrinfo(res);
    return ret;
ERROR_GETADDRINFO:
    return ret;
}

int address_to_string(const address_t * address, char ** pbuffer)
{
    struct sockaddr     * sa;
    struct sockaddr_in    sa4;
    struct sockaddr_in6   sa6;
    socklen_t             sa_len;
    size_t                buffer_len;

    switch (address->family) {
        case AF_INET:
            sa = (struct sockaddr *) &sa4;
            sa4.sin_family = address->family;
            sa4.sin_port   = 0;
            sa4.sin_addr   = address->ip.ipv4;
            sa_len         = sizeof(struct sockaddr_in);
            buffer_len     = INET_ADDRSTRLEN;
            break;
        case AF_INET6:
            sa = (struct sockaddr *) &sa6;
            sa6.sin6_family = address->family;
            sa6.sin6_port   = 0;
            sa6.sin6_addr   = address->ip.ipv6;
            sa_len          = sizeof(struct sockaddr_in6);
            buffer_len      = INET6_ADDRSTRLEN;
            break;
        default:
            *pbuffer = NULL;
            return EINVAL;
    }

    if (!(*pbuffer = malloc(buffer_len))) {
        return ENOMEM;
    }

    return getnameinfo(sa, sa_len, *pbuffer, buffer_len, NULL, 0, NI_NUMERICHOST);
}

bool address_resolv(const char * str_ip, char ** phostname)
{
    ip_t             ip;
    struct hostent * hp;
    int              family;
    size_t           ip_len;
    bool             ret;
    
    if (!str_ip) goto ERROR;

    family = address_guess_family(str_ip);

    ip_len = family == AF_INET  ? sizeof(struct sockaddr_in)  :
             family == AF_INET6 ? sizeof(struct sockaddr_in6) :
             0;

    // Invalid family
    if (!ip_len) {
        perror("address_resolv: Invalid family");
        goto ERROR;
    }

    // Can't parse str_ip
    if (!inet_pton(family, str_ip, &ip)) {
        perror("address_resolv: Can't parse IP address");
        goto ERROR;
    }

    if ((ret = (hp = gethostbyaddr(&ip, ip_len, family)) != NULL)) {
        *phostname = strdup(hp->h_name);
    }
    return ret; 
ERROR:
    errno = EINVAL;
    return false;
}
