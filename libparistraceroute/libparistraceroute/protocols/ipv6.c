#include <stddef.h>       // offsetof()
#include <string.h>       // memcpy(), memset()
#include <unistd.h>       // close()
#include <arpa/inet.h>    // inet_pton()
#include <netinet/ip6.h>
#include <stdio.h>        // perror
#include <sys/types.h>
#include <netdb.h>
#include <sys/socket.h>

#include "../field.h"
#include "../protocol.h"

// TODO rfc6564/rfc6437/rfc5095 ?

/* Field names */
#define IPV6_FIELD_VERSION           "version"
#define IPV6_FIELD_TCL               "tcl"
//#define IPV6_FIELD_FLOWLABEL         "flowlabel"
#define IPV6_FIELD_NEXT_HEADER       "next_header"
#define IPV6_FIELD_SRC_IP            "src_ip"
#define IPV6_FIELD_DST_IP            "dst_ip"

// Watch out for this one, it is needed since probe.c uses it. All Protocols need it.
// In IPv6 it is mapped to NEXT_HEADER TODO find out whether that is OK
#define IPV6_FIELD_PROTOCOL          "protocol"
// XXX compat for now, need lenght field. Ugly Hack
#define IPV6_FIELD_PAYLOADLENGTH     "length"
//#define IPV6_FIELD_PAYLOADLENGTH     "payload_length"
// XX compat for now, ttl vs hoplimit
//#define IPV6_FIELD_HOPLIMIT          "hoplimit"
#define IPV6_FIELD_HOPLIMIT          "ttl"
// Hacking ... Split field in two 16 bit
// Version + TCL + 4 bits of flow that we ignore
#define IPV6_FIELD_FLOWLABEL_LOWER         "flow_id"
// Version + TCL + 4 bits
#define IPV6_FIELD_FLOWLABEL_UPPER         "flow_upper"

// Extension Headers
#define IPV6_FIELD_HBH_NEXT_HEADER   "hbh_next_header"
#define IPV6_FIELD_HBH_HDR_EXT_LEN   "hbh_hdr_ext_len"
#define IPV6_FIELD_HBH_OPTIONS       "hbh_options"

#define IPV6_FIELD_RH_NEXT_HEADER        "rh_next_header"
#define IPV6_FIELD_RH_HDR_EXT_LEN        "rh_hdr_ext_len"
#define IPV6_FIELD_RH_ROUTING_TYPE       "rh_routing_type"
#define IPV6_FIELD_RH_SEGMENTS_LEFT      "rh_segments_left"
#define IPV6_FIELD_RH_TYPE_SPECIFIC_DATA "rh_type_specific_data"

#define IPV6_FIELD_FH_NEXT_HEADER        "fh_next_header"
#define IPV6_FIELD_FH_RESERVED           "fh_reserved"
#define IPV6_FIELD_FH_FRAGMENT_OFFSET    "fh_fragment_offset"
#define IPV6_FIELD_FH_RES                "fh_res"
#define IPV6_FIELD_FH_M_FLAG             "fh_m_flag"
#define IPV6_FIELD_FH_IDENTIFICATION     "fh_identification"

#define IPV6_FIELD_DOH_NEXT_HEADER    "doh_next_header"
#define IPV6_FIELD_DOH_HDR_EXT_LEN    "doh_hdr_ext_len"
#define IPV6_FIELD_DOH_OPTIONS        "doh_options"


// TODO find defaults
/* Default field values */
#define IPV6_DEFAULT_VERSION         6
#define IPV6_DEFAULT_TCL             0 // XXX useful default?
#define IPV6_DEFAULT_FLOWLABEL_LOWER 0
#define IPV6_DEFAULT_FLOWLABEL_UPPER 0

#define IPV6_DEFAULT_PAYLOADLENGTH   0
#define IPV6_DEFAULT_NEXT_HEADER     17 // IPv6
#define IPV6_DEFAULT_HOPLIMIT        64
#define IPV6_DEFAULT_SRC_IP          0
#define IPV6_DEFAULT_DST_IP          0

