#include <stdlib.h>
#include <stdio.h>
#include <string.h>      // memcpy()
#include <stdbool.h>
#include <errno.h>       // ERRNO, EINVAL
#include <stddef.h>      // offsetof()
#include <netinet/ip.h>
#include <netinet/udp.h>
#include <arpa/inet.h>

#include "../protocol.h"

#define UDP_DEFAULT_SRC_PORT 2828
#define UDP_DEFAULT_DST_PORT 2828

#define UDP_FIELD_SRC_PORT "src_port"
#define UDP_FIELD_DST_PORT "dst_port"
#define UDP_FIELD_LENGTH   "length"
#define UDP_FIELD_CHECKSUM "checksum"

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
        .key = UDP_FIELD_SRC_PORT,
        .type = TYPE_INT16,
        .offset = offsetof(struct udphdr, SRC_PORT),
    }, {
        .key = UDP_FIELD_DST_PORT,
        .type = TYPE_INT16,
        .offset = offsetof(struct udphdr, DST_PORT),
    }, {
        .key = UDP_FIELD_LENGTH,
        .type = TYPE_INT16,
        .offset = offsetof(struct udphdr, LENGTH),
    }, {
        .key = UDP_FIELD_CHECKSUM,
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

inline unsigned int udp_get_num_fields(void) {
    return sizeof(udp_fields) / sizeof(protocol_field_t);
}

/**
 * \brief Retrieve the size of an UDP header 
 * \return The size of an UDP header
 */

inline unsigned int udp_get_header_size(void) {
    return sizeof(struct udphdr);
}

/**
 * \brief Write the default UDP header
 * \param data The address of an allocated buffer that will store the header
 */

inline void udp_write_default_header(unsigned char *data) {
    memcpy(data, &udp_default, sizeof(struct udphdr));
}

/**
 * \brief Compute and write the checksum related to an UDP header
 * \param buf A pre-allocated UDP header
 * \param pseudo_hdr The pseudo header 
 * \sa http://www.networksorcery.com/enp/protocol/udp.htm#Checksum
 * \return true if everything is fine, false otherwise  
 */

bool udp_write_checksum(unsigned char *buf, buffer_t * psh)
{
    unsigned char * tmp;
    unsigned int    len;
    unsigned short  res;
    struct udphdr * udp_hdr = (struct udphdr *) buf;

    if (!psh) { // pseudo header required
        errno = EINVAL;
        return false;
    }

    //len = sizeof(struct udphdr) + buffer_get_size(psh) +4;
    len = ntohs(udp_hdr->LENGTH) + buffer_get_size(psh);
    tmp = malloc(len * sizeof(unsigned char));
    if (!tmp) { // not enough memory
        errno = ENOMEM;
        return false;
    }

    /* XXX shall we replace this by buffer operations */
    memcpy(tmp, buffer_get_data(psh), buffer_get_size(psh));

    //memcpy(tmp + buffer_get_size(psh), udp_hdr, sizeof(struct udphdr)+4);
    memcpy(tmp + buffer_get_size(psh), udp_hdr, ntohs(udp_hdr->LENGTH));//sizeof(struct udphdr));
    res = csum(*(unsigned short**) &tmp, (len >> 1));
    udp_hdr->check = res;

    free(tmp);
    return true;
}

#define HD_UDP_DADDR 4
#define HD_UDP_PAD 8
#define HD_UDP_PROT 9
#define HD_UDP_LEN 10

buffer_t * udp_create_psh_ipv4(unsigned char* ipv4_buffer)
//unsigned char** pseudo_header, int* size)
{
    buffer_t *psh;
    struct iphdr *iph;
    unsigned char *data;
    
    psh = buffer_create();
    buffer_resize(psh, 12);

    data = buffer_get_data(psh);

    iph = (struct iphdr*) ipv4_buffer;
    *(( u_int32_t *)  data                ) = (u_int32_t) iph->saddr;
    *(( u_int32_t *) (data + HD_UDP_DADDR)) = (u_int32_t) iph->daddr;
    *(( u_int8_t  *) (data + HD_UDP_PAD  )) =             0;
    *(( u_int8_t  *) (data + HD_UDP_PROT )) = (u_int8_t)  iph->protocol;
    *(( u_int16_t *) (data + HD_UDP_LEN  )) = (u_int16_t) htons(ntohs(iph->tot_len)-4*iph->ihl);

    return psh;
}

buffer_t * udp_create_psh(unsigned char * buffer)
{
    // http://www.networksorcery.com/enp/protocol/udp.htm#Checksum
    return udp_create_psh_ipv4(buffer);
}

static protocol_t udp = {
    .name                 = "udp",
    .protocol             = SOL_UDP, // 17
    .get_num_fields       = udp_get_num_fields,
    .write_checksum       = udp_write_checksum,
    .create_pseudo_header = udp_create_psh,
    .fields               = udp_fields,
  //.defaults             = udp_defaults,             // XXX used when generic
    .header_len           = sizeof(struct udphdr),
    .write_default_header = udp_write_default_header, // TODO generic
  //.socket_type          = NULL,
    .get_header_size      = udp_get_header_size,
    .need_ext_checksum    = true
};

PROTOCOL_REGISTER(udp);
