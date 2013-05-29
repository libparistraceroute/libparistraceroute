#include <stdlib.h>        // malloc()
#include <string.h>        // memcpy()
#include <stdbool.h>       // bool
#include <errno.h>         // ERRNO, EINVAL
#include <stddef.h>        // offsetof()
#include <netinet/icmp6.h> // icmp6_hdr
#include <netinet/ip6.h>   // ip6_hdr
#include <arpa/inet.h>

#include "../protocol.h"

#define ICMP6_FIELD_TYPE        "type"
#define ICMP6_FIELD_CODE        "code"
#define ICMP6_FIELD_CHECKSUM    "checksum"
#define ICMP6_FIELD_BODY        "body"

#define ICMP6_DEFAULT_TYPE      0
#define ICMP6_DEFAULT_CODE      0
#define ICMP6_DEFAULT_CHECKSUM  0
#define ICMP6_DEFAULT_BODY      0

// Compat
#define ICMP6_FIELD_PROTOCOL    "protocol"

// implem TODO to remove see ipv6_pseudo_header_t
#define HD_ICMP6_DADDR 16
#define HD_ICMP6_LEN 32
#define HD_ICMP6_PAD 36
#define HD_ICMP6_NXH 39

/* ICMP fields */
static protocol_field_t icmp6_fields[] = {
    {   
        .key = ICMP6_FIELD_TYPE,
        .type = TYPE_INT8,
        .offset = offsetof(struct icmp6_hdr, icmp6_type),
    }, {
        .key = ICMP6_FIELD_CODE,
        .type = TYPE_INT8,
        .offset = offsetof(struct icmp6_hdr, icmp6_code),
    }, {
        .key = ICMP6_FIELD_CHECKSUM,
        .type = TYPE_INT16,
        .offset = offsetof(struct icmp6_hdr, icmp6_cksum),
    }, {
        .key = ICMP6_FIELD_BODY,
        .type = TYPE_INT32,
        .offset = offsetof(struct icmp6_hdr, icmp6_dataun), // XXX union type
    },
    // TODO Multiple possibilities for the last field ! 
    // e.g. "protocol" when we repeat some packet header for example
    END_PROTOCOL_FIELDS
};

/* Default ICMP values */
static struct icmp6_hdr icmp6_default = {
    .icmp6_code   = ICMP6_DEFAULT_TYPE,
    .icmp6_type   = ICMP6_DEFAULT_CODE,
    .icmp6_cksum  = ICMP6_DEFAULT_CHECKSUM,
    .icmp6_dataun = ICMP6_DEFAULT_BODY // XXX union type
};

/**
 * \brief Retrieve the number of fields in a ICMP header
 * \return The number of fields
 * TODO does this make sense for ICMPv6?
 */

size_t icmp6_get_num_fields(void) {
    return sizeof(icmp6_fields) / sizeof(protocol_field_t);
}

/**
 * \brief Retrieve the size of an ICMP6 header
 * \return The size of an ICMP6 header
 */

size_t icmp6_get_header_size(void) {
    return sizeof(struct icmp6_hdr);
}

/**
 * \brief Write the default ICMP6 header
 * \param data The address of an allocated buffer that will store the header
 */

void icmp6_write_default_header(unsigned char *data) {
    memcpy(data, &icmp6_default, sizeof(struct icmp6_hdr));
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

bool icmp6_write_checksum(uint8_t * buf, buffer_t * psh)
{
    uint8_t * tmp;
    size_t    len;
    uint16_t  res;
    struct icmp6_hdr * icmp6_hed = (struct icmp6_hdr *) buf;

    if (!psh) { // pseudo header required
        errno = EINVAL;
        return false;
    }

    len = buffer_get_size(buf) + buffer_get_size(psh);
    if (!(tmp = malloc(len))) { // not enough memory
        return false;
    }

    memcpy(tmp, buffer_get_data(psh), buffer_get_size(psh));
    memcpy(tmp + buffer_get_size(psh), icmp6_hed, buffer_get_size(buf));
    res = csum(*(uint16_t **) &tmp, (len >> 1));
    icmp6_hed->icmp6_cksum = res;

    free(tmp);
    return true;
}


// Pseudoheaders should maybe become generic
// Definitions for Pseudoheader

buffer_t * icmp6_create_psh_ipv6(uint8_t * ipv6_buffer)
{
    /* http://tools.ietf.org/html/rfc2460
       +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
       |                                                               |
       +                                                               +
       |                                                               |
       +                         Source Address                        +
       |                                                               |
       +                                                               +
       |                                                               |
       +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
       |                                                               |
       +                                                               +
       |                                                               |
       +                      Destination Address                      +
       |                                                               |
       +                                                               +
       |                                                               |
       +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
       |                   Upper-Layer Packet Length                   |
       +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
       |                      zero                     |  Next Header  |
       +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
       */
    buffer_t       * psh;
    struct ip6_hdr * iph;
    uint8_t        * data;

    psh = buffer_create();
    buffer_resize(psh, 40);
    data = buffer_get_data(psh);

    iph = (struct ip6_hdr *) ipv6_buffer;
    *((uint32_t *) (data))                       =  iph->ip6_src.__in6_u.__u6_addr32[0]; // src_ip
    *((uint32_t *) (data + 4           ))        =  iph->ip6_src.__in6_u.__u6_addr32[1]; // src_ip
    *((uint32_t *) (data + 8           ))        =  iph->ip6_src.__in6_u.__u6_addr32[2]; // src_ip
    *((uint32_t *) (data + 12          ))        =  iph->ip6_src.__in6_u.__u6_addr32[3]; // src_ip
    *((uint32_t *) (data + HD_ICMP6_DADDR   ))   =  iph->ip6_dst.__in6_u.__u6_addr32[0]; // dst_ip
    *((uint32_t *) (data + HD_ICMP6_DADDR + 4 )) =  iph->ip6_dst.__in6_u.__u6_addr32[1]; // dst_ip
    *((uint32_t *) (data + HD_ICMP6_DADDR + 8 )) =  iph->ip6_dst.__in6_u.__u6_addr32[2]; // dst_ip
    *((uint32_t *) (data + HD_ICMP6_DADDR + 12)) =  iph->ip6_dst.__in6_u.__u6_addr32[3]; // dst_ip
    *((uint32_t *) (data + HD_ICMP6_LEN))        = (u_int32_t)  iph->ip6_ctlun.ip6_un1.ip6_un1_plen; // XXX ext headers need to be substracted
    *((uint32_t *) (data + HD_ICMP6_PAD))        =  0x00000000; // 3 Bytes zero (3 stay zero, one gets overwritten)
    *((uint8_t  *) (data + HD_ICMP6_NXH))        = 58;  // ICMP6 takes last byte

    return psh;
}

static protocol_t icmp6 = {
    .name                 = "icmp6",
    .protocol             = 1, // DO not change ... BREAKS paristraceroute
    .get_num_fields       = icmp6_get_num_fields,
    .write_checksum       = icmp6_write_checksum,
    .create_pseudo_header = icmp6_create_psh_ipv6,
    .fields               = icmp6_fields,
    .header_len           = sizeof(struct icmp6_hdr),
    .write_default_header = icmp6_write_default_header, // TODO generic
    .get_header_size      = icmp6_get_header_size,
    .need_ext_checksum    = true, 
    .finalize             = 1
};

PROTOCOL_REGISTER(icmp6);
