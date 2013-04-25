#include <stdlib.h>           // malloc()
#include <string.h>           // memcpy()
#include <stdbool.h>          // bool
#include <errno.h>            // ERRNO, EINVAL
#include <stddef.h>           // offsetof()
#include <netinet/ip.h>
#include <netinet/ip6.h>
#include <netinet/udp.h>
#include <netinet/in.h>       // IPPROTO_UDP = 17
#include <arpa/inet.h>

#include "../protocol.h"
#include "buffer.h"
#define UDP_DEFAULT_SRC_PORT 2828
#define UDP_DEFAULT_DST_PORT 2828

#define UDP_FIELD_SRC_PORT "src_port"
#define UDP_FIELD_DST_PORT "dst_port"
#define UDP_FIELD_LENGTH   "length"
#define UDP_FIELD_CHECKSUM "checksum"


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

unsigned int udp_get_num_fields(void) {
    return sizeof(udp_fields) / sizeof(protocol_field_t);
}

/**
 * \brief Retrieve the size of an UDP header 
 * \return The size of an UDP header
 */

unsigned int udp_get_header_size(void) {
    return sizeof(struct udphdr);
}

/**
 * \brief Write the default UDP header
 * \param data The address of an allocated buffer that will store the header
 */

<<<<<<< HEAD
void udp_write_default_header(unsigned char *data) {
=======
inline void udp_write_default_header(uint8_t *data) {
>>>>>>> origin/master
    memcpy(data, &udp_default, sizeof(struct udphdr));
}

// TODO Check Checksum calculation for ipv6
/**
 * \brief Compute and write the checksum related to an UDP header
 * \param udp_header A pre-allocated UDP header. The UDP checksum
 *    stored in this header is updated by this function.
 * \param ip_psh The IP layer part of the pseudo header:
 *    In IPv4: {
 *       src_ip   (32 bits),
 *       dst_ip   (32 bits),
 *       zeros    (8  bits),
 *       proto    (8  bits),
 *       size_udp (16 bits) 
 *    }
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
        errno = ENOMEM;
        return false;
    }

    // Put the excerpt of the IP header into the pseudo header
    memcpy(psh, buffer_get_data(ip_psh), buffer_get_size(ip_psh));

    // Put the UDP header into the pseudo header
    memcpy(psh + buffer_get_size(ip_psh), udp_hdr, ntohs(udp_hdr->LENGTH));//sizeof(struct udphdr));

    // Compute the checksum
    udp_hdr->check = csum((const uint16_t *) psh, size_psh);
    free(psh);
    return true;
}

// TODO define struct ipv4_psh_t ? 

<<<<<<< HEAD
#define HD_UDP6_DADDR 16
#define HD_UDP6_LEN 32
#define HD_UDP6_PAD 36
#define HD_UDP6_NXH 39


buffer_t * udp_create_psh_ipv4(unsigned char* ipv4_buffer)
//unsigned char** pseudo_header, int* size)
=======
buffer_t * udp_create_psh_ipv4(uint8_t * ipv4_buffer)
>>>>>>> origin/master
{
    buffer_t     * ipv4_psh;
    struct iphdr * ip_hdr = (struct iphdr *) ipv4_buffer;
    uint8_t      * data;
    
<<<<<<< HEAD
    psh = buffer_create();
    buffer_resize(psh, 12);

    data = buffer_get_data(psh);


    iph = (struct iphdr*) ipv4_buffer;
    *(( u_int32_t *)  data                ) = (u_int32_t) iph->saddr;
    *(( u_int32_t *) (data + HD_UDP_DADDR)) = (u_int32_t) iph->daddr;
    *(( u_int8_t  *) (data + HD_UDP_PAD  )) =             0;
    *(( u_int8_t  *) (data + HD_UDP_PROT )) = (u_int8_t)  iph->protocol;
    *(( u_int16_t *) (data + HD_UDP_LEN  )) = (u_int16_t) htons(ntohs(iph->tot_len)-4*iph->ihl);
=======
    if (!(ipv4_psh = buffer_create())) {
        errno = ENOMEM;
        return NULL;
    }
>>>>>>> origin/master

    // The IPv4 part of a pseudo header is made of 12 bytes:
    // {
    //   src_ip   (4 bytes) (offset 0  byte)
    //   dst_ip   (4 bytes) (offset 4  bytes)
    //   zeros    (1 byte)  (offset 8  bytes)
    //   proto    (1 byte)  (offset 9  bytes)
    //   size_udp (2 bytes) (offset 10 bytes)
    // } = 12 bytes

    buffer_resize(ipv4_psh, 12);
    data = buffer_get_data(ipv4_psh);

    *((uint32_t *) (data + 0 )) = (uint32_t) ip_hdr->saddr;
    *((uint32_t *) (data + 4 )) = (uint32_t) ip_hdr->daddr;
    *((uint8_t  *) (data + 8 )) = 0;
    *((uint8_t  *) (data + 9 )) = (uint8_t)  ip_hdr->protocol;
    *((uint16_t *) (data + 10)) = (uint16_t) htons(ntohs(ip_hdr->tot_len) - 4 * ip_hdr->ihl);

    return ipv4_psh;
}

<<<<<<< HEAD
buffer_t * udp_create_psh_ipv6(unsigned char* ipv6_buffer)
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
    buffer_t *psh;
    struct ip6_hdr *iph;
    unsigned char *data;

    psh = buffer_create();
    buffer_resize(psh, 40);

    data = buffer_get_data(psh);


    iph = (struct ip6_hdr *) ipv6_buffer;
    *(( u_int32_t *) (data                ))	=  iph->ip6_src.__in6_u.__u6_addr32[0]; // src_ip
    *(( u_int32_t *) (data  + 4           ))	=  iph->ip6_src.__in6_u.__u6_addr32[1]; // src_ip
    *(( u_int32_t *) (data  + 8           )) 	=  iph->ip6_src.__in6_u.__u6_addr32[2]; // src_ip
    *(( u_int32_t *) (data  + 12          ))	=  iph->ip6_src.__in6_u.__u6_addr32[3]; // src_ip
    *(( u_int32_t *) (data + HD_UDP6_DADDR   ))	=  iph->ip6_dst.__in6_u.__u6_addr32[0]; // dst_ip
    *(( u_int32_t *) (data + HD_UDP6_DADDR+4 ))	=  iph->ip6_dst.__in6_u.__u6_addr32[1]; // dst_ip
    *(( u_int32_t *) (data + HD_UDP6_DADDR+8 ))	=  iph->ip6_dst.__in6_u.__u6_addr32[2]; // dst_ip
    *(( u_int32_t *) (data + HD_UDP6_DADDR+12))	=  iph->ip6_dst.__in6_u.__u6_addr32[3]; // dst_ip
    *(( u_int16_t *) (data + HD_UDP6_LEN))		=  iph->ip6_ctlun.ip6_un1.ip6_un1_plen; // XXX ext headers need to be substracted
    *(( u_int32_t  *) (data + HD_UDP6_PAD  )) 	=  0x00000000; // 3 Bytes zero (3 stay zero, one gets overwritten)
    *(( u_int8_t  *) (data + HD_UDP6_NXH )) 	= 17;  // UDP takes last byte

    return psh;

}

buffer_t * udp_create_psh(unsigned char * buffer)
=======
buffer_t * udp_create_psh(uint8_t * buffer)
>>>>>>> origin/master
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
