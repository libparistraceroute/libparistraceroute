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
#define ICMP_FIELD_REST_OF_HEADER   "rest_of_header"

#define ICMP_DEFAULT_TYPE           0
#define ICMP_DEFAULT_CODE           0
#define ICMP_DEFAULT_CHECKSUM       0
#define ICMP_DEFAULT_REST_OF_HEADER 0

/**
 * ICMP fields
 */

static protocol_field_t icmp_fields[] = {
    {
        .key = ICMP_FIELD_TYPE,
        .type = TYPE_UINT8,
        .offset = offsetof(struct icmphdr, type),
    }, {
        .key = ICMP_FIELD_CODE,
        .type = TYPE_UINT8,
        .offset = offsetof(struct icmphdr, code),
    }, {
        .key = ICMP_FIELD_CHECKSUM,
        .type = TYPE_UINT16,
        .offset = offsetof(struct icmphdr, checksum),
    }, {
        .key = ICMP_FIELD_REST_OF_HEADER,
        .type = TYPE_UINT16,
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

static struct icmphdr icmp_default = {
    .code           = ICMP_DEFAULT_TYPE,
    .type           = ICMP_DEFAULT_CODE,
    .checksum       = ICMP_DEFAULT_CHECKSUM,
    .un.gateway     = ICMP_DEFAULT_REST_OF_HEADER // XXX union type
};

/**
 * \brief Retrieve the number of fields in a ICMP header
 * \return The number of fields
 */

size_t icmp_get_num_fields(void) {
    return sizeof(icmp_fields) / sizeof(protocol_field_t);
}

/**
 * \brief Retrieve the size of an ICMP header 
 * \param icmpv4_header Address of an ICMP header or NULL
 * \return The size of an ICMP header
 */

size_t icmp_get_header_size(const uint8_t * icmpv4_header) {
    return sizeof(struct icmphdr);
}

/**
 * \brief Write the default ICMP header
 * \param icmpv4_header The address of an allocated buffer that will
 *    store the ICMPv4 header or NULL.
 * \return The size of the default header.
 */

size_t icmp_write_default_header(uint8_t * icmpv4_header) {
    size_t size = sizeof(struct icmphdr);
    if (icmpv4_header) memcpy(icmpv4_header, &icmp_default, size);
    return size;
}

/**
 * \brief Compute and write the checksum related to an ICMP header
 * \param icmp_header A pre-allocated ICMP header. The ICMP checksum
 *    stored in this buffer is updated by this function.
 * \param ip_psh Pass NULL 
 * \sa http://www.networksorcery.com/enp/protocol/icmp.htm#Checksum
 * \return true if everything is ok, false otherwise
 */

bool icmp_write_checksum(uint8_t * icmp_header, buffer_t * ip_psh)
{
    struct icmphdr * icmp_hdr;

    // No pseudo header not required in ICMP
    if (ip_psh) {
        errno = EINVAL;
        return false;
    }

    icmp_hdr = (struct icmphdr *) icmp_header;
    icmp_hdr->checksum = csum((uint16_t *) icmp_header, sizeof(struct icmphdr));
    return true;
}

static protocol_t icmp = {
    .name                 = "icmp",
    .protocol             = IPPROTO_ICMP,
    .write_checksum       = icmp_write_checksum,
    .fields               = icmp_fields,
    .write_default_header = icmp_write_default_header, // TODO generic
    .get_header_size      = icmp_get_header_size,
};

PROTOCOL_REGISTER(icmp);