/* Extension Headers */
#define IPV6_DEFAULT_HBH_NEXT_HEADER   59 // NO next header
#define IPV6_DEFAULT_HBH_HDR_EXT_LEN   0 // XXX TBD
#define IPV6_DEFAULT_HBH_OPTIONS       0

#define IPV6_DEFAULT_RH_NEXT_HEADER        59 // NO next header
#define IPV6_DEFAULT_RH_HDR_EXT_LEN        0 // XXX TBD
#define IPV6_DEFAULT_RH_ROUTING_TYPE       0
#define IPV6_DEFAULT_RH_SEGMENTS_LEFT      0
#define IPV6_DEFAULT_RH_TYPE_SPECIFIC_DATA 0

#define IPV6_DEFAULT_FH_NEXT_HEADER     59 // NO next header
#define IPV6_DEFAULT_FH_RESERVED        0
#define IPV6_DEFAULT_FH_FRAGMENT_OFFSET 0
#define IPV6_DEFAULT_FH_RES             0
#define IPV6_DEFAULT_FH_M_FLAG          0
#define IPV6_DEFAULT_FH_IDENTIFICATION  0

#define IPV6_DEFAULT_DOH_NEXT_HEADER    59 // NO next header
#define IPV6_DEFAULT_DOH_HDR_EXT_LEN    0
#define IPV6_DEFAULT_DOH_OPTIONS        0

#define IPV6_STRSIZE 46 // as of netinet/in.h INET6_ADDRSTRLEN

// Accessors

/**
 * \brief Update the source IP of an IPv6 header.
 * \param ipv6_header Address of the IPv6 header we want to update
 * \param field The string field containing the new source (resolved) IP.
 * \return true iif successful.
 */

bool ipv6_set_src_ip(uint8_t * ipv6_header, const field_t * field){
    struct ip6_hdr * ip6_hed = (struct ip6_hdr *) ipv6_header;
    return (inet_pton(AF_INET6, (const char *) field->value.string, &ip6_hed->ip6_src) != -1);
}

/**
 * \brief Create the a string field containing the source IP of an IPv6 header.
 * \param ipv6_header The queried IPv6 header.
 * \return The corresponding field, NULL in case of failure.
 */

field_t * ipv6_get_src_ip(const uint8_t * ipv6_header){
    char res[IPV6_STRSIZE];
    const struct ip6_hdr * ip6_hed = (const struct ip6_hdr *) ipv6_header;

    memset(res, 0, IPV6_STRSIZE);
    inet_ntop(AF_INET6, &ip6_hed->ip6_src, res, IPV6_STRSIZE);
    return STR(IPV6_FIELD_SRC_IP, res);
}

/**
 * \brief Update the destination IP of an IPv6 header according to a field
 * \param ipv6_header Address of the IPv6 header we want to update
 * \param field The string field containing the new destination (resolved) IP
 * \return true iif successful
 */

bool ipv6_set_dst_ip(uint8_t * ipv6_header, const field_t * field){
    struct ip6_hdr * ip6_hed = (struct ip6_hdr *) ipv6_header;
    return (inet_pton(AF_INET6, (const char *) field->value.string, &ip6_hed->ip6_dst) != -1);
}

/**
 * \brief Create the a string field containing the destination IP of an IPv6 header.
 * \param ipv6_header The queried IPv4 header.
 * \return The corresponding field, NULL in case of failure.
 */

field_t * ipv6_get_dst_ip(const uint8_t * ipv6_header){
    char res[IPV6_STRSIZE];
    const struct ip6_hdr * ip6_hed = (const struct ip6_hdr *) ipv6_header;

    memset(res, 0, IPV6_STRSIZE);
    inet_ntop(AF_INET6, &ip6_hed->ip6_dst, res, IPV6_STRSIZE);
    return STR(IPV6_FIELD_DST_IP, res);
}


