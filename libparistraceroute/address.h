#ifndef LIBPT_ADDRESS_H
#define LIBPT_ADDRESS_H

#include <stdbool.h>    // bool
#include <netinet/in.h> // in_addr, in6_addr

#include "use.h"

#ifdef __cplusplus
extern "C" {
#endif

//---------------------------------------------------------------------------
// ip*_t
//---------------------------------------------------------------------------

#ifdef USE_IPV4
typedef struct in_addr  ipv4_t;
#endif

#ifdef USE_IPV6
typedef struct in6_addr ipv6_t;
#endif

typedef union {
#ifdef USE_IPV4
    ipv4_t ipv4;
#endif
#ifdef USE_IPV6
    ipv6_t ipv6;
#endif
} ip_t;

#ifdef USE_IPV4
/**
 * \brief Print an IPv4 address
 * \param ipv4 The address to print
 */

void ipv4_dump(const ipv4_t * ipv4);
#endif

#ifdef USE_IPV6
/**
 * \brief Print an IPv6 address
 * \param ipv6 The address to print
 */

void ipv6_dump(const ipv6_t * ipv6);
#endif

//---------------------------------------------------------------------------
// address_t
//---------------------------------------------------------------------------

typedef struct {
    int  family;  /**< Address family: AF_INET or AF_INET6 */
    ip_t ip;      /**< IP address (binary) */
} address_t;

/**
 * \brief Allocate a new address_t instance. Most of time you could
 *    directly use address_from_string.
 * \return The newly created address_t instance if successful,
 *    NULL otherwise.
 */

address_t * address_create();

/**
 * \brief Compare two address_t instances.
 * \param x The first address_t instance.
 * \param y The second address_t instance.
 * \return A value < 0 if x is lower than y,
 *         a value > 0 if y is lower than x,
 *         0 if x is equal to y.
 * If the both addresses do not belong to the same family
 * the value, the addresses are only compared on their family.
 */

int address_compare(const address_t * x, const address_t * y);

/**
 * \brief Release an address_t instance from the memory.
 * \param address An address instance.
 */

void address_free(address_t * address);

/**
 * \brief Initialize an address_t according to a string.
 * \param family Address family (AF_INET or AF_INET6).
 *    \sa address_guess_family
 * \param hostname An IP address (string format) or a FQDN.
 * \param address A preallocated address_t instance.
 * \return see getaddrinfo's returned values (0 if successful).
 */

int address_from_string(int family, const char * hostname, address_t * address);

/**
 * \brief Duplicate an address.
 * \param address The address to copy.
 * \return The copied address if successful, NULL otherwise
 */

address_t * address_dup(const address_t * address);

/**
 * \brief Retrieve the size of the nested IP address of an
 *    address_t instance.
 * \param address An address instance.
 * \param The size of the nested IP address if successful, 0 otherwise.
 */

size_t address_get_size(const address_t * address);

/**
 * \brief Print an address
 * \param address The address to print
 * \param oIPStr if not ULL, it's a preallocated string to store a IP in string format
 * \param iIPStrLen the length in bytes of \coIPStr
 */

void address_dump(const address_t * address, char * oIPStr, uint8_t iIPStrLen);

/**
 * \brief Guess address family of an IP by using the
 *    first result of getaddrinfo (if any).
 * \param str_ip An IP address (string format)
 * \param pfamily Address of an integer in which the address
 *   family will be written. *pfamily may be equal to AF_INET
 *   AF_INET6, etc.
 * \return true iif successfull
 */

bool address_guess_family(const char * str_ip, int * pfamily);

/**
 * \brief Initialize an ip_t instance according to a string
 * \param family Address family (AF_INET or AF_INET6)
 * \param hostname An IP address (human readable format) or a hostname)
 * \param ip A pre-allocated ip_t that we update
 * \return see getaddrinfo's returned values
 */

int ip_from_string(int family, const char * hostname, ip_t * ip);

/**
 * \brief Convert an IP address into a human readable string
 * \param addr The address that must be converted
 * \param pbuffer The address of a char * that will be updated to point
 *    to an allocated buffer.
 * \return The value returned by getnameinfo
 */

int address_to_string(const address_t * addr, char ** pbuffer);

/**
 * \brief Converts an IP stored in a string into its corresponding hostname.
 *    DNS lookups may be cached to improve the overall performance. This
 *    cache is shared between all running algorithm instance using
 *    libparistraceroute.
 * \param address An address_t instance
 * \param phostname Pass a pointer initialized to NULL.
 *    *phostname is automatically allocated if it is required.
 *    If the resolution fails, *phostname remains equal to NULL.
 *    Otherwise, *phostname points to the FQDN and must be freed once it is no more used.
 * \param mask_cache A value among:
 *    CACHE_DISABLED : do not feed nor read the cache
 *    CACHE_WRITE    : perform a DNS lookup and store the result in the cache
 *    CACHE_READ     : use cached DNS lookups, if not found, perform a dns lookup
 *    CACHE_ENABLED  : see IP_HOSTNAME_CACHE_WRITE and IP_HOSTNAME_CACHE_READ
 * \return true iif successfull
 */

#define CACHE_DISABLED  0
#define CACHE_WRITE    (1 << 0)
#define CACHE_READ     (1 << 1)
#define CACHE_ENABLED  (CACHE_READ | CACHE_WRITE)

bool address_resolv(const address_t * address, char ** phostname, int mask_cache);

#ifdef __cplusplus
}
#endif

#endif // LIBPT_ADDRESS_H
