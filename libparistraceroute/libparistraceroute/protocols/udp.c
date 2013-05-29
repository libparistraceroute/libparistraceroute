#include <stdlib.h>           // malloc()
#include <string.h>           // memcpy()
#include <stdbool.h>          // bool
#include <errno.h>            // ERRNO, EINVAL
#include <stddef.h>           // offsetof()
#include <netinet/ip.h>       // ip_hdr
#include <netinet/ip6.h>      // ip6_hdr
#include <netinet/udp.h>      // udphdr
#include <netinet/in.h>       // IPPROTO_UDP = 17
#include <arpa/inet.h>
#include <stdio.h>

#include "../protocol.h"      // csum
#include "buffer.h"           // buffer_t
#include "field.h"            // uint128_t

#define UDP_DEFAULT_SRC_PORT 2828
#define UDP_DEFAULT_DST_PORT 2828

#define UDP_FIELD_SRC_PORT   "src_port"
#define UDP_FIELD_DST_PORT   "dst_port"
#define UDP_FIELD_LENGTH     "length"
#define UDP_FIELD_CHECKSUM   "checksum"


// XXX mandatory fields ?
// XXX UDP parsing missing

// BSD/Linux abstraction
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

/**
 * UDP fields
 */

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

/**
 * Default UDP values
 */

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

size_t udp_get_num_fields(void) {
    return sizeof(udp_fields) / sizeof(protocol_field_t);
}

/**
 * \brief Retrieve the size of an UDP header 
 * \return The size of an UDP header
 */

size_t udp_get_header_size(void) {
    return sizeof(struct udphdr);
}

/**
 * \brief Write the default UDP header
 * \param data The address of an allocated buffer that will store the header
 */

void udp_write_default_header(uint8_t *data) {
    memcpy(data, &udp_default, sizeof(struct udphdr));
}

/**
 * \brief Compute and write the checksum related to an UDP header
 * \param udp_header Points to the begining of the UDP header and its content.
 *    The UDP checksum stored in this header is updated by this function.
 * \param ip_psh The IP layer part of the pseudo header. This buffer should
 *    contain the content of an ipv4_pseudo_header_t or an ipv6_pseudo_header_t
 *    structure.
 * \sa http://www.networksorcery.com/enp/protocol/udp.htm#Checksum
 * \return true if everything is fine, false otherwise  
 */

bool udp_write_checksum(uint8_t * udp_header, buffer_t * ip_psh)
{
    struct udphdr * udp_hdr = (struct udphdr *) udp_header;
    uint8_t       * psh;
    size_t          size_psh;

    // UDP checksum requires an part of the IP header
    if (!ip_psh) {
        errno = EINVAL;
        return false;
    }

    // Allocate the buffer which will contains the pseudo header
    size_psh = ntohs(udp_hdr->LENGTH) + buffer_get_size(ip_psh);
    if (!(psh = malloc(size_psh * sizeof(uint8_t)))) {
        return false;
    }

    // Put the excerpt of the IP header into the pseudo header
    memcpy(psh, buffer_get_data(ip_psh), buffer_get_size(ip_psh));

    // Put the UDP header and its content into the pseudo header
    memcpy(psh + buffer_get_size(ip_psh), udp_hdr, ntohs(udp_hdr->LENGTH));//sizeof(struct udphdr));

    // Overrides the UDP checksum in psh with zeros
    // Checksum debug: http://www4.ncsu.edu/~mlsichit/Teaching/407/Resources/udpChecksum.html
    memset(psh + buffer_get_size(ip_psh) + offsetof(struct udphdr, CHECKSUM), 0, sizeof(uint16_t));

    // Compute the checksum
    udp_hdr->check = csum((const uint16_t *) psh, size_psh);
    free(psh);
    return true;
}

/**
 * IPv4 pseudo header
 */

typedef struct {
    uint32_t  ip_src;    /**< Source IPv4 */
    uint32_t  ip_dst;    /**< Destination IPv4 */
    uint8_t   zero;      /**< Zeros */
    uint8_t   protocol;  /**< Protocol number of the first nested protocol (ex: UDP == 17) */
    uint16_t  size;      /**< Size of IP layer contents (IP packet size - IP header size)  */
} ipv4_pseudo_header_t;

/**
 * \brief Create an IPv4 pseudo header
 * \param ipv4_segment Address of the IPv4 segment 
 * \return The buffer containing the corresponding pseudo header,
 *    NULL in case of failure
 */

static buffer_t * udp_create_psh_ipv4(const uint8_t * ipv4_segment)
{
    const struct iphdr * ip_hdr = (const struct iphdr *) ipv4_segment;
    buffer_t           * ipv4_psh;
    ipv4_pseudo_header_t ipv4_pseudo_header; // TODO we should directly points to ipv4_psh's bytes to avoid one copy
    
    if (!(ipv4_psh = buffer_create())) {
        printf("udp_create_psh_ipv4: error create\n");
        goto ERR_BUFFER_CREATE;
    }

    // Deduce the size of the UDP segment (header + data) according to the IP header
    // - size of the IP segment: ip_hdr->tot_len
    // - size of the IP header:  4 * ip_hdr->ihl
    size_t udp_segment_size = htons(ntohs(ip_hdr->tot_len) - 4 * ip_hdr->ihl);

    // Initialize the pseudo header
    ipv4_pseudo_header.ip_src   = ip_hdr->saddr;
    ipv4_pseudo_header.ip_dst   = ip_hdr->daddr;
    ipv4_pseudo_header.zero     = 0;
    ipv4_pseudo_header.protocol = ip_hdr->protocol;
    ipv4_pseudo_header.size     = udp_segment_size; 

    // Prepare and return the corresponding buffer
    if (!buffer_write_bytes(ipv4_psh, (uint8_t *) &ipv4_pseudo_header, sizeof(ipv4_pseudo_header_t))) {
        printf("udp_create_psh_ipv4: error write\n");
        goto ERR_BUFFER_WRITE_BYTES;
    }

    return ipv4_psh;

ERR_BUFFER_WRITE_BYTES:
    buffer_free(ipv4_psh);
ERR_BUFFER_CREATE:
    return NULL;
}

