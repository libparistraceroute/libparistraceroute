#include "use.h"

#include <stdlib.h>           // malloc()
#include <string.h>           // memcpy()
#include <stdbool.h>          // bool
#include <stdint.h>           // uint*_t, UINT16_MAX 
#include <errno.h>            // ERRNO, EINVAL
#include <stddef.h>           // offsetof()
#include <netinet/tcp.h>      // tcphdr
#include <netinet/in.h>       // IPPROTO_TCP == 6

#include "../protocol.h"      // csum
#include "../bits.h"

#ifdef USE_IPV4
#    include "ipv4_pseudo_header.h"
#endif

#ifdef USE_IPV6
#    include "ipv6_pseudo_header.h"
#endif

// Default values
#define TCP_DEFAULT_SRC_PORT           2222 
#define TCP_DEFAULT_DST_PORT           3333
#define TCP_DEFAULT_WINDOW_SIZE        5840  // In [2, 65535]
#define TCP_DEFAULT_DATA_OFFSET        5    // Size of the TCP header in words (= 4 bytes). Must be greater or equal than 5.
#define TCP_DEFAULT_CWR                0
#define TCP_DEFAULT_ECE                0
#define TCP_DEFAULT_URG                0
#define TCP_DEFAULT_ACK                0
#define TCP_DEFAULT_PSH                0
#define TCP_DEFAULT_RST                0
#define TCP_DEFAULT_SYN                0
#define TCP_DEFAULT_FIN                0

// The following offsets cannot be retrieved with offsetof() so they are hardcoded 
// - 12th byte
#define TCP_OFFSET_DATA_OFFSET         12
#define TCP_OFFSET_IN_BITS_DATA_OFFSET 0
#define TCP_OFFSET_IN_BITS_RESERVED    4
#define TCP_OFFSET_IN_BITS_NS          7

// - 13th byte
#define TCP_OFFSET_MASK                13
#define TCP_OFFSET_IN_BITS_CWR         0
#define TCP_OFFSET_IN_BITS_ECE         1
#define TCP_OFFSET_IN_BITS_URG         2
#define TCP_OFFSET_IN_BITS_ACK         3
#define TCP_OFFSET_IN_BITS_PSH         4
#define TCP_OFFSET_IN_BITS_RST         5
#define TCP_OFFSET_IN_BITS_SYN         6
#define TCP_OFFSET_IN_BITS_FIN         7

// Field names
#define TCP_FIELD_SRC_PORT             "src_port"
#define TCP_FIELD_DST_PORT             "dst_port"
#define TCP_FIELD_SEQ_NUM              "seq_num"
#define TCP_FIELD_ACK_NUM              "ack_num"
#define TCP_FIELD_DATA_OFFSET          "data_offset"
#define TCP_FIELD_RESERVED             "reserved"    // 000 
#define TCP_FIELD_NS                   "ns"          // Nonce
#define TCP_FIELD_CWR                  "cwr"         // Congestion Window Reduced
#define TCP_FIELD_ECE                  "ece"         // ECN Echo
#define TCP_FIELD_URG                  "urg"         // Urgent
#define TCP_FIELD_ACK                  "ack"         // Acknowledgment
#define TCP_FIELD_PSH                  "psh"         // Push
#define TCP_FIELD_RST                  "rst"         // Reset
#define TCP_FIELD_SYN                  "syn"         // Syn
#define TCP_FIELD_FIN                  "fin"         // Fin
#define TCP_FIELD_WINDOW_SIZE          "window_size" // in [2, 65535]
//#define TCP_FIELD_LENGTH               "length"      // This is a virtual field which is based on window size
#define TCP_FIELD_CHECKSUM             "checksum"
#define TCP_FIELD_URGENT_PTR           "urgent_pointer"
#define TCP_FIELD_OPTIONS              "options" // if data offset > 5, padded at the end with 0 if necessary

// BSD/Linux abstraction
#ifdef __FAVOR_BSD
#    define SRC_PORT    th_sport
#    define DST_PORT    th_dport
#    define SEQ_NUM     th_seq
#    define ACK_NUM     th_ack
#    define DATA_OFFSET th_off
#    define WINDOW_SIZE th_win
#    define CHECKSUM    th_sum
#    define URGENT_PTR  th_urp
#else
#    define SRC_PORT    source
#    define DST_PORT    dest
#    define SEQ_NUM     seq
#    define ACK_NUM     ack_seq
#    define DATA_OFFSET doff
#    define WINDOW_SIZE window
#    define CHECKSUM    check
#    define URGENT_PTR  urg_ptr 
#endif

/**
 * \brief Retrieve the size of an TCP header 
 * \param tcp_segment Address of an TCP header or NULL
 * \return The size of an TCP header
 */

size_t tcp_get_header_size(const uint8_t * tcp_segment) {
    return (tcp_segment[TCP_OFFSET_DATA_OFFSET] & 0xf0) >> 2;
}

