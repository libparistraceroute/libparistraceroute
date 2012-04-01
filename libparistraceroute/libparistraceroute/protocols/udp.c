#include <stdlib.h>
#include <stdio.h>
#include <string.h>      // memcpy()
#include <stdbool.h>
#include <errno.h>       // ERRNO, EINVAL
#include <stddef.h>      // offsetof()
#include <netinet/udp.h>
#include <arpa/inet.h>

#include "../protocol.h"

#define UDP_DEFAULT_SRC_PORT 2828
#define UDP_DEFAULT_DST_PORT 2828

// XXX mandatory fields ?
// XXX UDP parsing missing
#ifdef __FAVOR_BSD
#   define SRC_PORT uh_sport
#   define DST_PORT uh_dport
#   define LENGTH   uh_ulen
#   define CHECKSUM uh_sum
#else
#   define SRC_PORT source 
#   define DST_PORT dest 
#   define LENGTH   len
#   define CHECKSUM check 
#endif

/* UDP fields */
static protocol_field_t udp_fields[] = {
    {
        .key = "src_port",
        .type = TYPE_INT16,
        .offset = offsetof(struct udphdr, SRC_PORT),
    }, {
        .key = "dst_port",
        .type = TYPE_INT16,
        .offset = offsetof(struct udphdr, DST_PORT),
    }, {
        .key = "length",
        .type = TYPE_INT16,
        .offset = offsetof(struct udphdr, LENGTH),
    }, {
        .key = "checksum",
        .type = TYPE_INT16,
        .offset = offsetof(struct udphdr, CHECKSUM),
        // optional = 0
    },
    END_PROTOCOL_FIELDS
};

/* Default UDP values */
static struct udphdr udp_default = {
    .SRC_PORT = UDP_DEFAULT_SRC_PORT,
    .DST_PORT = UDP_DEFAULT_DST_PORT,
    .LENGTH   = 0,
    .CHECKSUM = 0
};

/**
 * \brief Retrieve the number of fields in a UDP header
 * \return The number of fields
 */

unsigned int udp_get_num_fields(void)
{
    return sizeof(udp_fields) / sizeof(protocol_field_t);
}

/**
 * \brief Retrieve the size of an UDP header 
 * \return The size of an UDP header
 */

unsigned int udp_get_header_size(void)
{
    return sizeof(struct udphdr);
}

/**
 * \brief Write the default UDP header
 * \param data The address of an allocated buffer that will store the header
 */

void udp_write_default_header(unsigned char *data)
{
    memcpy(data, &udp_default, sizeof(struct udphdr));
}

/**
 * \brief Compute and write the checksum related to an UDP header
 * \param udp_hdr A pre-allocated UDP header
 * \param pseudo_hdr The pseudo header 
 * \sa http://www.networksorcery.com/enp/protocol/udp.htm#Checksum
 * \return 0 if everything is ok, EINVAL if pseudo_hdr is invalid,
 *    ENOMEM if a memory error arises
 */

int udp_write_checksum(unsigned char *buf, pseudoheader_t * psh)
{
    unsigned char * tmp;
    unsigned int len;
    unsigned short res;
    struct udphdr *udp_hdr = (struct udphdr *)buf;
    if (!psh) return EINVAL; // pseudo header required

    len = sizeof(struct udphdr) + psh->size;
    tmp = malloc(len * sizeof(unsigned char));
    if(!tmp) return ENOMEM;

    memcpy(tmp, psh->data, psh->size);
    memcpy(tmp + psh->size, udp_hdr, sizeof(struct udphdr));
    res = csum(*(unsigned short**) &tmp, (len >> 1));
    udp_hdr->check = res;

    free(tmp);
    return 0;
}

static protocol_t udp = {
    .name                 = "udp",
    .protocol             = SOL_UDP, // 17
    .get_num_fields       = udp_get_num_fields,
    .write_checksum       = udp_write_checksum,
  //.create_pseudo_header = NULL,
    .fields               = udp_fields,
  //.defaults             = udp_defaults,             // XXX used when generic
    .header_len           = sizeof(struct udphdr),
    .write_default_header = udp_write_default_header, // TODO generic
  //.socket_type          = NULL,
    .get_header_size      = udp_get_header_size,
    .need_ext_checksum    = true
};

PROTOCOL_REGISTER(udp);
