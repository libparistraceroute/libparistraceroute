#include "use.h"
#ifdef USE_IPV4

#include "ipv4_pseudo_header.h"

#include "os/netinet/ip.h"    // ip_hdr
#include <arpa/inet.h>        // htons

buffer_t * ipv4_pseudo_header_create(const uint8_t * ipv4_segment)
{
    const struct iphdr * ip_hdr = (const struct iphdr *) ipv4_segment;
    buffer_t           * ipv4_psh;
    ipv4_pseudo_header_t ipv4_pseudo_header; // TODO we should directly points to ipv4_psh's bytes to avoid one copy

    if (!(ipv4_psh = buffer_create())) {
        goto ERR_BUFFER_CREATE;
    }

    // Deduce the size of the UDP segment (header + data) according to the IP header
    // - size of the IP segment: ip_hdr->tot_len
    // - size of the IP header:  4 * ip_hdr->ihl
    size_t size = htons(ntohs(ip_hdr->tot_len) - 4 * ip_hdr->ihl);

    // Initialize the pseudo header
    ipv4_pseudo_header.ip_src   = ip_hdr->saddr;
    ipv4_pseudo_header.ip_dst   = ip_hdr->daddr;
    ipv4_pseudo_header.zero     = 0;
    ipv4_pseudo_header.protocol = ip_hdr->protocol;
    ipv4_pseudo_header.size     = size;

    // Prepare and return the corresponding buffer
    if (!buffer_write_bytes(ipv4_psh, (uint8_t *) &ipv4_pseudo_header, sizeof(ipv4_pseudo_header_t))) {
        goto ERR_BUFFER_WRITE_BYTES;
    }

    return ipv4_psh;

ERR_BUFFER_WRITE_BYTES:
    buffer_free(ipv4_psh);
ERR_BUFFER_CREATE:
    return NULL;
}

#endif // USE_IPV4