/**
 * TCP fields
 * Note: TCP has no "length" field. The size of the payload is
 * deduced from the IP layer.
 */

static protocol_field_t tcp_fields[] = {
    {
        .key             = TCP_FIELD_SRC_PORT,
        .type            = TYPE_UINT16,
        .offset          = offsetof(struct tcphdr, SRC_PORT),
    }, {
        .key             = TCP_FIELD_DST_PORT,
        .type            = TYPE_UINT16,
        .offset          = offsetof(struct tcphdr, DST_PORT),
    }, {
        .key             = TCP_FIELD_SEQ_NUM,
        .type            = TYPE_UINT32,
        .offset          = offsetof(struct tcphdr, SEQ_NUM),
    }, {
        .key             = TCP_FIELD_ACK_NUM,
        .type            = TYPE_UINT32,
        .offset          = offsetof(struct tcphdr, ACK_NUM),
#ifdef USE_BITS
    // 12th byte (TCP_OFFSET_DATA_OFFSET)
    }, {
        .key             = TCP_FIELD_DATA_OFFSET,
        .type            = TYPE_BITS,
        .size_in_bits    = 4,
        .offset          = TCP_OFFSET_DATA_OFFSET,
        .offset_in_bits  = TCP_OFFSET_IN_BITS_DATA_OFFSET,
    }, {
        .key             = TCP_FIELD_RESERVED,
        .type            = TYPE_BITS,
        .size_in_bits    = 3,
        .offset          = TCP_OFFSET_DATA_OFFSET,
        .offset_in_bits  = TCP_OFFSET_IN_BITS_RESERVED, 
    }, {
        .key             = TCP_FIELD_NS,
        .type            = TYPE_BITS,
        .size_in_bits    = 1,
        .offset          = TCP_OFFSET_DATA_OFFSET,
        .offset_in_bits  = TCP_OFFSET_IN_BITS_NS, 
    // 13th byte (TCP_OFFSET_MASK)
    }, {
        .key             = TCP_FIELD_CWR,
        .type            = TYPE_BITS,
        .size_in_bits    = 1,
        .offset          = TCP_OFFSET_MASK,
        .offset_in_bits  = TCP_OFFSET_IN_BITS_CWR, 
    }, {
        .key             = TCP_FIELD_ECE,
        .type            = TYPE_BITS,
        .size_in_bits    = 1,
        .offset          = TCP_OFFSET_MASK,
        .offset_in_bits  = TCP_OFFSET_IN_BITS_ECE, 
    }, {
        .key             = TCP_FIELD_URG,
        .type            = TYPE_BITS,
        .size_in_bits    = 1,
        .offset          = TCP_OFFSET_MASK,
        .offset_in_bits  = TCP_OFFSET_IN_BITS_URG,
    }, {
        .key             = TCP_FIELD_ACK,
        .type            = TYPE_BITS,
        .size_in_bits    = 1,
        .offset          = TCP_OFFSET_MASK, 
        .offset_in_bits  = TCP_OFFSET_IN_BITS_ACK,
    }, {
        .key             = TCP_FIELD_PSH,
        .type            = TYPE_BITS,
        .size_in_bits    = 1,
        .offset          = TCP_OFFSET_MASK,
        .offset_in_bits  = TCP_OFFSET_IN_BITS_PSH,
    }, {
        .key             = TCP_FIELD_RST,
        .type            = TYPE_BITS,
        .size_in_bits    = 1,
        .offset          = TCP_OFFSET_MASK, 
        .offset_in_bits  = TCP_OFFSET_IN_BITS_RST,
    }, {
        .key             = TCP_FIELD_SYN,
        .type            = TYPE_BITS,
        .size_in_bits    = 1,
        .offset          = TCP_OFFSET_MASK,
        .offset_in_bits  = TCP_OFFSET_IN_BITS_SYN,
    }, {
        .key             = TCP_FIELD_FIN,
        .type            = TYPE_BITS,
        .size_in_bits    = 1,
        .offset          = TCP_OFFSET_MASK, 
        .offset_in_bits  = TCP_OFFSET_IN_BITS_FIN,
#endif
    }, {
        .key             = TCP_FIELD_WINDOW_SIZE,
        .type            = TYPE_UINT16,
        .offset          = offsetof(struct tcphdr, WINDOW_SIZE),
    }, {
        .key             = TCP_FIELD_CHECKSUM,
        .type            = TYPE_UINT16,
        .offset          = offsetof(struct tcphdr, CHECKSUM),
    }, {
        .key             = TCP_FIELD_URGENT_PTR,
        .type            = TYPE_UINT16,
        .offset          = offsetof(struct tcphdr, URGENT_PTR),
    },
    // options if data offset > 5
    END_PROTOCOL_FIELDS
};

