#include "use.h"

#include <stdlib.h>           // malloc()
#include <string.h>           // memcpy()
#include <stdbool.h>          // bool
#include <errno.h>            // ERRNO, EINVAL
#include <stddef.h>           // offsetof()
#include <netinet/tcp.h>      // tcphdr
#include <netinet/in.h>       // IPPROTO_TCP == 6

#include "../field.h"         // field_t
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
#define TCP_DEFAULT_WINDOW_SIZE        2     // In [2, 65535]
#define TCP_DEFAULT_DATA_OFFSET        5     // Size of the TCP header in words (= 4 bytes). Must be greater or equal than 5.

// The following offsets cannot be retrieved with offsetof() so they are hardcoded 
// - 12th byte
#define TCP_OFFSET_DATA_OFFSET         12
#define TCP_OFFSET_IN_BITS_DATA_OFFSET 0
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
#define TCP_FIELD_RESERVED             "reserved" // 0 0 
#define TCP_FIELD_NS                   "ns"
#define TCP_FIELD_CWR                  "cwr"
#define TCP_FIELD_ECE                  "ece"
#define TCP_FIELD_URG                  "urg"
#define TCP_FIELD_ACK                  "ack"
#define TCP_FIELD_PSH                  "psh"
#define TCP_FIELD_RST                  "rst"
#define TCP_FIELD_SYN                  "syn"
#define TCP_FIELD_FIN                  "fin"
#define TCP_FIELD_WINDOW_SIZE          "window_size" // in [2, 65535]
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
 * TCP fields
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
    }, {
        .key             = TCP_FIELD_DATA_OFFSET,
        .type            = TYPE_BITS,
        .size_in_bits    = 4,
        .offset          = TCP_OFFSET_DATA_OFFSET,
        .offset_in_bits  = TCP_OFFSET_IN_BITS_DATA_OFFSET,
/*
    }, {
        .key          = TCP_FIELD_RESERVED,
        // 3 bits
        .offset       = offsetof(struct tcphdr, res1),
    }, {
        .key          = TCP_FIELD_URG,
        // 1 bits
        .offset       = TCP_OFFSET_MASK,
        .offset_bits  = TCP_OFFSET_IN_BITS_URG,
    }, {
        .key          = TCP_FIELD_ACK,
        // 1 bits
        .offset       = TCP_OFFSET_MASK, 
        .offset_bits  = TCP_OFFSET_IN_BITS_ACK,
    }, {
        .key          = TCP_FIELD_PSH,
        // 1 bits
        .offset       = TCP_OFFSET_MASK,
        .offset_bits  = TCP_OFFSET_IN_BITS_PSH,
    }, {
        .key          = TCP_FIELD_RST,
        // 1 bits
        .offset       = TCP_OFFSET_MASK, 
        .offset_bits  = TCP_OFFSET_IN_BITS_RST,
    }, {
        .key          = TCP_FIELD_SYN,
        // 1 bits
        .offset       = TCP_OFFSET_MASK,
        .offset_bits  = TCP_OFFSET_IN_BITS_SYN,
    }, {
        .key          = TCP_FIELD_FIN,
        // 1 bits
        .offset       = TCP_OFFSET_MASK, 
        .offset_bits  = TCP_OFFSET_IN_BITS_FIN,
*/
    }, {
        .key          = TCP_FIELD_WINDOW_SIZE,
        .type         = TYPE_UINT16,
        .offset       = offsetof(struct tcphdr, WINDOW_SIZE),
    }, {
        .key          = TCP_FIELD_CHECKSUM,
        .type         = TYPE_UINT16,
        .offset       = offsetof(struct tcphdr, CHECKSUM),
    }, {
        .key          = TCP_FIELD_URGENT_PTR,
        .type         = TYPE_UINT16,
        .offset       = offsetof(struct tcphdr, URGENT_PTR),
    },
    // options if data offset > 5
    END_PROTOCOL_FIELDS
};

/**
 * Default TCP values
 */

static struct tcphdr tcp_default = {
    .WINDOW_SIZE = TCP_DEFAULT_WINDOW_SIZE,
};

/**
 * \brief Retrieve the size of an TCP header 
 * \param tcp_header Address of an TCP header or NULL
 * \return The size of an TCP header
 */

size_t tcp_get_header_size(const uint8_t * tcp_header) {
    size_t    size_in_words;

    if (!layer_extract(tcp_header, TCP_FIELD_DATA_OFFSET , &size_in_words)) {
        goto ERR_LAYER_EXTRACT;
    }
    return 4 * size_in_words;

ERR_LAYER_EXTRACT:
    return 0;
}

/**
 * \brief Write the default TCP header
 * \param tcp_header The address of an allocated buffer that will
 *    store the TCP header or NULL.
 * \return The size of the default header.
 */

size_t tcp_write_default_header(uint8_t * tcp_header) {
    struct tcphdr * tcp_hdr = (struct tcphdr *) tcp_header;
    size_t          size    = 4 * TCP_DEFAULT_DATA_OFFSET; 
    
    if (tcp_header) {
        memset(tcp_header, 0, size);
        memcpy(tcp_header, &tcp_default, size);
        tcp_hdr->DATA_OFFSET = TCP_DEFAULT_DATA_OFFSET;
        tcp_hdr->SRC_PORT    = htons(TCP_DEFAULT_SRC_PORT);
        tcp_hdr->DST_PORT    = htons(TCP_DEFAULT_DST_PORT);
    }

    return size;
}

/**
 * \brief Compute and write the checksum related to an TCP header
 * \param tcp_header Points to the begining of the TCP header and its content.
 *    The TCP checksum stored in this header is updated by this function.
 * \param ip_psh The IP layer part of the pseudo header. This buffer should
 *    contain the content of an ipv4_pseudo_header_t or an ipv6_pseudo_header_t
 *    structure.
 * \sa http://www.networksorcery.com/enp/protocol/tcp.htm#Checksum
 * \return true if everything is fine, false otherwise  
 */

bool tcp_write_checksum(uint8_t * tcp_header, buffer_t * ip_psh)
{
    struct tcphdr * tcp_hdr  = (struct tcphdr *) tcp_header;
    size_t          size_ip  = buffer_get_size(ip_psh),
                    size_tcp = tcp_get_header_size(tcp_header) + ntohs(tcp_hdr->window),
                    size_psh = size_ip + size_tcp;
    uint8_t       * psh;

    // TCP checksum computation requires the IPv* header
    if (!ip_psh) {
        errno = EINVAL;
        return false;
    }

    // Allocate the buffer which will contains the pseudo header
    if (!(psh = malloc(size_psh))) {
        return false;
    }

    // Put the excerpt of the IP header into the pseudo header
    memcpy(psh, buffer_get_data(ip_psh), size_ip);

    // Put the TCP header and its content into the pseudo header
    memcpy(psh + size_ip, tcp_hdr, size_tcp);

    // Overrides the TCP checksum in psh with zeros
    memset(psh + size_ip + offsetof(struct tcphdr, check), 0, sizeof(uint16_t));

    // Compute the checksum
    tcp_hdr->check = csum((const uint16_t *) psh, size_psh);
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
