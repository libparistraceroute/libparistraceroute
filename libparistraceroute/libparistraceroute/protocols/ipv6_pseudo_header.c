#include "ipv6_pseudo_header.h"

#include <string.h>           // memcpy
#include <netinet/ip6.h>      // ip6_hdr


buffer_t * ipv6_pseudo_header_create(const uint8_t * ipv6_segment)
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

