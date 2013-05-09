#include <stdbool.h>      // bool
#include <unistd.h>       // close()
#include <stdio.h>        // DEBUG
#include <stddef.h>       // offsetof()
#include <string.h>       // memcpy()
#include <arpa/inet.h>    // inet_pton()
#include <netinet/ip.h>   // iphdr
#include <netinet/in.h>   // IPPROTO_UDP, INET_ADDRSTRLEN

#include "../field.h"
#include "../protocol.h"

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
#define IPV4_DEFAULT_PROTOCOL        IPPROTO_UDP
#define IPV4_DEFAULT_CHECKSUM        0
#define IPV4_DEFAULT_SRC_IP          0            // See ipv4_get_default_src_ip()
#define IPV4_DEFAULT_DST_IP          0            // Must be set by the user (see network.c)

// The following offsets cannot be retrieved with offsetof() 
// Offsets are always aligned with the begining of the header and expressed in bytes.
#define IPV4_OFFSET_VERSION          0
#define IPV4_OFFSET_IHL              0

/**
 * \brief Translate an IPv4 set in a string field into a binary IP
 * \param field The field containing the IPv4 to set (string format)
 * \param ip The address where we write the translated IP 
 * \return true iif successful.
 */

static bool ipv4_field_to_ipv4(const field_t * field, void * ip) {
    bool ret = false;

    switch (field->type) {
        case TYPE_STRING:
            // TODO we could use address_ip_from_string(AF_INET, field->value.string, ip), see "address.h"
            ret = (inet_pton(AF_INET, field->value.string, ip) == 1);
            break;
        case TYPE_INT32:
            memcpy(ip, &(field->value.int32), sizeof(uint32_t));
            ret = true;
            break;
        default:
            break;
    }
    return ret;
}

/**
 * \brief Create a string field from an IPv4 address 
 * \param field The field containing the IP to set (string format)
 * \param ip The address where we write the translated IP 
 * \return true iif successful.
 */

static field_t * ipv4_create_ipv4_field(const char * field_name, const void * ip) {
    char      str_ip[INET_ADDRSTRLEN];
    field_t * field = NULL;

    if (inet_ntop(AF_INET, ip, str_ip, INET_ADDRSTRLEN)) {
        field = STR(field_name, str_ip);
    }
    return field;
}

//-----------------------------------------------------------
// set/get callbacks (accessors to IPv4 string and int4 fields) 
//-----------------------------------------------------------

// TODO layer functions should directly call ipv4_create_ipv4_field 
// and ipv4_field_to_ipv4, thus we could remove the 4 following
// functions. For uint4 fields, pass the address of the aligned byte,
// the setter and the getter must read/write the 4 appropriate bits.

/**
 * \brief Create the an int4 field containing the version of the IPv4 header
 * \param ipv4_header The queried IPv4 header. 
 * \return The corresponding field, NULL in case of failure.
 */

field_t * ipv4_get_version(const uint8_t * ipv4_header) {
    const uint8_t * byte = ipv4_header + IPV4_OFFSET_VERSION;
    uint8_t         int4 = *byte >> 4;
    return I4(IPV4_FIELD_VERSION, int4);
}

/**
 * \brief Update the version of an IPv4 header
 * \param field The string field containing the new source (resolved) IP
 * \return true iif successful
 */

bool ipv4_set_version(uint8_t * ipv4_header, const field_t * field) {
    uint8_t * byte = ipv4_header + IPV4_OFFSET_VERSION; 
    *byte = (*byte & 0x0f) | (field->value.int4 << 4);
    return true;
}

/**
 * \brief Create the an int4 field containing the version of the IP header
 * \param ipv4_header The queried IPv4 header. 
 * \return The corresponding field, NULL in case of failure.
 */

field_t * ipv4_get_ihl(const uint8_t * ipv4_header) {
    const uint8_t * byte = ipv4_header + IPV4_OFFSET_IHL; 
    uint8_t         int4 = *byte & 0x0f;
    return I4(IPV4_FIELD_VERSION, int4);
}

