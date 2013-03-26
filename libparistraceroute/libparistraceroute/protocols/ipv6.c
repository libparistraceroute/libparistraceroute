#include <stdlib.h>
#include <stddef.h> // offsetof()
#include <string.h> // memcpy()
#include <unistd.h> // close()
#include <stdio.h>
#include <arpa/inet.h> // inet_pton()
#include <netinet/ip6.h>
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
// VErsion + TCL + 4 bits
#define IPV6_FIELD_FLOWLABEL_UPPER         "flow_upper"




/* Extension Headers */
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
#define IPV6_DEFAULT_NEXT_HEADER     17 // UDP
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



/*
 * TODO
 * it might be interesting to have a given format for interacting with the user,
 * and a size for writing into the packet.
 * ex. IP addresses string <-> bit representation
 * inet_htons?
 */

// Accessors

int ipv6_set_dst_ip(unsigned char *buffer, field_t *field){
    int res;
	struct ip6_hdr *ip6_hed;

    ip6_hed = (struct ip6_hdr *)buffer;

    res = inet_pton(AF_INET6, (char*)field->value.string, &ip6_hed->ip6_dst);
    if (res != 1){
    	printf("Error while setting destination address\n");
        return -1; // Error while setting destination address
    }
    return 0;
}


field_t *ipv6_get_dst_ip(unsigned char *buffer){
    char res[IPV6_STRSIZE];
	struct ip6_hdr *ip6_hed;

    ip6_hed = (struct ip6_hdr *)buffer;

    memset(res, 0, IPV6_STRSIZE);
    inet_ntop(AF_INET6, &ip6_hed->ip6_dst, res, IPV6_STRSIZE);

    return field_create_string(IPV6_FIELD_DST_IP, res);
}


int ipv6_set_src_ip(unsigned char *buffer, field_t *field){
    int res;
	struct ip6_hdr *ip6_hed;

    ip6_hed = (struct ip6_hdr *)buffer;
    res = inet_pton(AF_INET6, (char*)field->value.string, &ip6_hed->ip6_src);
    if (res != 1){
        return -1; // Error while setting source address
    }
    return 0;
}

field_t *ipv6_get_src_ip(unsigned char *buffer){
    char res[IPV6_STRSIZE];
	struct ip6_hdr *ip6_hed;

	ip6_hed = (struct ip6_hdr *)buffer;

    memset(res, 0, IPV6_STRSIZE);
    inet_ntop(AF_INET6, &ip6_hed->ip6_src, res, IPV6_STRSIZE);

    return field_create_string(IPV6_FIELD_SRC_IP, res);
}


/* IPv6 fields */
static protocol_field_t ipv6_fields[] = {

    {
//        .key      = IPV6_FIELD_VERSION,
//        .type     = TYPE_INT4,
//        .offset   = offsetof(struct ip6_hdr, ip6_ctlun.ip6_un2_vfc), // ip6_un1_flow contains 4 bits version, 8 bits tcl, 20 bits flow-ID
//    }, {
//        .key      = IPV6_FIELD_TCL,
//        .type     = TYPE_INT8,
//        .offset   = offsetof(struct ip6_hdr, ip6_un1_flow,) // Same as above 8 bits
//    }, {
        .key      = IPV6_FIELD_FLOWLABEL_LOWER,
        .type     = TYPE_INT16,
        .offset   = (offsetof(struct ip6_hdr,  ip6_ctlun.ip6_un1.ip6_un1_flow) +2), // Same as above 20 bits /reduce it to 16 for testing.
    }, {
        .key      = IPV6_FIELD_PAYLOADLENGTH,
        .type     = TYPE_INT16,
        .offset   = offsetof(struct ip6_hdr, ip6_ctlun.ip6_un1.ip6_un1_plen),
    }, {
        .key      = IPV6_FIELD_NEXT_HEADER,
        .type     = TYPE_INT8,
        .offset   = offsetof(struct ip6_hdr, ip6_ctlun.ip6_un1.ip6_un1_nxt),
    }, {
        .key      = IPV6_FIELD_HOPLIMIT,
        .type     = TYPE_INT8,
        .offset   = offsetof(struct ip6_hdr, ip6_ctlun.ip6_un1.ip6_un1_hlim),
    }, {
        .key      = IPV6_FIELD_PROTOCOL,
        .type     = TYPE_INT8,
        .offset   = offsetof(struct ip6_hdr, ip6_ctlun.ip6_un1.ip6_un1_nxt),
    }, {
        .key      = IPV6_FIELD_SRC_IP,
        .type     = TYPE_INT128,
        .offset   = offsetof(struct ip6_hdr, ip6_src),
        .set      = ipv6_set_src_ip,
        .get      = ipv6_get_src_ip,
    }, {
       .key      = IPV6_FIELD_DST_IP,
       .type     = TYPE_INT128,
       .offset   = offsetof(struct ip6_hdr, ip6_dst),
       .set      = ipv6_set_dst_ip,
       .get      = ipv6_get_dst_ip,
    },
    END_PROTOCOL_FIELDS
};


