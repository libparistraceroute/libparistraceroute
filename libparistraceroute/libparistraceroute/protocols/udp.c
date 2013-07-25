#include "use.h"

#include <stdlib.h>           // malloc()
#include <string.h>           // memcpy()
#include <stdbool.h>          // bool
#include <errno.h>            // ERRNO, EINVAL
#include <stddef.h>           // offsetof()
#include <netinet/udp.h>      // udphdr
#include <netinet/in.h>       // IPPROTO_UDP = 17

#include "../protocol.h"      // csum

#ifdef USE_IPV4
#    include "ipv4_pseudo_header.h"
#endif

#ifdef USE_IPV6
#    include "ipv6_pseudo_header.h"
#endif

#define UDP_DEFAULT_SRC_PORT 2828
#define UDP_DEFAULT_DST_PORT 2828

#define UDP_FIELD_SRC_PORT   "src_port"
#define UDP_FIELD_DST_PORT   "dst_port"
#define UDP_FIELD_LENGTH     "length"
#define UDP_FIELD_CHECKSUM   "checksum"

// XXX mandatory fields ?
// XXX UDP parsing missing

// BSD/Linux abstraction
#ifdef __FAVOR_BSD
#    define SRC_PORT uh_sport
#    define DST_PORT uh_dport
#    define LENGTH   uh_ulen
#    define CHECKSUM uh_sum
#else
#    define SRC_PORT source 
#    define DST_PORT dest 
#    define LENGTH   len
#    define CHECKSUM check 
#endif

/**
 * UDP fields
 */

static protocol_field_t udp_fields[] = {
    {
        .key = UDP_FIELD_SRC_PORT,
        .type = TYPE_UINT16,
        .offset = offsetof(struct udphdr, SRC_PORT),
    }, {
        .key = UDP_FIELD_DST_PORT,
        .type = TYPE_UINT16,
        .offset = offsetof(struct udphdr, DST_PORT),
    }, {
        .key = UDP_FIELD_LENGTH,
        .type = TYPE_UINT16,
        .offset = offsetof(struct udphdr, LENGTH),
    }, {
        .key = UDP_FIELD_CHECKSUM,
        .type = TYPE_UINT16,
        .offset = offsetof(struct udphdr, CHECKSUM),
        // optional = 0
    },
    END_PROTOCOL_FIELDS
};

/**
 * Default UDP values
 */

static struct udphdr udp_default = {
    .LENGTH   = 0,
    .CHECKSUM = 0
};

/**
 * \brief Retrieve the size of an UDP header 
 * \param udp_header Address of an UDP header or NULL
 * \return The size of an UDP header
 */

size_t udp_get_header_size(const uint8_t * udp_header) {
    return sizeof(struct udphdr);
}

/**
 * \brief Write the default UDP header
 * \param udp_header The address of an allocated buffer that will
 *    store the UDP header or NULL.
 * \return The size of the default header.
 */

size_t udp_write_default_header(uint8_t * udp_header) {
    size_t size = sizeof(struct udphdr);
    if (udp_header) {
        memcpy(udp_header, &udp_default, size);
        ((struct udphdr *) udp_header)->SRC_PORT = htons(UDP_DEFAULT_SRC_PORT);
        ((struct udphdr *) udp_header)->DST_PORT = htons(UDP_DEFAULT_DST_PORT);
    }
    return size;
}

/**
 * \brief Compute and write the checksum related to an UDP header
 * \param udp_header Points to the begining of the UDP header and its content.
 *    The UDP checksum stored in this header is updated by this function.
 * \param ip_psh The IP layer part of the pseudo header. This buffer should
 *    contain the content of an ipv4_pseudo_header_t or an ipv6_pseudo_header_t
 *    structure.
 * \sa http://www.networksorcery.com/enp/protocol/udp.htm#Checksum
 * \return true if everything is fine, false otherwise  
 */

bool udp_write_checksum(uint8_t * udp_header, buffer_t * ip_psh)
{
    struct udphdr * udp_hdr  = (struct udphdr *) udp_header;
    size_t          size_ip  = buffer_get_size(ip_psh),
                    size_udp = ntohs(udp_hdr->LENGTH),
                    size_psh = size_ip + size_udp;
    uint8_t       * psh;

    // UDP checksum computation requires the IPv* header
    if (!ip_psh) {
        errno = EINVAL;
        return false;
    }

    // Allocate the buffer which will contains the pseudo header
    if (!(psh = malloc(size_psh))) {
        return false;
    }

    // Put the excerpt of the IP header into the pseudo header
    memcpy(psh, buffer_get_data(ip_psh), size_ip);

    // Put the UDP header and its content into the pseudo header
    memcpy(psh + size_ip, udp_hdr, size_udp);

    // Overrides the UDP checksum in psh with zeros
    // Checksum debug: http://www4.ncsu.edu/~mlsichit/Teaching/407/Resources/udpChecksum.html
    memset(psh + size_ip + offsetof(struct udphdr, CHECKSUM), 0, sizeof(uint16_t));

    // Compute the checksum
    udp_hdr->check = csum((const uint16_t *) psh, size_psh);
    free(psh);
    return true;
}

buffer_t * udp_create_pseudo_header(const uint8_t * ip_segment)
{
    buffer_t * buffer = NULL;

    // TODO Duplicated from packet.c (see packet_guess_address_family)
    // TODO we should use instanceof
    switch (ip_segment[0] >> 4) {
        case 4:
#ifdef USE_IPV4
            buffer = ipv4_pseudo_header_create(ip_segment);
#endif
            break;
        case 6:
#ifdef USE_IPV6
            buffer = ipv6_pseudo_header_create(ip_segment);
#endif
            break;
        default:
            break;
    }

    return buffer;
}

static protocol_t udp = {
    .name                 = "udp",
    .protocol             = IPPROTO_UDP, 
    .write_checksum       = udp_write_checksum,
    .create_pseudo_header = udp_create_pseudo_header,
    .fields               = udp_fields,
  //.defaults             = udp_defaults,             // XXX used when generic
    .write_default_header = udp_write_default_header, // TODO generic
  //.socket_type          = NULL,
    .get_header_size      = udp_get_header_size,
};

PROTOCOL_REGISTER(udp);
