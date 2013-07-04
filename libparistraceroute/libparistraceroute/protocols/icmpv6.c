#include <stdlib.h>        // malloc()
#include <string.h>        // memcpy()
#include <stdbool.h>       // bool
#include <errno.h>         // ERRNO, EINVAL
#include <stddef.h>        // offsetof()
#include <netinet/icmp6.h> // icmp6_hdr
#include <netinet/ip6.h>   // ip6_hdr
#include <netinet/in.h>    // IPPROTO_ICMPV6 
#include <arpa/inet.h>

#include "../protocol.h"
#include "ipv6_pseudo_header.h"

#define ICMPV6_FIELD_TYPE        "type"
#define ICMPV6_FIELD_CODE        "code"
#define ICMPV6_FIELD_CHECKSUM    "checksum"
#define ICMPV6_FIELD_BODY        "body"

#define ICMPV6_DEFAULT_TYPE      0
#define ICMPV6_DEFAULT_CODE      0
#define ICMPV6_DEFAULT_CHECKSUM  0
#define ICMPV6_DEFAULT_BODY      0

// ICMP fields
static protocol_field_t icmpv6_fields[] = {
    {   
        .key    = ICMPV6_FIELD_TYPE,
        .type   = TYPE_UINT8,
        .offset = offsetof(struct icmp6_hdr, icmp6_type),
    }, {
        .key    = ICMPV6_FIELD_CODE,
        .type   = TYPE_UINT8,
        .offset = offsetof(struct icmp6_hdr, icmp6_code),
    }, {
        .key    = ICMPV6_FIELD_CHECKSUM,
        .type   = TYPE_UINT16,
        .offset = offsetof(struct icmp6_hdr, icmp6_cksum),
    }, {
        .key    = ICMPV6_FIELD_BODY,
        .type   = TYPE_UINT32,
        .offset = offsetof(struct icmp6_hdr, icmp6_dataun), // XXX union type
    },
    // TODO Multiple possibilities for the last field ! 
    // e.g. "protocol" when we repeat some packet header for example
    END_PROTOCOL_FIELDS
};

/* Default ICMP values */
static struct icmp6_hdr icmpv6_default = {
    .icmp6_type   = ICMPV6_DEFAULT_TYPE,
    .icmp6_code   = ICMPV6_DEFAULT_CODE,
    .icmp6_cksum  = ICMPV6_DEFAULT_CHECKSUM,
    .icmp6_dataun = ICMPV6_DEFAULT_BODY // XXX union type
};

/**
 * \brief Retrieve the size of an ICMP6 header
 * \param icmpv6_header (unused) Address of an ICMPv6 header or NULL.
 * \return The size of an ICMP6 header
 */

size_t icmpv6_get_header_size(const uint8_t * icmpv6_header) {
    return sizeof(struct icmp6_hdr);
}

/**
 * \brief Write the default ICMP6 header
 * \param icmpv6_header The address of an allocated buffer that will
 *    store the ICMPv6 header or NULL.
 * \return The size of the default header.
 */

size_t icmpv6_write_default_header(uint8_t * icmpv6_header) {
    size_t size = sizeof(struct icmp6_hdr);
    if (icmpv6_header) memcpy(icmpv6_header, &icmpv6_default, size);
    return size; 
}

/**
 * \brief Compute and write the checksum related to an ICMPv6 header
 * \param icmp_hdr A pre-allocated ICMPv6 header
 * \param pseudo_hdr The pseudo header 
 * \sa http://www.networksorcery.com/enp/protocol/icmp.htm#Checksum
 * \return 0 if everything is ok, EINVAL if pseudo_hdr is invalid,
 *    ENOMEM if a memory error arises
 */

// XXX http://en.wikipedia.org/wiki/ICMPv6#Message_checksum

bool icmpv6_write_checksum(uint8_t * icmp_header, buffer_t * ip_psh)
{
    // TODO reimplement this function like udp_write_checksum

    uint8_t * tmp;
    size_t    len;
    struct    icmp6_hdr * icmpv6_hed = (struct icmp6_hdr *) icmp_header;

    if (!ip_psh) { // pseudo header required
        errno = EINVAL;
        return false;
    }

    // TODO use csum
    len = sizeof(struct icmp6_hdr) + buffer_get_size(ip_psh);
    if (!(tmp = malloc(len))) { // not enough memory
        return false;
    }

    memcpy(tmp, buffer_get_data(ip_psh), buffer_get_size(ip_psh));
    memcpy(tmp + buffer_get_size(ip_psh), icmpv6_hed, sizeof(struct icmp6_hdr));
    icmpv6_hed->icmp6_cksum = csum(*(uint16_t **) &tmp, (len >> 1));

    free(tmp);
    return true;
}

static protocol_t icmpv6 = {
    .name                 = "icmpv6",
    .protocol             = IPPROTO_ICMPV6,
    .write_checksum       = icmpv6_write_checksum,
    .create_pseudo_header = ipv6_pseudo_header_create,
    .fields               = icmpv6_fields,
    .write_default_header = icmpv6_write_default_header, // TODO generic memcpy + header size
    .get_header_size      = icmpv6_get_header_size,
};

PROTOCOL_REGISTER(icmpv6);
