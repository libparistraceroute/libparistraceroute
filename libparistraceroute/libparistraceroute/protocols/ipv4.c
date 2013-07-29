#include "../use.h"       // USE_BITS

#ifdef USE_IPV4

#include "config.h"

#include <stdbool.h>      // bool
#include <unistd.h>       // close()
#include <stddef.h>       // offsetof()
#include <string.h>       // memcpy()
#include <arpa/inet.h>    // inet_pton()
#include <netinet/ip.h>   // iphdr
#include <netinet/in.h>   // IPPROTO_IPIP, INET_ADDRSTRLEN

#include "../field.h"     // field_t
#include "../protocol.h"  // csum
#include "../bits.h"      // byte_* // TODO to remove

// Field names
#define IPV4_FIELD_VERSION           "version" 
#define IPV4_FIELD_IHL               "ihl"
#define IPV4_FIELD_TOS               "tos"
#define IPV4_FIELD_LENGTH            "length"
#define IPV4_FIELD_IDENTIFICATION    "identification"
#define IPV4_FIELD_FRAGOFF           "fragoff"
#define IPV4_FIELD_TTL               "ttl"
#define IPV4_FIELD_PROTOCOL          "protocol"
#define IPV4_FIELD_CHECKSUM          "checksum"
#define IPV4_FIELD_SRC_IP            "src_ip"
#define IPV4_FIELD_DST_IP            "dst_ip"

// Default field values
#define IPV4_DEFAULT_VERSION         4 
#define IPV4_DEFAULT_IHL             5
#define IPV4_DEFAULT_TOS             0
#define IPV4_DEFAULT_LENGTH          0
#define IPV4_DEFAULT_IDENTIFICATION  1
#define IPV4_DEFAULT_FRAGOFF         0
#define IPV4_DEFAULT_TTL             255 
#define IPV4_DEFAULT_PROTOCOL        IPPROTO_IPIP
#define IPV4_DEFAULT_CHECKSUM        0
#define IPV4_DEFAULT_SRC_IP          0 // See ipv4_get_default_src_ip()
#define IPV4_DEFAULT_DST_IP          0 // Must be set by the user (see network.c)

// The following offsets cannot be retrieved with offsetof() so they are hardcoded 
#ifdef USE_BITS
#    define IPV4_OFFSET_VERSION          0
#    define IPV4_OFFSET_IN_BITS_VERSION  0 // IPV4_OFFSET_VERSION * 8 + 0
#    define IPV4_OFFSET_IHL              0
#    define IPV4_OFFSET_IN_BITS_IHL      4 // IPV4_OFFSET_IHL * 8 + 4
#endif

//#ifdef USE_BITS
////-----------------------------------------------------------
//// set/get callbacks (accessors to IPv4 string and int4 fields) 
////-----------------------------------------------------------
//
//// TODO layer functions should directly call ipv4_create_ipv4_field 
//// and ipv4_field_to_ipv4, thus we could remove the 4 following
//// functions. For uint4 fields, pass the address of the aligned byte,
//// the setter and the getter must read/write the 4 appropriate bits.
//
///**
// * \brief Create the an int4 field containing the version of the IPv4 header
// * \param ipv4_header The queried IPv4 header. 
// * \return The corresponding field, NULL in case of failure.
// */
//
//field_t * ipv4_get_version(const uint8_t * ipv4_header) {
//    return BITS(
//        IPV4_FIELD_VERSION, 
//        ipv4_header + IPV4_OFFSET_VERSION,
//        IPV4_OFFSET_IN_BITS_VERSION,
//        4
//    );
//}
//
///**
// * \brief Update the version of an IPv4 header
// * \param field The string field containing the new source (resolved) IP
// * \return true iif successful
// */
//
//bool ipv4_set_version(uint8_t * ipv4_header, const field_t * field) {
//    return byte_write_bits(
//        ipv4_header + IPV4_OFFSET_VERSION,
//        IPV4_OFFSET_IN_BITS_VERSION,
//        field->value.bits, 4, 4
//    );
//}
//
///**
// * \brief Create the an int4 field containing the version of the IP header
// * \param ipv4_header The queried IPv4 header. 
// * \return The corresponding field, NULL in case of failure.
// */
//
//field_t * ipv4_get_ihl(const uint8_t * ipv4_header) {
//    return BITS(
//        IPV4_FIELD_IHL,
//        ipv4_header + IPV4_OFFSET_IHL,
//        IPV4_OFFSET_IN_BITS_IHL,
//        4
//    );
//}
//
///**
// * \brief Update the Internet Header Length (IHL) of an IPv4 header.
// * \param field The string field containing the new source (resolved) IP.
// * \return true iif successful.
// */
//
//bool ipv4_set_ihl(uint8_t * ipv4_header, const field_t * field) {
//    return byte_write_bits(
//        ipv4_header + IPV4_OFFSET_IHL,
//        IPV4_OFFSET_IN_BITS_IHL,
//        field->value.bits, 4, 4
//    );
//}
//#endif

