#ifndef ADDRESS_H
#define ADDRESS_H

#include <string.h>     // memcpy
#include <netinet/in.h>

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
 * \brief Initialize an address_t according to a string
 * \param string An IP address (human readable format) or a hostname)
 * \return see getaddrinfo's returned values 
 */

int address_from_string(const char * string, address_t * address);

/**
 * \brief Print an address
 * \param ipThe address to print
 */

void ipv4_dump(ipv4_t ip);

/**
 * \brief Print an address
 * \param address The address to print
 */

void address_dump(const address_t * address);

/**
 * \brief Convert an IP address into a human readable string
 * \param addr The address that must be converted
 * \param pbuffer The address of a char * that will be updated to point
 *    to an allocated buffer.
 * \return The value returned by getnameinfo
 */

int address_to_string(const address_t * addr, char ** pbuffer); 

/**
 * \brief Converts an IP stored in a string into its corresponding hostname
 * \param str_ip A string containing either an IPv4 or either an IPv6 address
 * \return NULL if no hostname has been successfully resolv, the corresponding
 *    hostname otherwise
 */

char * address_resolv(const char * str_ip);

#endif 
