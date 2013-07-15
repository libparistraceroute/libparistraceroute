#include <stddef.h>       // offsetof()
#include <string.h>       // memcpy(), memset()
#include <unistd.h>       // close()
#include <netinet/in.h>   // IPPROTO_UDP
#include <netinet/ip6.h>  // ip6_hdr
#include <stdio.h>        // perror
#include <bits/endian.h>

#include "../field.h"
#include "../protocol.h"

// TODO rfc6564/rfc6437/rfc5095 ?

// Field names
#define IPV6_FIELD_VERSION           "version"
#define IPV6_FIELD_TRAFFIC_CLASS     "traffic_class"
#define IPV6_FIELD_FLOWLABEL         "flow_label"
#define IPV6_FIELD_PAYLOADLENGTH     "payload_length" // TODO
#define IPV6_FIELD_HOPLIMIT          "hop_limit"
#define IPV6_FIELD_NEXT_HEADER       "next_header"
#define IPV6_FIELD_SRC_IP            "src_ip"
#define IPV6_FIELD_DST_IP            "dst_ip"

// Fields to be IPv4 compliant (these are simply aliases)
#define IPV6_FIELD_PROTOCOL          "protocol"

#define IPV6_FIELD_LENGTH            "length" // TODO define payload_length which returns ip6.plen and length with return ip6.plen + 40 and the appropriates cbs
#define IPV6_FIELD_TTL               "ttl"

// Default field values
#define IPV6_DEFAULT_VERSION         6
#define IPV6_DEFAULT_TRAFFIC_CLASS   0
#define IPV6_DEFAULT_FLOW_LABEL      0
#define IPV6_DEFAULT_PAYLOAD_LENGTH  0
#define IPV6_DEFAULT_NEXT_HEADER     IPPROTO_UDP
#define IPV6_DEFAULT_HOPLIMIT        64
#define IPV6_DEFAULT_SRC_IP          0
#define IPV6_DEFAULT_DST_IP          0

// IPv6 fields

static protocol_field_t ipv6_fields[] = {

    {
//        .key      = IPV6_FIELD_VERSION,
//        .type     = TYPE_UINT4,
//        .offset   = offsetof(struct ip6_hdr, ip6_flow), // ip6_un1_flow contains 4 bits version, 8 bits tcl, 20 bits flow-ID
//    }, {
//        .key      = IPV6_FIELD_TRAFFIC_CLASS,
//        .type     = TYPE_UINT8,
//        .offset   = offsetof(struct ip6_hdr, ip6_un1_flow,) // Same as above 8 bits
//    }, {
        .key     = IPV6_FIELD_FLOWLABEL,
        .type    = TYPE_UINT16,
        .offset  = (offsetof(struct ip6_hdr, ip6_ctlun.ip6_un1.ip6_un1_flow) + 2), // May contain 20 bits / reduced to 16 bits for testing.
    }, {
        .key     = IPV6_FIELD_LENGTH,
        .type    = TYPE_UINT16,
        .offset  = offsetof(struct ip6_hdr, ip6_plen),
    }, {
        // "next_header" is an alias of "protocol"
        .key     = IPV6_FIELD_NEXT_HEADER,
        .type    = TYPE_UINT8,
        .offset  = offsetof(struct ip6_hdr, ip6_nxt),
    }, {
        .key     = IPV6_FIELD_PROTOCOL,
        .type    = TYPE_UINT8,
        .offset  = offsetof(struct ip6_hdr, ip6_nxt),
    }, {
        .key     = IPV6_FIELD_HOPLIMIT,
        .type    = TYPE_UINT8,
        .offset  = offsetof(struct ip6_hdr, ip6_hlim),
    }, {
        // "ttl" is an alias of "hop_limit"
        .key     = IPV6_FIELD_TTL,
        .type    = TYPE_UINT8,
        .offset  = offsetof(struct ip6_hdr, ip6_hlim),
    }, {
        .key     = IPV6_FIELD_SRC_IP,
        .type    = TYPE_IPV6,
        .offset  = offsetof(struct ip6_hdr, ip6_src),
    }, {
       .key      = IPV6_FIELD_DST_IP,
       .type     = TYPE_IPV6,
       .offset   = offsetof(struct ip6_hdr, ip6_dst),
    },
    END_PROTOCOL_FIELDS
};

// Default IPv6 values 
static const struct ip6_hdr ipv6_default = {
    .ip6_flow = 0, // This will be directly set in ipv6_write_default_header
    .ip6_plen = IPV6_DEFAULT_PAYLOAD_LENGTH,
    .ip6_nxt  = IPV6_DEFAULT_NEXT_HEADER,
    .ip6_hlim = IPV6_DEFAULT_HOPLIMIT,
    .ip6_src  = IPV6_DEFAULT_SRC_IP,
    .ip6_dst  = IPV6_DEFAULT_DST_IP,
};

