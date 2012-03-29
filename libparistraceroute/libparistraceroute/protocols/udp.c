#include <stdlib.h>
#include <stdio.h>
#include <string.h>      // memcpy()
#include <stdbool.h>
#include <errno.h>       // ERRNO, EINVAL
#include <stddef.h>      // offsetof()
#include <netinet/udp.h>
#include <arpa/inet.h>

#include "../protocol.h"

#define UDP_DEFAULT_SRC_PORT 2828
#define UDP_DEFAULT_DST_PORT 2828

// XXX mandatory fields ?
// XXX UDP parsing missing
#ifdef __FAVOR_BSD
#   define SRC_PORT uh_sport
#   define DST_PORT uh_dport
#   define LENGTH   uh_ulen
#   define CHECKSUM uh_sum
#else
#   define SRC_PORT source 
#   define DST_PORT dest 
#   define LENGTH   len
#   define CHECKSUM check 
#endif

/* UDP fields */
static protocol_field_t udp_fields[] = {
    {
        .key = "src_port",
        .type = TYPE_INT16,
        .offset = offsetof(struct udphdr, SRC_PORT),
    }, {
        .key = "dst_port",
        .type = TYPE_INT16,
        .offset = offsetof(struct udphdr, DST_PORT),
    }, {
        .key = "length",
        .type = TYPE_INT16,
        .offset = offsetof(struct udphdr, LENGTH),
    }, {
        .key = "checksum",
        .type = TYPE_INT16,
        .offset = offsetof(struct udphdr, CHECKSUM),
    }
};

/* Default UDP values */
static struct udphdr udp_default = {
    .SRC_PORT = UDP_DEFAULT_SRC_PORT,
    .DST_PORT = UDP_DEFAULT_DST_PORT,
    .LENGTH   = 0,
    .CHECKSUM = 0
};

/**
 * \brief Retrieve the number of fields in a UDP header
 * \return The number of fields
 */

unsigned int udp_get_num_fields(void)
{
    return sizeof(udp_fields) / sizeof(protocol_field_t);
}

/**
 * \brief Retrieve the size of an UDP header 
 * \return The size of an UDP header
 */

unsigned int udp_get_header_size(void)
{
    return sizeof(struct udphdr);
}

/**
 * \brief Write the default UDP header
 * \param data The address of an allocated buffer that will store the header
 */

void udp_write_default_header(unsigned char *data)
{
    memcpy(data, &udp_default, sizeof(struct udphdr));
}

/**
 * \brief Compute and write the checksum related to an UDP header
 * \param udp_hdr A pre-allocated UDP header
 * \param pseudo_hdr The pseudo header 
 * \sa http://www.networksorcery.com/enp/protocol/udp.htm#Checksum
 * \return 0 if everything is ok, EINVAL if pseudo_hdr is invalid,
 *    ENOMEM if a memory error arises
 */

int udp_write_checksum(struct udphdr * udp_hdr, pseudoheader_t * pseudo_hdr)
{
    unsigned char * tmp;
    unsigned int len;
    unsigned short res;

    if (!pseudo_hdr) return EINVAL; // pseudo header required

    len = sizeof(struct udphdr) + pseudo_hdr->size;
    tmp = malloc(len * sizeof(unsigned char));
    if(!tmp) return ENOMEM;

    memcpy(tmp, pseudo_hdr->data, pseudo_hdr->size);
    memcpy(tmp + pseudo_hdr->size, udp_hdr, sizeof(struct udphdr));
    res = csum(*(unsigned short**) &tmp, (len >> 1));
    udp_hdr->check = res;

    free(tmp);
    return 0;
}

/**
 * \brief Callback that indicates whether this protocol 
 *   needs a pseudo header in order to compute its checksum.
 * \sa ../protocol.h
 * \return true iif we use a dedicated checksum function.
 */

bool udp_need_ext_checksum() {
    return true;
}

static protocol_t udp = {
    .name                 = "udp",
    .get_num_fields       = udp_get_num_fields,
    .write_checksum       = CAST_WRITE_CHECKSUM udp_write_checksum,
  //.create_pseudo_header = NULL,
    .fields               = udp_fields,
  //.defaults             = udp_defaults,             // XXX used when generic
    .header_len           = sizeof(struct udphdr),
    .write_default_header = udp_write_default_header, // TODO generic
  //.socket_type          = NULL,
    .get_header_size      = udp_get_header_size,
    .need_ext_checksum    = udp_need_ext_checksum
};

PROTOCOL_REGISTER(udp);

// END


