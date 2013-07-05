#include <stdlib.h>           // malloc()
#include <string.h>           // memcpy()
#include <stdbool.h>          // bool
#include <errno.h>            // ERRNO, EINVAL
#include <stddef.h>           // offsetof()
#include <netinet/ip_icmp.h>
#include <netinet/in.h>       // IPPROTO_ICMP = 1
#include <arpa/inet.h>

#include "../protocol.h"

#define ICMP_FIELD_TYPE             "type"
#define ICMP_FIELD_CODE             "code"
#define ICMP_FIELD_CHECKSUM         "checksum"
#define ICMP_FIELD_BODY             "body"

#define ICMP_DEFAULT_TYPE           ICMP_ECHO // 8
#define ICMP_DEFAULT_CODE           0
#define ICMP_DEFAULT_CHECKSUM       0
#define ICMP_DEFAULT_BODY           0

/**
 * ICMP fields
 */

static protocol_field_t icmpv4_fields[] = {
    {
        .key    = ICMP_FIELD_TYPE,
        .type   = TYPE_UINT8,
        .offset = offsetof(struct icmphdr, type),
    }, {
        .key    = ICMP_FIELD_CODE,
        .type   = TYPE_UINT8,
        .offset = offsetof(struct icmphdr, code),
    }, {
        .key    = ICMP_FIELD_CHECKSUM,
        .type   = TYPE_UINT16,
        .offset = offsetof(struct icmphdr, checksum),
    }, {
        .key    = ICMP_FIELD_BODY,
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
    .type           = ICMP_DEFAULT_TYPE,
    .code           = ICMP_DEFAULT_CODE,
    .checksum       = ICMP_DEFAULT_CHECKSUM,
    .un.gateway     = ICMP_DEFAULT_BODY // XXX union type
};

/**
 * \brief Retrieve the size of an ICMP header 
 * \param icmpv4_header Address of an ICMP header or NULL
 * \return The size of an ICMP header
 */

size_t icmpv4_get_header_size(const uint8_t * icmpv4_header) {
    return sizeof(struct icmphdr);
}

/**
 * \brief Write the default ICMP header
 * \param icmpv4_header The address of an allocated buffer that will
 *    store the ICMPv4 header or NULL.
 * \return The size of the default header.
 */

size_t icmpv4_write_default_header(uint8_t * icmpv4_header) {
    size_t size = sizeof(struct icmphdr);
    if (icmpv4_header) memcpy(icmpv4_header, &icmpv4_default, size);
    return size;
}

/**
 * \brief Compute and write the checksum related to an ICMP header
 * \param icmpv4_header A pre-allocated ICMP header. The ICMP checksum
 *    stored in this buffer is updated by this function.
 * \param ipv4_psh Pass NULL 
 * \sa http://www.networksorcery.com/enp/protocol/icmp.htm#Checksum
 * \return true if everything is ok, false otherwise
 */

bool icmpv4_write_checksum(uint8_t * icmpv4_header, buffer_t * ipv4_psh)
{
    struct icmphdr * icmpv4_hdr = (struct icmphdr *) icmpv4_header;

    // No pseudo header not required in ICMPv4
    if (ipv4_psh) {
        errno = EINVAL;
        return false;
    }

    icmpv4_hdr->checksum = csum((uint16_t *) icmpv4_header, sizeof(struct icmphdr));
    return true;
}

static protocol_t icmp = {
    .name                 = "icmpv4",
    .protocol             = IPPROTO_ICMP,
    .write_checksum       = icmpv4_write_checksum,
    .fields               = icmpv4_fields,
    .write_default_header = icmpv4_write_default_header, // TODO generic
    .get_header_size      = icmpv4_get_header_size,
};

PROTOCOL_REGISTER(icmp);
