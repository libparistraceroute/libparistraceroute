#include <stdlib.h>
#include <stdio.h>
#include <string.h>      // memcpy()
#include <stdbool.h>
#include <errno.h>       // ERRNO, EINVAL
#include <stddef.h>      // offsetof()
#include <netinet/ip_icmp.h>
#include <arpa/inet.h>

#include "../protocol.h"

#define ICMP_FIELD_TYPE           "type"
#define ICMP_FIELD_CODE           "code"
#define ICMP_FIELD_CHECKSUM       "checksum"
#define ICMP_FIELD_REST_OF_HEADER "rest_of_header"

#define ICMP_DEFAULT_TYPE           0
#define ICMP_DEFAULT_CODE           0
#define ICMP_DEFAULT_CHECKSUM       0
#define ICMP_DEFAULT_REST_OF_HEADER 0

/* ICMP fields */
static protocol_field_t icmp_fields[] = {
    {
        .key = ICMP_FIELD_TYPE,
        .type = TYPE_INT8,
        .offset = offsetof(struct icmphdr, type),
    }, {
        .key = ICMP_FIELD_CODE,
        .type = TYPE_INT8,
        .offset = offsetof(struct icmphdr, code),
    }, {
        .key = ICMP_FIELD_CHECKSUM,
        .type = TYPE_INT16,
        .offset = offsetof(struct icmphdr, checksum),
    }, {
        .key = ICMP_FIELD_REST_OF_HEADER,
        .type = TYPE_INT16,
        .offset = offsetof(struct icmphdr, un), // XXX union type 
        // optional = 0
    },
    // TODO Multiple possibilities for the last field ! 
    // e.g. "protocol" when we repeat some packet header for example
    END_PROTOCOL_FIELDS
};

/* Default ICMP values */
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

unsigned int icmp_get_num_fields(void)
{
    return sizeof(icmp_fields) / sizeof(protocol_field_t);
}

/**
 * \brief Retrieve the size of an ICMP header 
 * \return The size of an ICMP header
 */

unsigned int icmp_get_header_size(void)
{
    return sizeof(struct icmphdr);
}

/**
 * \brief Write the default ICMP header
 * \param data The address of an allocated buffer that will store the header
 */

void icmp_write_default_header(unsigned char *data)
{
    memcpy(data, &icmp_default, sizeof(struct icmphdr));
}

/**
 * \brief Compute and write the checksum related to an ICMP header
 * \param icmp_hdr A pre-allocated ICMP header
 * \param pseudo_hdr The pseudo header 
 * \sa http://www.networksorcery.com/enp/protocol/icmp.htm#Checksum
 * \return 0 if everything is ok, EINVAL if pseudo_hdr is invalid,
 *    ENOMEM if a memory error arises
 */

int icmp_write_checksum(unsigned char *buf, buffer_t * psh)
{
    struct icmphdr *icmp_hed;
    unsigned short len;

    if (psh) return EINVAL; // pseudo header not required

    icmp_hed = (struct icmphdr *)buf;
    len = sizeof(struct icmphdr);

    icmp_hed->checksum = csum(*(unsigned short**)buf, len >> 1);

    return 0;

}

static protocol_t icmp = {
    .name                 = "icmp",
    .protocol             = 1,
    .get_num_fields       = icmp_get_num_fields,
    .write_checksum       = icmp_write_checksum,
    .fields               = icmp_fields,
    .header_len           = sizeof(struct icmphdr),
    .write_default_header = icmp_write_default_header, // TODO generic
    .get_header_size      = icmp_get_header_size,
    .need_ext_checksum    = true
};

PROTOCOL_REGISTER(icmp);
