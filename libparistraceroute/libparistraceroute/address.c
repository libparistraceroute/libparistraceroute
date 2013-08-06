#include "use.h"
#include "config.h"

#include <stdio.h>      // perror
#include <stdlib.h>     // malloc
#include <errno.h>      // errno, ENOMEM, EINVAL
#include <string.h>     // memcpy
#include <netdb.h>      // getnameinfo, getaddrinfo
#include <sys/socket.h> // getnameinfo, getaddrinfo, sockaddr_*
#include <netinet/in.h> // INET_ADDRSTRLEN, INET6_ADDRSTRLEN
#include <arpa/inet.h>  // inet_pton

#include "address.h"

#ifdef USE_CACHE
#    include "containers/map.h"

static map_t * cache_ip_hostname = NULL;

static void __cache_ip_hostname_create() __attribute__((constructor));
static void __cache_ip_hostname_free()   __attribute__((destructor(200)));

static void str_dump(const char * s) {
    printf("%s (%p)", s, s);
}

static void __cache_ip_hostname_create() {
    cache_ip_hostname = map_create(
        address_dup, address_free, address_dump, address_compare,
        strdup,      free,         str_dump
    );
}

static void __cache_ip_hostname_free() {
    if (cache_ip_hostname) map_free(cache_ip_hostname);
}

#endif

#define AI_IDN        0x0040

static void ip_dump(int family, const void * ip, char * buffer, size_t buffer_len) {
    if (inet_ntop(family, ip, buffer, buffer_len)) {
        printf("%s", buffer);
    } else {
        printf("???");
    }
}

int ip_from_string(int family, const char * hostname, ip_t * ip) {
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
#ifdef USE_IPV4
        case AF_INET:
            addr = &(((struct sockaddr_in  *) ai->ai_addr)->sin_addr);
            addr_len = sizeof(ipv4_t);
            break;
#endif
#ifdef USE_IPV6
        case AF_INET6:
            addr = &(((struct sockaddr_in6 *) ai->ai_addr)->sin6_addr);
            addr_len = sizeof(ipv6_t);
            break;
#endif
        default:
            fprintf(stderr, "ip_from_string: Invalid family\n");
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

#ifdef USE_IPV4
void ipv4_dump(const ipv4_t * ipv4) {
    char buffer[INET_ADDRSTRLEN];
    ip_dump(AF_INET, ipv4, buffer, INET_ADDRSTRLEN);
}
#endif

#ifdef USE_IPV6
void ipv6_dump(const ipv6_t * ipv6) {
    char buffer[INET6_ADDRSTRLEN];
    ip_dump(AF_INET6, ipv6, buffer, INET6_ADDRSTRLEN);
}
#endif

void address_dump(const address_t * address) {
    char buffer[INET6_ADDRSTRLEN];
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
        fprintf(stderr, "%s", gai_strerror(err));
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

address_t * address_create() {
    return malloc(sizeof(address_t));
}

void address_free(address_t * address) {
    if (address) free(address);
}

address_t * address_dup(const address_t * address) {
    address_t * dup;
    if ((dup = address_create())) {
        memcpy(dup, address, sizeof(address_t));
    }
    return dup;
}

int address_compare(const address_t * x, const address_t * y) {
    size_t          i, address_size;
    const uint8_t * px, * py;

    if (x->family < y->family) return -1;
    if (x->family > y->family) return 1;

    address_size = address_get_size(x);

    switch (x->family) {
        case AF_INET:
            px = (const uint8_t *) &x->ip.ipv4;
            py = (const uint8_t *) &y->ip.ipv4;
            break;
        case AF_INET6:
            px = (const uint8_t *) &x->ip.ipv6;
            py = (const uint8_t *) &y->ip.ipv6;
            break;
    }

    for (i = 0; (i < address_size) && (*px++ == *py++); i++);
    return *--px - *--py;
}

int address_to_string(const address_t * address, char ** pbuffer)
{
    struct sockaddr     * sa;
    struct sockaddr_in    sa4;
    struct sockaddr_in6   sa6;
    socklen_t             socket_size;
    int                   ret = -1;
    size_t                buffer_size;

    switch (address->family) {
#ifdef USE_IPV4
        case AF_INET:
            sa = (struct sockaddr *) &sa4;
            memset(sa, 0, sizeof(struct sockaddr_in));
            sa4.sin_family = address->family;
            socket_size    = sizeof(struct sockaddr_in);
            sa4.sin_addr   = address->ip.ipv4;
            buffer_size    = INET_ADDRSTRLEN;
            break;
#endif
#ifdef USE_IPV6
        case AF_INET6:
            sa = (struct sockaddr *) &sa6;
            memset(sa, 0, sizeof(struct sockaddr_in6));
            sa6.sin6_family = address->family;
            socket_size     = sizeof(struct sockaddr_in6);
            memcpy(&sa6.sin6_addr, &address->ip.ipv6, sizeof(ipv6_t));
            buffer_size     = INET6_ADDRSTRLEN;
            break;
#endif
        default:
            *pbuffer = NULL;
            fprintf(stderr, "address_to_string: Family not supported (family = %d)\n", address->family);
            goto ERR_INVALID_FAMILY;
    }

    if (!(*pbuffer = malloc(buffer_size))) {
        goto ERR_MALLOC;
    }

    if ((ret = getnameinfo(sa, socket_size, *pbuffer, buffer_size, NULL, 0, NI_NUMERICHOST)) != 0) {
        fprintf(stderr, "address_to_string: %s", gai_strerror(ret));
        goto ERR_GETNAMEINFO;
    }

    return ret;

ERR_GETNAMEINFO:
    free(*pbuffer);
ERR_INVALID_FAMILY:
ERR_MALLOC:
    return ret;
}

size_t address_get_size(const address_t * address) {
    switch (address->family) {
        case AF_INET:  return sizeof(ipv4_t);
        case AF_INET6: return sizeof(ipv6_t);
        default:
            fprintf(stderr, "address_get_size: Invalid family");
            break;
    }
    return 0;
}

bool address_resolv(const address_t * address, char ** phostname, int mask_cache)
{
    struct hostent * hp;
    bool             found = false;

    if (!address) goto ERR_INVALID_PARAMETER;

#ifdef USE_CACHE
    if (cache_ip_hostname && (mask_cache & CACHE_READ)) {
        found = map_find(cache_ip_hostname, address, phostname);
    }
#endif

#ifdef USE_CACHE
    if (!found) {
#endif
        if (!(hp = gethostbyaddr(&address->ip, address_get_size(address), address->family))) {
            // see h_error
            goto ERR_GETHOSTBYADDR;
        }

        if (!(*phostname = strdup(hp->h_name))) {
            goto ERR_STRDUP;
        }
#ifdef USE_CACHE
        if (mask_cache & CACHE_WRITE) {
            map_update(cache_ip_hostname, address, *phostname);
        }
    }
#endif

    return true;

ERR_GETHOSTBYADDR:
    // This is to avoid to get errno set to 22 (EINVAL) if the DNS lookup fails.
    errno = 0;
ERR_STRDUP:
ERR_INVALID_PARAMETER:
    return false;
}
