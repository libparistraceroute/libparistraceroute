#include <stdio.h>
#include <stdlib.h>        // malloc()
#include <string.h>        // memcpy()
#include <stdbool.h>       // bool
#include <errno.h>         // ERRNO, EINVAL
#include <stddef.h>        // offsetof()
#include <netinet/icmp6.h> // icmp6_hdr
#include <netinet/ip6.h>   // ip6_hdr
#include <netinet/in.h>    // IPPROTO_ICMPV6
#include <arpa/inet.h>

#include "../probe.h"
#include "../protocol.h"
#include "../layer.h"
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
    .icmp6_data32 = { ICMPV6_DEFAULT_BODY }
};

/**
 * \brief Retrieve the size of an ICMP6 header
 * \param icmpv6_segment (unused) Address of an ICMPv6 header or NULL.
 * \return The size of an ICMP6 header, 0 if icmpv6_segment is NULL.
 */

size_t icmpv6_get_header_size(const uint8_t * icmpv6_segment) {
    return icmpv6_segment ? sizeof(struct icmp6_hdr) : 0;
}

/**
 * \brief Write the default ICMP6 header
 * \param icmpv6_segment The address of an allocated buffer that will
 *    store the ICMPv6 header or NULL.
 * \return The size of the default header.
 */

size_t icmpv6_write_default_header(uint8_t * icmpv6_segment) {
    size_t size = sizeof(struct icmp6_hdr);
    if (icmpv6_segment) memcpy(icmpv6_segment, &icmpv6_default, size);
    return size;
}

/**
 * \brief Compute and write the checksum related to an ICMPv6 header.
 * \param icmpv6_header A pre-allocated ICMPv6 header.
 * \param ipv6_psh The pseudo header.
 * \return true iif successful.
 */

bool icmpv6_write_checksum(uint8_t * icmpv6_segment, buffer_t * ipv6_psh)
{
    struct icmp6_hdr * icmpv6_header = (struct icmp6_hdr *) icmpv6_segment;
    size_t             size_ipv6     = buffer_get_size(ipv6_psh),
                       size_icmpv6   = sizeof(struct icmp6_hdr),
                       size_psh      = size_ipv6 + size_icmpv6;
    uint8_t          * psh;

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

    // Put the excerpt of the IP header into the pseudo header
    memcpy(psh, buffer_get_data(ipv6_psh), size_ipv6);

    // Put the ICMPv6 header and its content into the pseudo header
    memcpy(psh + size_ipv6, icmpv6_header, size_icmpv6);

    // Overrides the ICMPv6 checksum in psh with zeros
    memset(psh + size_ipv6 + offsetof(struct icmp6_hdr, icmp6_cksum), 0, sizeof(uint16_t));

    icmpv6_header->icmp6_cksum = csum((uint16_t *) psh, size_psh);
    free(psh);
    return true;
}

const protocol_t * icmpv6_get_next_protocol(const layer_t * icmpv6_layer) {
    const protocol_t * next_protocol = NULL;
    uint8_t            icmpv6_type;

    if (layer_extract(icmpv6_layer, "type", &icmpv6_type)) {
        switch (icmpv6_type) {
            case ICMP6_DST_UNREACH:
            case ICMP6_TIME_EXCEEDED:
                next_protocol = protocol_search("ipv6");
                break;
            default:
                break;
        }
    }
    return next_protocol;
}

/**
 * \brief check whether the icmpv6 protocols of 2 probes match
 * \param _probe the probe to analyse
 * \param _reply the reply to the probe to analyse
 * \true if protocols match, false otherwise
 */

bool icmpv6_matches(const struct probe_s * _probe, const struct probe_s * _reply)
{
    uint8_t         reply_type = 0,
                    reply_code = 0,
                    probe_type = 0,
                    probe_code = 0;
    layer_t       * icmp_layer = NULL;
    const probe_t * probe      = (const probe_t *) _probe;
    const probe_t * reply      = (const probe_t *) _reply;

    if (probe_extract(reply, "type", &reply_type)
     && probe_extract(probe, "type", &probe_type)
     && probe_extract(probe, "code", &probe_code)) {

        if (reply_type == ICMP6_ECHO_REPLY) {
            return true;
        }

        if (!(icmp_layer = probe_get_layer(reply, 3)) || strcmp(icmp_layer->protocol->name, "icmpv4")) {
            return false;
        }

        if (probe_extract_ext(reply, "type", 3, &reply_type) && probe_extract_ext(reply, "code", 3, &reply_code)) {
            return (probe_type == reply_type) && (probe_code == reply_code);
        }
    }
    return false;
}

static protocol_t icmpv6 = {
    .name                 = "icmpv6",
    .protocol             = IPPROTO_ICMPV6,
    .write_checksum       = icmpv6_write_checksum,
    .create_pseudo_header = ipv6_pseudo_header_create,
    .fields               = icmpv6_fields,
    .write_default_header = icmpv6_write_default_header, // TODO generic memcpy + header size
    .get_header_size      = icmpv6_get_header_size,
    .get_next_protocol    = icmpv6_get_next_protocol,
    .matches              = icmpv6_matches,
};

PROTOCOL_REGISTER(icmpv6);
