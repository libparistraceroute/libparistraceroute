#ifndef H_PATR_PACKET
#define H_PATR_PACKET


#include <netinet/icmp6.h>
#include <netinet/ip_icmp.h>
#include <netinet/in.h>
#include <netinet/ip6.h>
#include <netinet/ip.h>

#include"field.h"
#include"proto.h"

/*
typedef struct iphdr_struct
  {
#if __BYTE_ORDER == __LITTLE_ENDIAN
    unsigned int ihl:4; //header lenght
    unsigned int version:4;
#elif __BYTE_ORDER == __BIG_ENDIAN
    unsigned int version:4;
    unsigned int ihl:4;
#else
# error	"Please fix <bits/endian.h>"
#endif
    u_int8_t tos;
    u_int16_t tot_len;
    u_int16_t id;
    u_int16_t frag_off;
    u_int8_t ttl;
    u_int8_t protocol;
    u_int16_t check;
    u_int32_t saddr;
    u_int32_t daddr;
    //The options start here.
  }iphdr_s;


struct timestamp
  {
    u_int8_t len;
    u_int8_t ptr;
#if __BYTE_ORDER == __LITTLE_ENDIAN
    unsigned int flags:4;
    unsigned int overflow:4;
#elif __BYTE_ORDER == __BIG_ENDIAN
    unsigned int overflow:4;
    unsigned int flags:4;
#else
# error	"Please fix <bits/endian.h>"
#endif
    u_int32_t data[9];
  };

*/



typedef struct packet_struct{
	/**pointer to the beginning of packet*/ char** data;
	/** size of packet */ int size;
	/** offset of protocol i (i e beginning of it) does not include payload offset */ int* offsets;
	/** payload size */ int payload_size;
	/** socket in which send the packet*/ int socket;
	/** the fields for the packet*/ field_s fields;
	/** list of protocols to put in packet*/ char** protos;
	/** number of protos to add in packet*/ int nb_protos;
} packet_s;

/**
 * create a packet from various data
 * @param size : the size of the packet to create
 * @param fields the different data to put in the fields of header
 * @return the packet pointer
 */

packet_s* create_packet(int payload_size, char** protos, int nb_protos);

/**
 * fill the packet headers, ... depending on fields
 * @param packet : the packet to fill
 * @param fields : the fields containing data, for example the payload content
 * @param checksums : an array that will receive the checksums computed (for probe). optional if not using probes
 * @return int indicates success or error
 */

int fill_packet(packet_s* packet, fieldlist_s* fields, unsigned short* checksums);

/**
 * show the content of a packet
 * @param packet : the packet to view
 * @return void
 */
void view_packet(packet_s* packet);

/**
 * free the space allocated to a packet
 * @param packet : the packet to free
 * @return void
 */
void free_packet(packet_s* packet);

/**
 * exchange the init of payload (size of short) and the i-th protocol checksum
 * useful for MDA and paris tracertoute principle of recognition
 * @param proto_exch : the number of protocol (in list of protocols of packet) to which switch checksum
 * @return int 1 if success -1 otherwise
 */
int packet_exchange_checksum_and_payload( packet_s* packet, int proto_exch);

/**
 * from traceroute, resolves the char adress IP into usable address
 * @param name the char address
 * @param addr the struct to receive the effective address
 * @return int value of error or success(0)
 */
/* 
static int getaddr (const char *name, sockaddr_u *addr);*/

#endif
