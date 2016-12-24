#include "../use.h"       // USE_BITS

#ifdef USE_IPV4
#include <stdio.h>
#include <stdlib.h>                 // malloc()
#include <string.h>                 // memcpy()
#include <stdbool.h>                // bool
#include <errno.h>                  // ERRNO, EINVAL
#include <stddef.h>                 // offsetof()
#include "os/netinet/ip_icmp.h"     // ICMP_ECHO
#include <netinet/in.h>             // IPPROTO_ICMP = 1
#include <arpa/inet.h>

#include "../protocol.h"
#include "../layer.h"
#include "../probe.h"

#define ICMPV4_FIELD_TYPE             "type"
#define ICMPV4_FIELD_CODE             "code"
#define ICMPV4_FIELD_CHECKSUM         "checksum"
#define ICMPV4_FIELD_BODY             "body"

#define ICMPV4_DEFAULT_TYPE           ICMP_ECHO // 8
#define ICMPV4_DEFAULT_CODE           0
#define ICMPV4_DEFAULT_CHECKSUM       0
#define ICMPV4_DEFAULT_BODY           0

/**
 * ICMP fields
 */

static protocol_field_t icmpv4_fields[] = {
    {
        .key    = ICMPV4_FIELD_TYPE,
        .type   = TYPE_UINT8,
        .offset = offsetof(struct icmphdr, ICMPV4_TYPE),
    }, {
        .key    = ICMPV4_FIELD_CODE,
        .type   = TYPE_UINT8,
        .offset = offsetof(struct icmphdr, ICMPV4_CODE),
    }, {
        .key    = ICMPV4_FIELD_CHECKSUM,
        .type   = TYPE_UINT16,
        .offset = offsetof(struct icmphdr, ICMPV4_CHECKSUM),
    },
#ifdef LINUX
    {
        .key    = ICMPV4_FIELD_BODY,
        .type   = TYPE_UINT16,
        .offset = offsetof(struct icmphdr, un), // XXX union type
        // optional = 0
    },
#endif
    // TODO Multiple possibilities for the last field !
    // e.g. "protocol" when we repeat some packet header for example
    END_PROTOCOL_FIELDS
};

/**
 * Default ICMP values
 */

static struct icmphdr icmpv4_default = {
    /*
    .type           = ICMPV4_DEFAULT_TYPE,
    .code           = ICMPV4_DEFAULT_CODE,
    .checksum       = ICMPV4_DEFAULT_CHECKSUM,
    .un.gateway     = ICMPV4_DEFAULT_BODY // XXX union type
    */
    .ICMPV4_TYPE        = ICMPV4_DEFAULT_TYPE,
    .ICMPV4_CODE        = ICMPV4_DEFAULT_CODE,
    .ICMPV4_CHECKSUM    = ICMPV4_DEFAULT_CHECKSUM,
#ifdef LINUX
    .un.gateway     = ICMPV4_DEFAULT_BODY // XXX union type
#endif
};

/**
 * \brief Retrieve the size of an ICMP header
 * \param icmpv4_segment Address of an ICMP header or NULL.
 * \return The size of an ICMP header, 0 if icmpv4_segment is NULL.
 */

size_t icmpv4_get_header_size(const uint8_t * icmpv4_segment) {
    return icmpv4_segment ? sizeof(struct icmphdr) : 0;
}

/**
 * \brief Write the default ICMP header
 * \param icmpv4_segment The address of an allocated buffer that will
 *    store the ICMPv4 header or NULL.
 * \return The size of the default header.
 */

size_t icmpv4_write_default_header(uint8_t * icmpv4_segment) {
    size_t size = sizeof(struct icmphdr);
    if (icmpv4_segment) memcpy(icmpv4_segment, &icmpv4_default, size);
    return size;
}

/**
 * \brief Compute and write the checksum related to an ICMP header
 * \param icmpv4_segment A pre-allocated ICMP header. The ICMP checksum
 *    stored in this buffer is updated by this function.
 * \param ipv4_psh Pass NULL
 * \sa http://www.networksorcery.com/enp/protocol/icmp.htm#Checksum
 * \return true if everything is ok, false otherwise
 */

bool icmpv4_write_checksum(uint8_t * icmpv4_segment, buffer_t * ipv4_psh)
{
    struct icmphdr * icmpv4_header = (struct icmphdr *) icmpv4_segment;

    // No pseudo header not required in ICMPv4
    if (ipv4_psh) {
        errno = EINVAL;
        return false;
    }

    // The ICMPv4 checksum must be set to 0 before its calculation
    icmpv4_header->ICMPV4_CHECKSUM = 0;
    icmpv4_header->ICMPV4_CHECKSUM = csum((uint16_t *) icmpv4_segment, sizeof(struct icmphdr));
    return true;
}

const protocol_t * icmpv4_get_next_protocol(const layer_t * icmpv4_layer) {
    const protocol_t * next_protocol = NULL;
    uint8_t            icmpv4_type;

    if (layer_extract(icmpv4_layer, "type", &icmpv4_type)) {
        switch (icmpv4_type) {
            case ICMP_DEST_UNREACH:
            case ICMP_TIME_EXCEEDED:
                next_protocol = protocol_search("ipv4");
                break;
            default:
                break;
        }
    }
    return next_protocol;
}

/**
 * \brief check whether the icmpv4 protocols of 2 probes match
 * \param _probe the probe to analyse
 * \param _reply the reply to the probe to analyse
 * \true if protocols match, false otherwise
 */

bool icmpv4_matches(const struct probe_s * _probe, const struct probe_s * _reply)
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

        if (reply_type == ICMP_ECHOREPLY) {
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




static protocol_t icmpv4 = {
    .name                 = "icmpv4",
    .protocol             = IPPROTO_ICMP,
    .write_checksum       = icmpv4_write_checksum,
    .fields               = icmpv4_fields,
    .write_default_header = icmpv4_write_default_header, // TODO generic
    .get_header_size      = icmpv4_get_header_size,
    .get_next_protocol    = icmpv4_get_next_protocol,
    .matches              = icmpv4_matches,
};

PROTOCOL_REGISTER(icmpv4);
#endif