/*
#define NUMBER_FIELD_UDP 5
#define NUMBER_MAND_FIELD_UDP 3
#define HEADER_SIZE_UDP 8
//#define IPPROTO_UDP 17
*/
/*
char* udp_fields[NUMBER_FIELD_UDP] = {"src_port","dest_port","lenght","checksum","with_tag"};
char* udp_mandatory_fields[NUMBER_MAND_FIELD_UDP]={"src_port","dest_port","lenght"};

static void set_default_values_udp(char** datagram){
	struct udphdr *udp_hed = (struct udphdr *)  *datagram;
	udp_hed->check=0; // zero now, calcule it later
}
*/
/*
static int set_packet_field_udp(void* value, char* name, char** datagram){//not directly a casted header to permit genericity
	if(value==NULL){
		#ifdef DEBUG
		fprintf(stderr,"set_packet_field_udp : trying to set NULL value for %s \n",name);
		#endif
		return -1;
	}
	struct udphdr *udp_hed = (struct udphdr *)  *datagram;
	if(strncasecmp(name,"dest_port",9)==0){
		#ifdef __FAVOR_BSD
		udp_hed->uh_dport=htons(*(( u_int16_t*)value));
		#else
		udp_hed->dest=htons(*(( u_int16_t*)value));
		#endif
		return 1; //ok
	}
	if(strncasecmp(name,"src_port",8)==0){
		#ifdef __FAVOR_BSD
		udp_hed->uh_sport=htons(*(( u_int16_t*)value));
		#else
		udp_hed->source=htons(*(( u_int16_t*)value));
		#endif
		return 1;
	}
	if(strncasecmp(name,"lenght",12)==0){
		#ifdef __FAVOR_BSD
		udp_hed->uh_ulen=htons(*(( u_int16_t*)value));
		#else
		udp_hed->len=htons(*(( u_int16_t*)value));
		#endif
		return 1;
	}
	if(strncasecmp(name,"checksum",8)==0){
		#ifdef __FAVOR_BSD
		if(udp_hed->uh_sum!=0){
			fprintf(stderr,"set_packet_field_udp : old checksum not null, probably overwriting checksum or tag data");
		}
		udp_hed->uh_sum=htons(*(( u_int16_t*)value));
		#else
		if(udp_hed->check!=0){
			fprintf(stderr,"set_packet_field_udp : old checksum not null, probably overwriting checksum or tag data");
		}
		udp_hed->check=htons(*(( u_int16_t*)value));
		#endif
		return 1;
	}
	else{
		return 0;
	}

}
*/

/**
 * \brief Allocate and set a memory block with the value of a field stored in the UDP header
 * \param name Key related to the field (for example "src_port")
 * \param datagram
 * \return The adress of the value, NULL otherwise
 */
void * get_packet_field_udp(char * name, char ** datagram){
	if(datagram == NULL || *datagram == NULL){
		#ifdef DEBUG
		fprintf(stderr, "get_packet_field_udp: trying to read NULL packet\n");
		#endif
		return NULL;
	}

	struct udphdr * udp_hdr = (struct udphdr *) * datagram;
	if(strncasecmp(name, "dst_port", 8) == 0) {
		u_int16_t * res = malloc(sizeof(u_int16_t));
        if(!res) return NULL;
		*res = ntohs(udp_hdr->DST_PORT);
		return res;
	} else if(strncasecmp(name,"src_port", 8) == 0) {
		u_int16_t * res = malloc(sizeof(u_int16_t));
        if(!res) return NULL;
		*res = ntohs(udp_hdr->SRC_PORT);
		return res;
	} else if(strncasecmp(name,"lenght", 6) == 0) {
		u_int16_t * res = malloc(sizeof(u_int16_t));
        if(!res) return NULL;
		*res = ntohs(udp_hdr->LENGTH);
		return res;
	} else if(strncasecmp(name,"checksum", 8) == 0) {
		u_int16_t * res = malloc(sizeof(u_int16_t));
        if(!res) return NULL;
		*res = ntohs(udp_hdr->CHECKSUM);
		return res;
	} else {
		#ifdef DEBUG
		fprintf(stderr, "get_packet_field_udp : unknown option for udp : %s\n", name);
		#endif
		return NULL;
	}
}