/* IPv6 fields */
static protocol_field_t ipv6_fields[] = {

    {
//        .key      = IPV6_FIELD_VERSION,
//        .type     = TYPE_UINT4,
//        .offset   = offsetof(struct ip6_hdr, ip6_ctlun.ip6_un2_vfc), // ip6_un1_flow contains 4 bits version, 8 bits tcl, 20 bits flow-ID
//    }, {
//        .key      = IPV6_FIELD_TCL,
//        .type     = TYPE_UINT8,
//        .offset   = offsetof(struct ip6_hdr, ip6_un1_flow,) // Same as above 8 bits
//    }, {
        .key      = IPV6_FIELD_FLOWLABEL_LOWER,
        .type     = TYPE_UINT16,
        .offset   = (offsetof(struct ip6_hdr,  ip6_ctlun.ip6_un1.ip6_un1_flow) +2), // Same as above 20 bits /reduce it to 16 for testing.
    }, {
        .key      = IPV6_FIELD_PAYLOADLENGTH,
        .type     = TYPE_UINT16,
        .offset   = offsetof(struct ip6_hdr, ip6_ctlun.ip6_un1.ip6_un1_plen),
    }, {
        .key      = IPV6_FIELD_NEXT_HEADER,
        .type     = TYPE_UINT8,
        .offset   = offsetof(struct ip6_hdr, ip6_ctlun.ip6_un1.ip6_un1_nxt),
    }, {
        .key      = IPV6_FIELD_HOPLIMIT,
        .type     = TYPE_UINT8,
        .offset   = offsetof(struct ip6_hdr, ip6_ctlun.ip6_un1.ip6_un1_hlim),
    }, {
        .key      = IPV6_FIELD_PROTOCOL,
        .type     = TYPE_UINT8,
        .offset   = offsetof(struct ip6_hdr, ip6_ctlun.ip6_un1.ip6_un1_nxt),
    }, {
        .key      = IPV6_FIELD_SRC_IP,
        .type     = TYPE_UINT128,
        .offset   = offsetof(struct ip6_hdr, ip6_src),
        .set      = ipv6_set_src_ip,
        .get      = ipv6_get_src_ip,
    }, {
       .key      = IPV6_FIELD_DST_IP,
       .type     = TYPE_UINT128,
       .offset   = offsetof(struct ip6_hdr, ip6_dst),
       .set      = ipv6_set_dst_ip,
       .get      = ipv6_get_dst_ip,
    },
    END_PROTOCOL_FIELDS
};


/* Default IPv6 values */
static struct ip6_hdr ipv6_default = {
// TODO make this work :) flowID mingle
//    .version        = IPV6_DEFAULT_VERSION,
//    .tcl            = IPV6_DEFAULT_TCL,
//    .flowlabel        = IPV6_DEFAULT_FLOWLABEL,
    .ip6_ctlun.ip6_un1.ip6_un1_flow = 0x00000006, // Version = 6, TCL = 0, FLOW = 0 // Beware of Byteorder
    .ip6_ctlun.ip6_un1.ip6_un1_plen = IPV6_DEFAULT_PAYLOADLENGTH,
    .ip6_ctlun.ip6_un1.ip6_un1_nxt  = IPV6_DEFAULT_NEXT_HEADER,
    .ip6_ctlun.ip6_un1.ip6_un1_hlim = IPV6_DEFAULT_HOPLIMIT,
    .ip6_src                        = IPV6_DEFAULT_SRC_IP,
    .ip6_dst                        = IPV6_DEFAULT_DST_IP,
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

    iph->ip6_ctlun.ip6_un2_vfc = (uint8_t) 0x60;

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
 * \brief Write the default IPv6 header
 * \param iv6_header The address of an allocated buffer that will
 *    store the IPv6 header or NULL.
 * \return The size of the default header.
 */

size_t ipv6_write_default_header(uint8_t * ipv6_header) {
    size_t size = sizeof(struct ip6_hdr);
    if (ipv6_header) memcpy(ipv6_header, &ipv6_default, size);
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
    .protocol             = 6,
    .write_checksum       = NULL, // IPv6 has no checksum, it depends on upper layers
    .create_pseudo_header = NULL,
    .fields               = ipv6_fields,
    .write_default_header = ipv6_write_default_header, // TODO generic with ipv4
//    .socket_type            = NULL, // TODO WHY?
    .get_header_size      = ipv6_get_header_size,
    .finalize             = ipv6_finalize,
    .instance_of          = ipv6_instance_of,
};

PROTOCOL_REGISTER(ipv6);