bool ipv6_get_default_src_ip(struct in6_addr dst_ipv6, struct in6_addr * psrc_ipv6) {
    struct sockaddr_in6 addr, name;
    int                 sockfd;
    short               family  = AF_INET6;
    socklen_t           addrlen = sizeof(struct sockaddr_in6);

    if ((sockfd = socket(family, SOCK_DGRAM, 0)) == -1) {
        goto ERR_SOCKET;
    }

    memset(&addr, 0, addrlen);
    addr.sin6_family = family;
    addr.sin6_addr = dst_ipv6;

    if (connect(sockfd, (struct sockaddr *) &addr, addrlen) == -1) {
        perror("E: Cannot create IPv6 socket");
        goto ERR_CONNECT;
    }

    if (getsockname(sockfd, (struct sockaddr *) &name, &addrlen) == -1) {
        goto ERR_GETSOCKNAME;
    }

    close(sockfd);
    *psrc_ipv6 = name.sin6_addr;
    return true;

ERR_GETSOCKNAME:
    close(sockfd);
ERR_CONNECT:
ERR_SOCKET:
    return false;
}

/**
 * \brief A set of actions to be done before sending the packet.
 * \param ipv6_header Address of the IPv6 header we want to update.
 */

bool ipv6_finalize(uint8_t * ipv6_header) {
    size_t           i;
    struct ip6_hdr * iph = (struct ip6_hdr *) ipv6_header;
    bool             do_update_src_ip = true,
                     ret = true;

    // If at least one byte of the src_ip is not null, we suppose
    // that the src_ip has been set...
    for (i = 0; i < 8 && !do_update_src_ip; i++) {
        if (iph->ip6_src.__in6_u.__u6_addr16[i] != 0) {
            do_update_src_ip = false;
            break;
        }
    }

    // ... otherwise, we set the src_ip
    if (do_update_src_ip) {
        ret = ipv6_get_default_src_ip(iph->ip6_dst, &iph->ip6_src);
    }

    return ret;
}

/**
 * \brief Retrieve the size of an IPv6 header
 * \param ipv6_header Address of an IPv6 header or NULL
 * \return The size of an IPv6 header
 */

size_t ipv6_get_header_size(const uint8_t * ipv6_header) {
    return sizeof(struct ip6_hdr);
}

/**
 * \brief Prepare an IPv6 "flow" (ie {version, traffic class, flow label} tuple).
 * \param version The IP version. Only the 4 less significant bits are considered.
 *    Version should be equal 0x6.
 * \param traffic_class The IPv6 traffic class.
 * \param flow_label The IPv6 flow label (host-side endianness). Only the 20 less
 *    significant bits are considered.
 * \return The corresponding flow (network-side endianness).
 */

static uint32_t ipv6_make_flow(uint8_t version, uint8_t traffic_class, uint32_t flow_label) {
    // Remove exceeding bits (useless for version which will be shifted
    // and for traffic_class which already has the right "size").
    flow_label &= 0xfffff;

    return htonl(
        (version       << 28) |
        (traffic_class << 20) |
        (flow_label)  
    );
}

/**
 * \brief Write the default IPv6 header
 * \param iv6_header The address of an allocated buffer that will
 *    store the IPv6 header or NULL.
 * \return The size of the default header.
 */

size_t ipv6_write_default_header(uint8_t * ipv6_header) {
    size_t   size = sizeof(struct ip6_hdr);
    uint32_t ipv6_flow;

    if (ipv6_header) {
        // Write the default IPv6 header.
        // The tuple "flow" = {version, traffic class, flow label} will be overwritten.
        memcpy(ipv6_header, &ipv6_default, size);

        // Prepare the default tuple {version, traffic class, flow label}.
        ipv6_flow = ipv6_make_flow(
            IPV6_DEFAULT_VERSION,
            IPV6_DEFAULT_TRAFFIC_CLASS,
            IPV6_DEFAULT_FLOW_LABEL
        );

        // Write this tuple in the header.
        memcpy(
            &((struct ip6_hdr *) ipv6_header)->ip6_flow,
            &ipv6_flow,
            sizeof(uint32_t)
        );
    }
    return size;
}

/**
 * \brief Test whether a sequence of bytes seems to be an IPv6 packet
 * \param bytes The sequence of evaluated bytes.
 * \return true iif it seems to be an IPv6 packet.
 */

bool ipv6_instance_of(uint8_t * bytes) {
    return (bytes[0] >> 4) == 6;
}

static protocol_t ipv6 = {
    .name                 = "ipv6",
    .protocol             = IPPROTO_IPV6,
    .write_checksum       = NULL,
    .create_pseudo_header = NULL,
    .fields               = ipv6_fields,
    .write_default_header = ipv6_write_default_header, // TODO generic with ipv4
    .get_header_size      = ipv6_get_header_size,
    .finalize             = ipv6_finalize,
    .instance_of          = ipv6_instance_of,
    .get_next_protocol    = protocol_get_next_protocol,
};

PROTOCOL_REGISTER(ipv6);