/**
 * \brief Update the Internet Header Length (IHL) of an IPv4 header.
 * \param field The string field containing the new source (resolved) IP.
 * \return true iif successful.
 */

bool ipv4_set_ihl(uint8_t * ipv4_header, const field_t * field) {
    uint8_t * byte = ipv4_header + IPV4_OFFSET_IHL; 
    *byte = (*byte & 0xf0) | field->value.int4;
    return true;
}

/**
 * \brief Create the a string field containing the source IP of an IPv4 header.
 * \param ipv4_header The queried IPv4 header. 
 * \return The corresponding field, NULL in case of failure.
 */

field_t * ipv4_get_src_ip(const uint8_t * ipv4_header) {
    return ipv4_create_ipv4_field(
        IPV4_FIELD_SRC_IP,
        ipv4_header + offsetof(struct iphdr, saddr)
    );
}

/**
 * \brief Update the source IP of an IPv4 header.
 * \param field The string field containing the new source (resolved) IP.
 * \return true iif successful.
 */

bool ipv4_set_src_ip(uint8_t * ipv4_header, const field_t * field) {
    return ipv4_field_to_ipv4(
        field,
        ipv4_header + offsetof(struct iphdr, saddr)
    );
}

/**
 * \brief Create the a string field containing the destination IP of an IPv4 header.
 * \param ipv4_header The queried IPv4 header.
 * \return The corresponding field, NULL in case of failure.
 */

field_t * ipv4_get_dst_ip(const uint8_t * ipv4_header) {
    return ipv4_create_ipv4_field(
        IPV4_FIELD_DST_IP,
        ipv4_header + offsetof(struct iphdr, daddr)
    );
}

/**
 * \brief Update the destination IP of an IPv4 header according to a field 
 * \param field The string field containing the new destination (resolved) IP
 * \return true iif successful
 */

bool ipv4_set_dst_ip(uint8_t * ipv4_header, const field_t * field) {
    return ipv4_field_to_ipv4(
        field,
        ipv4_header + offsetof(struct iphdr, daddr)
    );
}

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
    addr.sin_family      = family;
    addr.sin_addr.s_addr = dst_ipv4;

    if (connect(sockfd, (struct sockaddr *) &addr, addrlen) == -1) {
        goto ERR_CONNECT; 
    }

    if (getsockname(sockfd, (struct sockaddr *) &name, &addrlen) == -1) {
        goto ERR_GETSOCKNAME;
    }

    close(sockfd);
    *psrc_ipv4 = name.sin_addr.s_addr;

    printf("dst_ipv4 = %d.%d.%d.%d\n",
        ((uint8_t *) &dst_ipv4)[0],
        ((uint8_t *) &dst_ipv4)[1],
        ((uint8_t *) &dst_ipv4)[2],
        ((uint8_t *) &dst_ipv4)[3]
    );

    printf("src_ipv4 = %d.%d.%d.%d\n",
        ((uint8_t *) psrc_ipv4)[0],
        ((uint8_t *) psrc_ipv4)[1],
        ((uint8_t *) psrc_ipv4)[2],
        ((uint8_t *) psrc_ipv4)[3]
    );

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
        ret &= ipv4_get_default_src_ip(iph->daddr, &iph->saddr);
	}

	return ret;
}

//-----------------------------------------------------------
// checksum and header_size callbacks
//-----------------------------------------------------------

/**
 * \brief Retrieve the size of an UDP header 
 * \return The size of an UDP header
 */

// TODO Use ipv4_get_ihl() according to void * header, and return IPV4_DEFAULT_IHL if this pointer is NULL
size_t ipv4_get_header_size(void) {
    return sizeof(struct iphdr);
}

/**
 * \brief Compute and write the checksum related to an IP header
 *    according to its other fields (including tot_len).
 * \param ipv4_header An initialized IPv4 header. Checksum may be
 *    not initialized, will be ignored, and overwritten
 * \return true iif successful 
 */

