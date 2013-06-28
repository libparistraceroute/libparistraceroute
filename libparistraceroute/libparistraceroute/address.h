#ifndef ADDRESS_H
#define ADDRESS_H

#include <stdbool.h>    // bool
#include <netinet/in.h> // in_addr, in6_addr

typedef struct in_addr  ipv4_t;
typedef struct in6_addr ipv6_t;

typedef union {
    ipv4_t ipv4;
    ipv6_t ipv6;
} ip_t;

typedef struct {
    int  family;  /**< Address family: AF_INET or AF_INET6 */
    ip_t ip;      /**< IP address (binary) */
} address_t;

/**
 * \brief Print an IPv4 address
 * \param ipv4 The address to print
 */

void ipv4_dump(const ipv4_t * ipv4);

/**
 * \brief Print an IPv6 address
 * \param ipv6 The address to print
 */

void ipv6_dump(const ipv6_t * ipv6);

/**
 * \brief Print an address
 * \param address The address to print
 */

void address_dump(const address_t * address);

/**
 * \brief Guess address family of an IP
 * \param str_ip IP (string format)
 * \param pfamily Address of an integer in which the address
 *   family will be written. *pfamily may be equal to AF_INET
 *   AF_INET6, etc.
 * \return true iif successfull
 */

bool address_guess_family(const char * str_ip, int * pfamily);

/**
 * \brief Initialize an address_t according to a string. If hostname
 *    contains a ".", it is considered as an IPv4 host. If hostname
 *    is a FQDN you should use address_ip_from_string().
 * \param hostname An IP address (string format). Do not pass a FQDN.
 * \return see getaddrinfo's returned values
 */

int address_from_string(const char * hostname, address_t * address);

/**
 * \brief Initialize an ip_t instance according to a string
 * \param family Address family (AF_INET or AF_INET6)
 * \param hostname An IP address (human readable format) or a hostname)
 * \param ip A pre-allocated ip_t that we update
 * \return see getaddrinfo's returned values
 */

int address_ip_from_string(int family, const char * hostname, ip_t * ip);

/**
 * \brief Convert an IP address into a human readable string
 * \param addr The address to convert
 * \param pbuffer The address of a char * that will be updated to point
 *    to an allocated buffer.
 * \return The value returned by getnameinfo
 */

int address_to_string(const address_t * addr, char ** pbuffer);

/**
 * \brief Converts an IP stored in a string into its corresponding hostname
 * \param str_ip A string containing either an IPv4 or either an IPv6 address
 * \param phostname Pass a pointer initialized to NULL.
 *    *phostname is automatically allocated if it is required.
 *    If the resolution fails, *phostname remains equal to NULL.
 *    Otherwise, *phostname points to the FQDN and must be freed once it is no more used.
 * \return true iif successfull
 */

bool address_resolv(const char * str_ip, char ** phostname);

// TODO address_resolv(const address_t * address, char ** phostname)

#endif