/**
 * IPv6 pseudo header: http://tools.ietf.org/html/rfc2460
 */

typedef struct {
    uint128_t ip_src;    /**< Source IPv6      */
    uint128_t ip_dst;    /**< Destination IPv6 */
    uint32_t  size;      /**< Size of IP layer contents (IP packet size - IP header size)  */ 
    uint16_t  zeros;     /**< Zeros */
    uint8_t   zero;      /**< Zeros */
    uint8_t   protocol;  /**< Protocol number of the first nested protocol (ex: UDP == 17) */
} ipv6_pseudo_header_t;

/*
#define HD_UDP6_DADDR 16
#define HD_UDP6_LEN 32
#define HD_UDP6_PAD 36
#define HD_UDP6_NXH 39
*/

static buffer_t * udp_create_psh_ipv6(const uint8_t * ipv6_segment)
{
    buffer_t             * psh;
    const struct ip6_hdr * iph = (const struct ip6_hdr *) ipv6_segment;
    ipv6_pseudo_header_t * data;

    if (!(psh = buffer_create())) {
        goto ERR_BUFFER_CREATE;
    }

    if (!(buffer_resize(psh, sizeof(ipv6_pseudo_header_t)))) {
        goto ERR_BUFFER_RESIZE;
    }

    data = (ipv6_pseudo_header_t *) buffer_get_data(psh);

/*
    *(( uint32_t *) (data     ))                 =  iph->ip6_src.__in6_u.__u6_addr32[0]; // src_ip
    *(( uint32_t *) (data + 4 ))                 =  iph->ip6_src.__in6_u.__u6_addr32[1]; // src_ip
    *(( uint32_t *) (data + 8 ))                 =  iph->ip6_src.__in6_u.__u6_addr32[2]; // src_ip
    *(( uint32_t *) (data + 12))                 =  iph->ip6_src.__in6_u.__u6_addr32[3]; // src_ip
    *(( uint32_t *) (data + HD_UDP6_DADDR     )) =  iph->ip6_dst.__in6_u.__u6_addr32[0]; // dst_ip
    *(( uint32_t *) (data + HD_UDP6_DADDR + 4 )) =  iph->ip6_dst.__in6_u.__u6_addr32[1]; // dst_ip
    *(( uint32_t *) (data + HD_UDP6_DADDR + 8 )) =  iph->ip6_dst.__in6_u.__u6_addr32[2]; // dst_ip
    *(( uint32_t *) (data + HD_UDP6_DADDR + 12)) =  iph->ip6_dst.__in6_u.__u6_addr32[3]; // dst_ip
*/
    memcpy(data + offsetof(ipv6_pseudo_header_t, ip_src), &iph->ip6_src, sizeof(struct in6_addr));
    memcpy(data + offsetof(ipv6_pseudo_header_t, ip_dst), &iph->ip6_dst, sizeof(struct in6_addr));
//    *(( uint16_t *) (data + HD_UDP6_LEN))        =  iph->ip6_ctlun.ip6_un1.ip6_un1_plen; // XXX ext headers need to be substracted
    data->size = iph->ip6_ctlun.ip6_un1.ip6_un1_plen; // XXX ext headers need to be substracted
//    *(( uint32_t *) (data + HD_UDP6_PAD))        =  0x00000000; // 3 Bytes zero (3 stay zero, one gets overwritten)
    data->zeros = 0;
    data->zero  = 0;
//    *(( uint8_t  *) (data + HD_UDP6_NXH))        = IPPROTO_UDP;  // UDP takes last byte

    return psh;

ERR_BUFFER_RESIZE:
    buffer_free(psh);
ERR_BUFFER_CREATE:
    return NULL;
}

buffer_t * udp_create_psh(const uint8_t * buffer)
{
    // TODO dispatch IPv4 and IPv6 header
    // http://www.networksorcery.com/enp/protocol/udp.htm#Checksum
    // XXX IPv6 hacks -> todo generic.
    /* buffer_t *psh;
    unsigned char ip_version = buffer_guess_ip_version(buffer); 
   
       psh = ip_version == 6 ? udp_create_psh_ipv6(buffer) :
                        == 4 ? udp_create_psh_ipv4(buffer) :
                        NULL;
       if(!psh){
           perror("E:can not create udp pseudo header");
       }*/
    return udp_create_psh_ipv4(buffer);
}

static protocol_t udp = {
    .name                 = "udp",
    .protocol             = IPPROTO_UDP, 
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