/* Default IPv6 values */
static struct ip6_hdr ipv6_default = {
// TODO make this work :) flowID mingle
//	.version		= IPV6_DEFAULT_VERSION,
//	.tcl			= IPV6_DEFAULT_TCL,
//	.flowlabel		= IPV6_DEFAULT_FLOWLABEL,
	.ip6_ctlun.ip6_un1.ip6_un1_flow	= 0x00000006, // Version = 6, TCL = 0, FLOW = 0 // Beware of Byteorder
	.ip6_ctlun.ip6_un1.ip6_un1_plen	= IPV6_DEFAULT_PAYLOADLENGTH,
	.ip6_ctlun.ip6_un1.ip6_un1_nxt	= IPV6_DEFAULT_NEXT_HEADER,
	.ip6_ctlun.ip6_un1.ip6_un1_hlim	= IPV6_DEFAULT_HOPLIMIT,
	.ip6_src						= IPV6_DEFAULT_SRC_IP,
	.ip6_dst						= IPV6_DEFAULT_DST_IP,
};


/**
 * \brief A set of actions to be done before sending the packet.
 */

int ipv6_finalize(unsigned char *buffer) {
    // destination address = force finding source
	// Need to reset IPversion
	struct ip6_hdr *ip6_hed = (struct ip6_hdr *)buffer;

	ip6_hed->ip6_ctlun.ip6_un2_vfc = (uint8_t) 0x60;

    /* Setting source address */

	int ip_src_notset = 0;
	int i;

	// Addup all parts of the field, if 0 no source has been specified
	for(i = 0; i < 8; i++){
		ip_src_notset += ip6_hed->ip6_src.__in6_u.__u6_addr16[i];
	}


	if(!ip_src_notset){
		int sock;
	    struct sockaddr_in6 addr, name;
	    int len = sizeof(struct sockaddr_in6);

	    sock = socket(AF_INET6, SOCK_DGRAM, 0);
	    if (sock < 0)
	    	return 0; // Cannot create datagram socket

	    memset(&addr, 0, len);

	    addr.sin6_family	= AF_INET6;
	    addr.sin6_addr		= ip6_hed->ip6_dst;
	    addr.sin6_port      = htons(32000); // XXX why 32000 ?

	    if (connect(sock,(struct sockaddr*)&addr,sizeof(struct sockaddr_in6)) < 0)
	    	return 0; // Cannot connect socket

	    if (getsockname(sock,(struct sockaddr*)&name,(socklen_t*)&len) < -1)
	    	return 0; // Cannot getsockname


		memcpy((char *)&ip6_hed->ip6_src, (char *)&name.sin6_addr, sizeof(struct in6_addr));


	    close(sock);


	}


	return 0;
}




/**
 * \brief Retrieve the size of an UDP header 
 * \return The size of an UDP header
 */

unsigned int ipv6_get_header_size(void)
{
    return sizeof(struct ip6_hdr);
}

/**
 * \brief Write the default UDP header
 * \param data The address of an allocated buffer that will store the header
 */

void ipv6_write_default_header(unsigned char *data)
{
    memcpy(data, &ipv6_default, sizeof(struct ip6_hdr));
}

/**
 * \brief Retrieve the number of fields in a UDP header
 * \return The number of fields
 */

unsigned int ipv6_get_num_fields(void)
{
    return sizeof(ipv6_fields) / sizeof(protocol_field_t);
}

/*bool ipv6_instance_of(unsigned char buffer)
{
   TYPE_INT4 version;
   
   version=;

   if ( version == 6 ){
      return true
   else
      return false;
   }
}*/

static protocol_t ipv6 = {
	.name				= "ipv6",
	.protocol			= 6,
        .get_num_fields     		= ipv6_get_num_fields,
	.write_checksum			= NULL, // IPv6 has no checksum, it depends on upper layers
	.create_pseudo_header 		= NULL, // What does this do?
	.fields				= ipv6_fields,
	.header_len			= sizeof(struct ip6_hdr), //Redundant? XXX12
	.write_default_header	        = ipv6_write_default_header, // TODO generic with ipv4
//	.socket_type			= NULL, // TODO WHY?
	.get_header_size		= ipv6_get_header_size, // Redundant? XXX12
	.need_ext_checksum		= false,
	.finalize			= ipv6_finalize,
//        .instance_of                    = ipv6_instance_of,
};

PROTOCOL_REGISTER(ipv6);