//-----------------------------------------------------------
// finalize callback
//-----------------------------------------------------------

bool ipv4_get_default_src_ip(uint32_t dst_ipv4, uint32_t * psrc_ipv4) {
    struct sockaddr_in  addr, name;
    int                 sockfd;
    short               family  = AF_INET;
    socklen_t           addrlen = sizeof(struct sockaddr_in);

    if ((sockfd = socket(family, SOCK_DGRAM, 0)) == -1) {
        goto ERR_SOCKET;
    }

    memset(&addr, 0, addrlen);
    addr.sin_family = family;
    addr.sin_addr.s_addr = dst_ipv4;

    if (connect(sockfd, (struct sockaddr *) &addr, addrlen) == -1) {
        goto ERR_CONNECT; 
    }

    if (getsockname(sockfd, (struct sockaddr *) &name, &addrlen) == -1) {
        goto ERR_GETSOCKNAME;
    }

    close(sockfd);
    *psrc_ipv4 = name.sin_addr.s_addr;
    return true;

ERR_GETSOCKNAME:
    close(sockfd);
ERR_CONNECT:
ERR_SOCKET:
    return false;
}

/**
 * \brief Fill the unset parts of the IPv4 layer to coherent values 
 * \param ipv4_header The IP header that must be updated
 * \return true iif successful
 */

// TODO pass mask of the ipv4 layer
bool ipv4_finalize(uint8_t * ipv4_header)
{
	struct iphdr * iph = (struct iphdr *) ipv4_header;
    bool           do_update_src_ip = true, // TODO should be based on the mask to allow spoofing
                   ret = true;

	if (do_update_src_ip) {
        ret = ipv4_get_default_src_ip(iph->daddr, &iph->saddr);
	}

	return ret;
}

//-----------------------------------------------------------
// checksum and header_size callbacks
//-----------------------------------------------------------

/**
 * \brief Retrieve the size of an IPv4 header 
 * \param ipv4_header (unused) Address of an IPv4 header or NULL.
 * \return The size of an IPv4 header
 */

size_t ipv4_get_header_size(const uint8_t * ipv4_header) {
    uint8_t         ihl;
    size_t          size;

    if (ipv4_header) {
        bits_extract(ipv4_header, IPV4_OFFSET_IN_BITS_IHL, 4, &ihl);
        size  = 4 * ihl;
    } else {
        size = sizeof(struct iphdr);
    }

    return size; 
}

/**
 * \brief Compute and write the checksum related to an IP header
 *    according to its other fields (including tot_len).
 * \param ipv4_header An initialized IPv4 header. Checksum may be
 *    not initialized, will be ignored, and overwritten
 * \param psh (Unused) You may pass NULL.
 * \return true iif successful 
 */

bool ipv4_write_checksum(uint8_t * ipv4_header, buffer_t * psh) {
	struct iphdr * iph = (struct iphdr *) ipv4_header;

    // The IPv4 checksum must be set to 0 before its calculation
    iph->check = 0;
    iph->check = csum((const uint16_t *) iph, sizeof(struct iphdr));
    return true;
}

//-----------------------------------------------------------
// IPv4 fields
//-----------------------------------------------------------