/*
int fill_packet_header_udp(fieldlist_s * fieldlist, char** datagram){
	//check if fields mandatory present
	void* val = NULL;
	if(check_presence_list_of_fields(udp_mandatory_fields,NUMBER_MAND_FIELD_UDP,fieldlist)==-1){
		fprintf(stderr, "fill_packet_header_udp : missing fields for udp header \n");
		return -1;
	}
	set_default_values_udp(datagram);
	int i=0;
	for(i=0; i < NUMBER_FIELD_UDP; i++){
		
		val = get_field_value_from_fieldlist(fieldlist, udp_fields[i]);
		set_packet_field_udp(val, udp_fields[i] , datagram);
	}
	return 0;
}
*/

/*
field_s* create_packet_fields_udp(){
	int i=0;
	field_s* next_el=NULL;
	for(i=0;i<NUMBER_FIELD_UDP;i++){
		field_s* field = malloc(sizeof(field_s));
		field->name = udp_fields[i];
		field->next =next_el;
		next_el=field;
	}
	return next_el;
}*/
/*
static int generate_socket_udp(int type_af){
	//TODO
	return 0;
}

int get_header_size_udp(char** packet, fieldlist_s* fieldlist){
	int size=HEADER_SIZE_UDP; //2* 32-byte words, no possible options
	return size;
}

int create_pseudo_header_udp(char** datagram, char* proto, unsigned char** pseudo_header, int* size){
	#ifdef DEBUG
	fprintf(stderr,"create_pseudo_header_udp : udp has no pseudo_header found for protocol %s. parameters left untouched.\n",proto);
	#endif
	return -1;
}
*/

/*
unsigned short compute_checksum_udp(char** packet, int packet_size, unsigned char** pseudo_header, int size){
	//TODO : difference entre erreur et checksum
	if(size==0||pseudo_header==NULL){
		#ifdef DEBUG
		fprintf(stderr,"compute_checksum_udp : passing a NULL pseudo header when udp needs one. end\n");
		#endif
		return 0;
	}
	unsigned char * total_header=malloc((packet_size+size)*sizeof(char));
	memset(total_header, 0, packet_size+size);
	memcpy(total_header,*pseudo_header,size);//udp checksum n all the packet
	memcpy(total_header+size,*packet,packet_size);
	#ifdef DEBUG
	int i =0;
	fprintf(stdout,"compute_checksum_udp : pseudo header for udp checksum : \n");
	for(i=0;i<size+packet_size;i++){
		fprintf(stdout," %02x ",total_header[i]);
	}
	fprintf(stdout,"\n");
	#endif
	//unsigned short res = sum_calc((unsigned short) size+packet_size, (unsigned short**)&total_header);
	unsigned short res = csum(*(unsigned short**)&total_header, ((size+packet_size) >> 1));
	#ifdef DEBUG
	fprintf(stdout,"compute_checksum_udp : value %d found for udp checksum (%04x) \n",res, res);
	#endif
	struct udphdr *udp_hed = (struct udphdr *)  *packet;
	udp_hed->check=res;
	return res;
}

int check_datagram_udp(char** datagram, int packet_size){
	if(datagram==NULL||*datagram==NULL){
		return 0;
	}
	char* data = *datagram;
	if(packet_size<HEADER_SIZE_UDP){
		return 0;
	}
	struct udphdr *udp_hed = (struct udphdr *) data;
	if(udp_hed->len!=htons(HEADER_SIZE_UDP)){
		return 0;
	}
	return 1;
}
*/
/*
unsigned short get_checksum_udp(char** datagram){
		if(datagram==NULL||*datagram==NULL){
		#ifdef DEBUG
		fprintf(stderr,"get_checksum_udp : passing a null packet to get checksum\n");
		#endif
		return 0;
	}
	struct udphdr *udp_hed = (struct udphdr *) *datagram;
	return udp_hed->check;
}*/
/*
static proto_s udp_ops = {
	.next=NULL,
	.name="UDP",
	.name_size=3,
	.proto_code=IPPROTO_UDP,
	.fields = udp_fields,
	.number_fields=NUMBER_FIELD_UDP,
	.mandatory_fields = udp_mandatory_fields,
	.number_mandatory_fields = NUMBER_MAND_FIELD_UDP,
	.need_ext_checksum=1,
	.set_packet_field=set_packet_field_udp,
	.get_packet_field=get_packet_field_udp,
	.set_default_values=set_default_values_udp,
	.check_datagram=check_datagram_udp,
	.fill_packet_header=fill_packet_header_udp,
	.after_filling=NULL,
	.get_header_size= get_header_size_udp,
	.create_pseudo_header = create_pseudo_header_udp,
	.compute_checksum=compute_checksum_udp,
	.after_checksum=NULL,
	.generate_socket = generate_socket_udp,
};


ADD_PROTO (udp_ops);


*/
