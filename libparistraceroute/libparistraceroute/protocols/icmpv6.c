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

#define ICMPV6_DEFAULT_TYPE      ICMP6_ECHO_REQUEST // 128
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
 * \brief Compute and write the checksum related to an ICMPv6 header.
 * \param icmpv6_hdr A pre-allocated ICMPv6 header.
 * \param ipv6_psh The pseudo header.
 * \return true iif successful. 
 */

#include <stdio.h> //////////

bool icmpv6_write_checksum(uint8_t * icmpv6_header, buffer_t * ipv6_psh)
{
    struct icmp6_hdr * icmpv6_hdr = (struct icmp6_hdr *) icmpv6_header;
    size_t             size_ipv6   = buffer_get_size(ipv6_psh),
                       size_icmpv6 = sizeof(struct icmp6_hdr),
                       size_psh    = size_ipv6 + size_icmpv6;
    uint8_t          * psh;


    printf("==============> icmpv6_write_checksum\n");

    // ICMPv6 checksum computation requires the IPv6 pseudoheader
    // http://en.wikipedia.org/wiki/ICMPv6#Message_checksum
    if (!ipv6_psh) {
        errno = EINVAL;
        return false;
    }

    // Allocate the buffer which will contains the pseudo header
    if (!(psh = malloc(size_psh))) {
        return false;
    }

    // Fix len
    ((ipv6_pseudo_header_t *) ipv6_psh->data)->size += htonl(40);

    // Put the excerpt of the IP header into the pseudo header
    memcpy(psh, buffer_get_data(ipv6_psh), size_ipv6);

    {
        printf("Put IPV6 in psh\n");
        buffer_t buffer = {
            .data = psh,
            .size = size_psh
        };
        buffer_dump(&buffer);
        printf("\n----\n");
    }


    // Put the ICMPv6 header and its content into the pseudo header
    memcpy(psh + size_ipv6, icmpv6_hdr, size_icmpv6);

    {
        printf("icmp header\n");
        buffer_t buffer = {
            .data = icmpv6_hdr,
            .size = size_icmpv6 
        };
        buffer_dump(&buffer);
        printf("\n----\n");
    }

    {
        printf("Put ICMPV6 in psh\n");
        buffer_t buffer = {
            .data = psh,
            .size = size_psh
        };
        buffer_dump(&buffer);
        printf("\n----\n");
    }


    // Overrides the ICMPv6 checksum in psh with zeros
    memset(psh + size_ipv6 + offsetof(struct icmp6_hdr, icmp6_cksum), 0, sizeof(uint16_t));

    {
        printf("Reset cksum in psh\n");
        buffer_t buffer = {
            .data = psh,
            .size = size_psh
        };
        buffer_dump(&buffer);
        printf("\n----\n");
    }

    icmpv6_hdr->icmp6_cksum = csum((uint16_t *) icmpv6_header, size_psh);
    printf("Resulting cksum = %x\n", icmpv6_hdr->icmp6_cksum);
    free(psh);
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