static protocol_field_t ipv4_fields[] = {
    {
#ifdef USE_BITS
        .key             = IPV4_FIELD_VERSION,
        .type            = TYPE_BITS,
        .offset          = IPV4_OFFSET_VERSION,
        .offset_in_bits  = IPV4_OFFSET_IN_BITS_VERSION,
        .size_in_bits    = 4,
//        .set             = ipv4_set_version,
//        .get             = ipv4_get_version
    }, {
        .key             = IPV4_FIELD_IHL,
        .type            = TYPE_BITS,
        .offset          = IPV4_OFFSET_IHL,
        .offset_in_bits  = IPV4_OFFSET_IN_BITS_IHL,
        .size_in_bits    = 4,
//        .set             = ipv4_set_ihl,
//        .get             = ipv4_get_ihl
    }, {
#endif // USE_BITS
        .key             = IPV4_FIELD_TOS,
        .type            = TYPE_UINT8,
        .offset          = offsetof(struct iphdr, tos),
    }, {
        .key             = IPV4_FIELD_LENGTH,
        .type            = TYPE_UINT16,
        .offset          = offsetof(struct iphdr, tot_len),
    }, {
        .key             = IPV4_FIELD_IDENTIFICATION,
        .type            = TYPE_UINT16,
        .offset          = offsetof(struct iphdr, id),
    }, {
        .key             = IPV4_FIELD_FRAGOFF,
        .type            = TYPE_UINT16,
        .offset          = offsetof(struct iphdr, frag_off),
    }, {
        .key             = IPV4_FIELD_TTL,
        .type            = TYPE_UINT8,
        .offset          = offsetof(struct iphdr, ttl),
    }, {
        .key             = IPV4_FIELD_PROTOCOL,
        .type            = TYPE_UINT8,
        .offset          = offsetof(struct iphdr, protocol),
    }, {
        .key             = IPV4_FIELD_CHECKSUM,
        .type            = TYPE_UINT16,
        .offset          = offsetof(struct iphdr, check),
    }, {
        .key             = IPV4_FIELD_SRC_IP,
        .type            = TYPE_IPV4,
        .offset          = offsetof(struct iphdr, saddr),
    }, {
        .key             = IPV4_FIELD_DST_IP,
        .type            = TYPE_IPV4,
        .offset          = offsetof(struct iphdr, daddr),
    },
    END_PROTOCOL_FIELDS
    // options if header length > 5 (not yet implemented)
};

//-----------------------------------------------------------
// Default IPv4 values
//-----------------------------------------------------------

static struct iphdr ipv4_default = {
    .version  = IPV4_DEFAULT_VERSION,
    .ihl      = IPV4_DEFAULT_IHL,
    .tos      = IPV4_DEFAULT_TOS,
    .tot_len  = IPV4_DEFAULT_LENGTH,
    .id       = IPV4_DEFAULT_IDENTIFICATION,
    .frag_off = IPV4_DEFAULT_FRAGOFF,
    .ttl      = IPV4_DEFAULT_TTL,
    .protocol = IPV4_DEFAULT_PROTOCOL,
    .check    = IPV4_DEFAULT_CHECKSUM,
    .saddr    = IPV4_DEFAULT_SRC_IP,
    .daddr    = IPV4_DEFAULT_DST_IP
};

/**
 * \brief Write the default IPv4 header
 * \param ipv4_header The address of an allocated buffer that will
 *    store the IPv4 header or NULL.
 * \return The size of the default header.
 */

size_t ipv4_write_default_header(uint8_t * ipv4_header) {
    size_t size = sizeof(struct iphdr);
    if (ipv4_header) memcpy(ipv4_header, &ipv4_default, size);
    return size;
}

/**
 * \brief Test whether a sequence of bytes seems to be an IPv4 packet
 * \param bytes The sequence of evaluated bytes.
 * \return true iif it seems to be an IPv4 packet.
 */

bool ipv4_instance_of(uint8_t * bytes) {
    uint8_t version;
    return bits_extract(bytes, IPV4_OFFSET_IN_BITS_VERSION, 4, &version)
        && version == IPV4_DEFAULT_VERSION; 
}

static protocol_t ipv4 = {
    .name                 = "ipv4",
    .protocol             = IPPROTO_IPIP, // XXX only IP over IP (encapsulation). Beware probe.c, icmpv4_get_next_protocol_id 
    .write_checksum       = ipv4_write_checksum,
    .create_pseudo_header = NULL,
    .fields               = ipv4_fields,
    .write_default_header = ipv4_write_default_header, // TODO generic
    .get_header_size      = ipv4_get_header_size,
    .finalize             = ipv4_finalize,
    .instance_of          = ipv4_instance_of,
    .get_next_protocol    = protocol_get_next_protocol,
};

PROTOCOL_REGISTER(ipv4);

#endif // USE_IPV4

