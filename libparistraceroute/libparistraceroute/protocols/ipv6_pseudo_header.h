#ifndef IPV6_PSEUDO_HEADER_H
#define IPV6_PSEUDO_HEADER_H

#include "buffer.h"      // buffer_t
#include "field.h"       // uint128_t

/**
 * IPv6 pseudo header: http://tools.ietf.org/html/rfc2460
 */

typedef struct {
    uint128_t ip_src;    /**< Source IPv6      */
    uint128_t ip_dst;    /**< Destination IPv6 */
    uint32_t  size;      /**< Size of IP layer contents (IP packet size - IP header size)  */ 
    uint16_t  zeros;     /**< Zeros */
    uint8_t   zero;      /**< Zeros */
    uint8_t   protocol;  /**< Protocol number of the first nested protocol (ex: UDP == 17) */
} ipv6_pseudo_header_t;

/**
 * \brief Create an IPv6 pseudo header
 * \param ipv6_segment Address of the IPv4 segment 
 * \return The buffer containing the corresponding pseudo header,
 *    NULL in case of failure
 */

buffer_t * ipv6_pseudo_header_create(const uint8_t * ipv6_segment);

#endif