uint8_t tcp_make_mask(
    uint8_t cwr,
    uint8_t ece,
    uint8_t urg,
    uint8_t ack,
    uint8_t psh,
    uint8_t rst,
    uint8_t syn,
    uint8_t fin
){
    return cwr << (7 - TCP_OFFSET_IN_BITS_CWR)
         | ece << (7 - TCP_OFFSET_IN_BITS_ECE)
         | urg << (7 - TCP_OFFSET_IN_BITS_URG)
         | ack << (7 - TCP_OFFSET_IN_BITS_ACK)
         | psh << (7 - TCP_OFFSET_IN_BITS_PSH)
         | rst << (7 - TCP_OFFSET_IN_BITS_RST)
         | syn << (7 - TCP_OFFSET_IN_BITS_SYN)
         | fin << (7 - TCP_OFFSET_IN_BITS_FIN);
}

/**
 * \brief Write the default TCP header
 * \param tcp_segment The address of an allocated buffer that will
 *    store the TCP header or NULL.
 * \return The size of the default header.
 */

size_t tcp_write_default_header(uint8_t * tcp_segment) {
    struct tcphdr * tcp_header = (struct tcphdr *) tcp_segment;
    size_t          size       = TCP_DEFAULT_DATA_OFFSET << 2;
    
    if (tcp_segment) {
        memset(tcp_segment, 0, size);
        tcp_header->DATA_OFFSET = TCP_DEFAULT_DATA_OFFSET;
        tcp_header->SRC_PORT    = htons(TCP_DEFAULT_SRC_PORT);
        tcp_header->DST_PORT    = htons(TCP_DEFAULT_DST_PORT);
        tcp_header->WINDOW_SIZE = htons(TCP_DEFAULT_WINDOW_SIZE);
        ((uint8_t *) tcp_header)[TCP_OFFSET_MASK] = tcp_make_mask(
            TCP_DEFAULT_CWR,
            TCP_DEFAULT_ECE,
            TCP_DEFAULT_URG,
            TCP_DEFAULT_ACK,
            TCP_DEFAULT_PSH,
            TCP_DEFAULT_RST,
            TCP_DEFAULT_SYN,
            TCP_DEFAULT_FIN
        );
    }

    return size;
}

/**
 * \brief Compute and write the checksum related to an TCP header
 * \param tcp_segment Points to the begining of the TCP header and its content.
 *    The TCP checksum stored in this header is updated by this function.
 * \param ip_psh The IP layer part of the pseudo header. This buffer should
 *    contain the content of an ipv4_pseudo_header_t or an ipv6_pseudo_header_t
 *    structure.
 * \sa http://www.networksorcery.com/enp/protocol/tcp.htm#Checksum
 * \return true if everything is fine, false otherwise  
 */

bool tcp_write_checksum(uint8_t * tcp_segment, buffer_t * ip_psh)
{
    struct tcphdr * tcp_header = (struct tcphdr *) tcp_segment;
    size_t          size_ip    = buffer_get_size(ip_psh),
                    size_tcp   = tcp_get_header_size(tcp_segment) + 2, // hardcoded payload size 
                    size_psh   = size_ip + size_tcp;
    uint8_t       * psh;

    // TCP checksum computation requires the IPv* header
    if (!ip_psh) {
        errno = EINVAL;
        return false;
    }

    // Allocate the buffer which will contains the pseudo header
    if (!(psh = calloc(1, size_psh))) {
        return false;
    }

    // Put the excerpt of the IP header into the pseudo header
    memcpy(psh, buffer_get_data(ip_psh), size_ip);

    // Put the TCP header and its content into the pseudo header
    memcpy(psh + size_ip, tcp_segment, size_tcp);

    // Overrides the TCP checksum in psh with zeros
    memset(psh + size_ip + offsetof(struct tcphdr, check), 0, sizeof(uint16_t));

    // Compute the checksum
    tcp_header->check = csum((const uint16_t *) psh, size_psh);
    free(psh);
    return true;
}

buffer_t * tcp_create_pseudo_header(const uint8_t * ip_segment)
{
    buffer_t * buffer = NULL;

    // TODO Duplicated from packet.c (see packet_guess_address_family)
    // TODO we should use instanceof
    switch (ip_segment[0] >> 4) {
        case 4:
            buffer = ipv4_pseudo_header_create(ip_segment);
            break;
        case 6:
            buffer = ipv6_pseudo_header_create(ip_segment);
            break;
        default:
            break;
    }

    return buffer;
}

static protocol_t tcp = {
    .name                 = "tcp",
    .protocol             = IPPROTO_TCP, 
    .write_checksum       = tcp_write_checksum,
    .create_pseudo_header = tcp_create_pseudo_header,
    .fields               = tcp_fields,
  //.defaults             = tcp_defaults,             // XXX used when generic
    .write_default_header = tcp_write_default_header, // TODO generic
  //.socket_type          = NULL,
    .get_header_size      = tcp_get_header_size,
};

PROTOCOL_REGISTER(tcp);
