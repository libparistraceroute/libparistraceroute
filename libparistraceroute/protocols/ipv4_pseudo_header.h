#include "use.h"
#ifdef USE_IPV4

#ifndef IPV4_PSEUDO_HEADER_H
#define IPV4_PSEUDO_HEADER_H

#include "buffer.h"      // buffer_t

/**
 * IPv4 pseudo header
 */

typedef struct {
    uint32_t  ip_src;    /**< Source IPv4 */
    uint32_t  ip_dst;    /**< Destination IPv4 */
    uint8_t   zero;      /**< Zeros */
    uint8_t   protocol;  /**< Protocol number of the first nested protocol (ex: UDP == 17) */
    uint16_t  size;      /**< Size of IP layer contents (IP packet size - IP header size)  */
} ipv4_pseudo_header_t;

/**
 * \brief Create an IPv4 pseudo header
 * \param ipv4_segment Address of the IPv4 segment 
 * \return The buffer containing the corresponding pseudo header,
 *    NULL in case of failure
 */

buffer_t * ipv4_pseudo_header_create(const uint8_t * ipv4_segment);

#endif
#endif // USE_IPV4