bool ipv4_write_checksum(uint8_t * ipv4_header, buffer_t * psh) {
	struct iphdr * iph = (struct iphdr *) ipv4_header;
    iph->check = csum((const uint16_t *) iph, sizeof(struct iphdr));
    return true;
}

//-----------------------------------------------------------
// IPv4 fields
//-----------------------------------------------------------

static protocol_field_t ipv4_fields[] = {
    {
        .key      = IPV4_FIELD_VERSION,
        .type     = TYPE_INT4,
        .offset   = IPV4_OFFSET_VERSION,
// TODO .offset_bits = 0
        .set      = ipv4_set_version,
        .get      = ipv4_get_version
    }, {
        .key      = IPV4_FIELD_IHL,
        .type     = TYPE_INT4,
        .offset   = IPV4_OFFSET_IHL,
// TODO .offset_bits = 4
        .set      = ipv4_set_ihl,
        .get      = ipv4_get_ihl
    }, {
        .key      = IPV4_FIELD_TOS,
        .type     = TYPE_INT8,
        .offset   = offsetof(struct iphdr, tos),
    }, {
        .key      = IPV4_FIELD_LENGTH,
        .type     = TYPE_INT16,
        .offset   = offsetof(struct iphdr, tot_len),
    }, {
        .key      = IPV4_FIELD_IDENTIFICATION,
        .type     = TYPE_INT16,
        .offset   = offsetof(struct iphdr, id),
    }, {
        .key      = IPV4_FIELD_FRAGOFF,
        .type     = TYPE_INT16,
        .offset   = offsetof(struct iphdr, frag_off),
    }, {
        .key      = IPV4_FIELD_TTL,
        .type     = TYPE_INT8,
        .offset   = offsetof(struct iphdr, ttl),
    }, {
        .key      = IPV4_FIELD_PROTOCOL,
        .type     = TYPE_INT8,
        .offset   = offsetof(struct iphdr, protocol),
    }, {
        .key      = IPV4_FIELD_CHECKSUM,
        .type     = TYPE_INT16,
        .offset   = offsetof(struct iphdr, check),
    }, {
        .key      = IPV4_FIELD_SRC_IP,
        .type     = TYPE_INT32,
        .offset   = offsetof(struct iphdr, saddr),
        .set      = ipv4_set_src_ip,
        .get      = ipv4_get_src_ip,
    }, {
        .key      = IPV4_FIELD_DST_IP,
        .type     = TYPE_INT32,
        .offset   = offsetof(struct iphdr, daddr),
        .set      = ipv4_set_dst_ip,
        .get      = ipv4_get_dst_ip,
    },
    END_PROTOCOL_FIELDS
    // options if header length > 5 (not yet implemented)
};

/**
 * \brief Retrieve the number of fields in a UDP header
 * \return The number of fields
 */

size_t ipv4_get_num_fields(void) {
    return sizeof(ipv4_fields) / sizeof(protocol_field_t);
}

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
 * \brief Write the default UDP header
 * \param ipv4_header The address of an pre-allocated IPv4 header 
 */

void ipv4_write_default_header(uint8_t * ipv4_header) {
    memcpy(ipv4_header, &ipv4_default, sizeof(struct iphdr));
}



static protocol_t ipv4 = {
    .name                 = "ipv4",
    .protocol             = 4, // XXX only IP over IP (encapsulation) 
    .get_num_fields       = ipv4_get_num_fields,
    .write_checksum       = ipv4_write_checksum,
    .create_pseudo_header = NULL,
    .fields               = ipv4_fields,
    .header_len           = sizeof(struct iphdr),
    .write_default_header = ipv4_write_default_header, // TODO generic
  //.socket_type          = NULL,
    .get_header_size      = ipv4_get_header_size,
    .need_ext_checksum    = false,
    .finalize             = ipv4_finalize,
//    .instance_of          = ipv4_instance_of,
};

PROTOCOL_REGISTER(ipv4);
