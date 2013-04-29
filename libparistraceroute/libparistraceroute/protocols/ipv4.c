#include <stdlib.h>
#include <stddef.h> // offsetof()
#include <string.h> // memcpy()
#include <unistd.h> // close()
#include <stdio.h>
#include <arpa/inet.h> // inet_pton()
//#include <netdb.h>
#include <netinet/ip.h>

#include "../field.h"
//#include "../protocol_field.h"
#include "../protocol.h"

/* Field names */
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

/* Default field values */
#define IPV4_DEFAULT_VERSION         4 
#define IPV4_DEFAULT_IHL             5
#define IPV4_DEFAULT_TOS             0
#define IPV4_DEFAULT_LENGTH          0
#define IPV4_DEFAULT_IDENTIFICATION  1
#define IPV4_DEFAULT_FRAGOFF         0
#define IPV4_DEFAULT_TTL             255 
#define IPV4_DEFAULT_PROTOCOL        17   // here UDP, http://en.wikipedia.org/wiki/List_of_IP_protocol_numbers 
#define IPV4_DEFAULT_CHECKSUM        0
#define IPV4_DEFAULT_SRC_IP          0
#define IPV4_DEFAULT_DST_IP          0

#define IPV4_STRSIZE 16

/*
 * TODO
 * it might be interesting to have a given format for interacting with the user,
 * and a size for writing into the packet.
 * ex. IP addresses string <-> bit representation
 */

// Accessors

int ipv4_set_dst_ip(unsigned char *buffer, field_t *field)
{
    int res;
	struct iphdr *ip_hed;
    
    ip_hed = (struct iphdr *)buffer;
    res = inet_pton(AF_INET, (char*)field->value.string, &ip_hed->daddr);
    if (res != 1)
        return -1; // Error while reading destination address
    return 0;
}


field_t *ipv4_get_dst_ip(unsigned char *buffer)
{
    char res[IPV4_STRSIZE];
	struct iphdr *ip_hed;
    
    ip_hed = (struct iphdr *)buffer;

    memset(res, 0, IPV4_STRSIZE);
    inet_ntop(AF_INET, &ip_hed->daddr, res, IPV4_STRSIZE);

    return field_create_string(IPV4_FIELD_DST_IP, res);
}

int ipv4_set_src_ip(unsigned char *buffer, field_t *field)
{
    int res;
	struct iphdr *ip_hed;
    
    ip_hed = (struct iphdr *)buffer;
    res = inet_pton(AF_INET, (char*)field->value.string, &ip_hed->saddr);
    if (res != 1)
        return -1; // Error while reading destination address
    return 0;
}


field_t *ipv4_get_src_ip(unsigned char *buffer)
{
    char res[IPV4_STRSIZE];
	struct iphdr *ip_hed;
    
    ip_hed = (struct iphdr *)buffer;

    memset(res, 0, IPV4_STRSIZE);
    inet_ntop(AF_INET, &ip_hed->saddr, res, IPV4_STRSIZE);

    return field_create_string(IPV4_FIELD_SRC_IP, res);
}

/* IPv4 fields */
static protocol_field_t ipv4_fields[] = {
//    {
//        .key    = IPV4_FIELD_VERSION,
//        .type   = TYPE_INT4,
//        .offset = offsetof(struct iphdr, version),
//    }, {
//        .key    = "ihl",
//        .type   = TYPE_INT4,
//        .offset = offsetof(struct iphdr, ihl),
//    },
    {
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

/* Default IPv4 values */
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
 * \brief Determine source address when it is not explicitely specified.
 * \param dip The destination IP used to compute the source IP
 * \return The source IP that will be used to send datagrams to the specified
 * destination
 *
 * The source address is dependent on the destination, and corresponds to the
 * interface that will be chosen. This is mainly useful for RAW sockets. We
 * perform a connect operation for this.
 *
 * TODO we can generalize this to other transport protocols ?
 */
uint32_t ipv4_get_default_sip(uint32_t dip) {
        int sock;
        struct sockaddr_in addr, name;
        int len = sizeof(struct sockaddr_in);

        sock = socket(AF_INET, SOCK_DGRAM, 0);
        if (sock < 0)
            return 0; // Cannot create datagram socket
        
        memset(&addr, 0, len);

        addr.sin_family      = AF_INET;
        addr.sin_addr.s_addr = dip;
        addr.sin_port        = htons(32000); // XXX why 32000 ?

        if (connect(sock,(struct sockaddr*)&addr,sizeof(struct sockaddr_in)) < 0)
            return 0; // Cannot connect socket

        if (getsockname(sock,(struct sockaddr*)&name,(socklen_t*)&len) < -1)
            return 0; // Cannot getsockname
        
        close(sock);

        return name.sin_addr.s_addr;
}

/**
 * \brief A set of actions to be done before sending the packet.
 */
int ipv4_finalize(unsigned char *buffer) {
    // destination address = force finding source

    /* Setting source address */
	struct iphdr *ip_hed = (struct iphdr *)buffer;
	if (ip_hed->saddr==0) {//has to be calculated 
		ip_hed->saddr = ipv4_get_default_sip(ip_hed->daddr);
	}

	return 0;
}

/**
 * \brief Retrieve the number of fields in a UDP header
 * \return The number of fields
 */

unsigned int ipv4_get_num_fields(void)
{
    return sizeof(ipv4_fields) / sizeof(protocol_field_t);
}

/**
 * \brief Retrieve the size of an UDP header 
 * \return The size of an UDP header
 */

unsigned int ipv4_get_header_size(void)
{
    return sizeof(struct iphdr);
}

/**
 * \brief Write the default UDP header
 * \param data The address of an allocated buffer that will store the header
 */

void ipv4_write_default_header(unsigned char *data)
{
    memcpy(data, &ipv4_default, sizeof(struct iphdr));
}

/**
 * \brief Compute and write the checksum related to an IP header
 *   according to its other fields (including tot_len).
 * \param ipv4_hdr A pre-allocated IPv4 header filled.
 * \sa http://www.networksorcery.com/enp/protocol/udp.htm#Checksum
 * \return 0 if everything is ok, another value otherwise 
 */

bool ipv4_write_checksum(unsigned char *buf, buffer_t * psh) {
	struct iphdr * iph = (struct iphdr *) buf;
    iph->check = csum((const uint16_t *) iph, sizeof(struct iphdr));
    return true;
}

//bool ipv4_instance_of(unsigned char buffer)
//{
  // TYPE_INT4 version;
   
  // version=;

   //if ( version == 4 ){
     // return true
  // else
    //  return false;
   //}
//}


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
