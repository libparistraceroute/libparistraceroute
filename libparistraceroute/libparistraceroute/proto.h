#ifndef H_PATR_PROTO
#define H_PATR_PROTO

#include "field.h"
#include "fieldlist.h"
#include <stddef.h>



 /** enumeration of the protocols known by our library */

typedef struct proto_struct{
	/** next struct */struct proto_struct *next;
	/** type of proto*/char* name;
	/** size of proto name, for cmp*/ int name_size;
	/** proto code*/ int proto_code; //like IP_PROTOTCP, or AF_INET
	/** the fields possible for this proto */ char** fields;
	/** the number of fields for this proto */ int number_fields;
	/** the fields mandatory for this proto */ char** mandatory_fields;
	/** the number of mandatory fields for this proto */ int number_mandatory_fields;
	///** initiate the socket */ void* (*init_socket)(void);//???
	/** if needs ext header for checksum*/ short need_ext_checksum;
	/** fonction to set a header field to a selected value)*/ int (*set_packet_field)(void* value, char* name,char** datagram);
	/** fonction to get the value of a selected field -> return value is malloc'ed during the process*/ void* (*get_packet_field)(char* name,char** datagram);
	/** set some values to default (has to be called)*/void (*set_default_values)(char** datagram);
	/** fonction to guess if a packet is encapsuled by this protocol*/ int (*check_datagram)(char** datagram, int packet_size);
	/** fonction to initiate packet (pointer to header correct part)*/ int (*fill_packet_header)(fieldlist_s * fieldlist, char** datagram);
	/** fonction to realise computes or whatever after filling packet*/ int (*after_filling)(char** datagram);
	/** calculate the estimated header size from fields options and default (in bytes)  */int (*get_header_size)(char** packet, fieldlist_s* fieldlist);
	/** calculates a pseudo header depending on proto */ int (*create_pseudo_header)(char** datagram, char* proto, unsigned char** pseudo_header, int* size);
	/** calculates the checksum, with a pseudo header if needed*/unsigned short (*compute_checksum)(char** packet, int packet_size, unsigned char** pseudo_header, int size);
	/** fonction to realise computes or whatever after checksum */ int (*after_checksum)(char** datagram);
	/** create adapted socket to protocol*/ int (*generate_socket)(int type_afinet); //not implemented yet
} proto_s;

/**
 * get the struct corresponding to the chosen algorithm
 * @param the protocol
 * @return proto_s the pointer to the struct
 */

const proto_s* choose_protocol(char* proto);

/**
 * get a print of the availables protocols
 * @param void
 * @return void
 */

void view_available_protocols();

/**
 * add the protocol algorithm functions to the list
 * @param ops the actual algorithms generic functions
 * @return void
 */
void add_proto_list (proto_s *ops);

/**
 * function to calc checksums (my be multi protocol)
 * @param len the size of header in bytes 
 * @param buff the buffer
 * @return the checksum
 */
unsigned short csum (unsigned short *buf, int nwords);

/**
 * function to estimate if a datagram is encapsulated by a certain protocol
 * @param datagram the packet to estimate
 * @param data_size the size of datagram
 * @param protocol the name of the expected protocol
 * @return 0 if false, 1 if true
 */
int guess_datagram_protocol(char* datagram, int data_size, char* protocol);

/**
 * function to get the size of a certain protocol header
 * @param datagram the packet to read it from (may be null -> guessing)
 * @param protocol the name of the expected protocol
 * @return the header size
 */
int proto_get_header_size(char* datagram, char* protocol);

/**
 * function to get the size of a certain protocol header
 * @param datagram the packet to read it from (may be null -> guessing)
 * @param protocol the name of the expected protocol
 * @return the header size
 */
void* proto_get_field(char* datagram, char* protocol, char* name);

/**
 * function to see the fields and mandatory fields to fill a protocol paquet header
 * @param protocol the name of the expected protocol
 * @return void
 */
void proto_view_fields(char* protocol);

//unsigned short get_checksum(char* datagram, char* protocol);
//TODO : les free pour la liste chainee

#define ADD_PROTO(MOD)	\
static void __init_ ## MOD (void) __attribute__ ((constructor));	\
static void __init_ ## MOD (void) {	\
				\
	add_proto_list (&MOD);	\
}

#endif
