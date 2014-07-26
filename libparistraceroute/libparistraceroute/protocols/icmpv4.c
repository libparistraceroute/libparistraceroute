#include <stdio.h>
#include <stdlib.h>           // malloc()
#include <string.h>           // memcpy()
#include <stdbool.h>          // bool
#include <errno.h>            // ERRNO, EINVAL
#include <stddef.h>           // offsetof()
#include <netinet/ip_icmp.h>  // ICMP_ECHO
#include <netinet/in.h>       // IPPROTO_ICMP = 1
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
        .offset = offsetof(struct icmphdr, type),
    }, {
        .key    = ICMPV4_FIELD_CODE,
        .type   = TYPE_UINT8,
        .offset = offsetof(struct icmphdr, code),
    }, {
        .key    = ICMPV4_FIELD_CHECKSUM,
        .type   = TYPE_UINT16,
        .offset = offsetof(struct icmphdr, checksum),
    }, {
        .key    = ICMPV4_FIELD_BODY,
        .type   = TYPE_UINT16,
        .offset = offsetof(struct icmphdr, un), // XXX union type 
        // optional = 0
    },
    // TODO Multiple possibilities for the last field ! 
    // e.g. "protocol" when we repeat some packet header for example
    END_PROTOCOL_FIELDS
};

/**
 * Default ICMP values
 */

static struct icmphdr icmpv4_default = {
    .type           = ICMPV4_DEFAULT_TYPE,
    .code           = ICMPV4_DEFAULT_CODE,
    .checksum       = ICMPV4_DEFAULT_CHECKSUM,
    .un.gateway     = ICMPV4_DEFAULT_BODY // XXX union type
};

/**
 * \brief Retrieve the size of an ICMP header 
 * \param icmpv4_segment Address of an ICMP header or NULL
 * \return The size of an ICMP header
 */

size_t icmpv4_get_header_size(const uint8_t * icmpv4_segment) {
    return sizeof(struct icmphdr);
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
    icmpv4_header->checksum = 0;
    icmpv4_header->checksum = csum((uint16_t *) icmpv4_segment, sizeof(struct icmphdr));
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

bool icmpv4_matches(const struct probe_s * probe, const struct probe_s * reply)
{
    uint8_t type = 0;
    int result = 0;
    if ((probe_extract((const probe_t *)reply, "type", &type))) {
       result = (type == ICMP_ECHOREPLY);
    }
    printf("icmpv4 = %d\n", result);
    return (bool)result;
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
