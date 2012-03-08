#include<stdlib.h>
#include<stdio.h>
#include<string.h>

#include "proto.h"
#include <netinet/udp.h>
#include <arpa/inet.h>

#define NUMBER_FIELD_UDP 5
#define NUMBER_MAND_FIELD_UDP 3
#define HEADER_SIZE_UDP 8
//#define IPPROTO_UDP 17

char* udp_fields[NUMBER_FIELD_UDP] = {"src_port","dest_port","total_lenght","checksum","with_tag"};
char* udp_mandatory_fields[NUMBER_MAND_FIELD_UDP]={"src_port","dest_port","total_lenght"};

static void set_default_values_udp(char** datagram){
	struct udphdr *udp_hed = (struct udphdr *)  *datagram;
	udp_hed->check=0; /* zero now, calcule it later*/
}

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
	if(strncasecmp(name,"total_lenght",12)==0){
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

void* get_packet_field_udp(char* name,char** datagram){
	if(datagram==NULL||*datagram==NULL){
		#ifdef DEBUG
		fprintf(stderr,"get_packet_field_udp :trying to read NULL packet\n");
		#endif
		return NULL;
	}
	//void* res;
	struct udphdr *udp_hed = (struct udphdr *)  *datagram;
	if(strncasecmp(name,"dest_port",9)==0){
		u_int16_t* res=malloc(sizeof(u_int16_t));
		#ifdef __FAVOR_BSD
		*res = ntohs(udp_hed->uh_dport);
		#else
		*res = ntohs(udp_hed->dest);
		#endif
		return res; 
	}
	if(strncasecmp(name,"src_port",8)==0){
		u_int16_t* res=malloc(sizeof(u_int16_t));
		#ifdef __FAVOR_BSD
		*res = ntohs(udp_hed->uh_sport);
		#else
		*res = ntohs(udp_hed->source);
		#endif
		return res;
	}
	if(strncasecmp(name,"total_lenght",12)==0){
		u_int16_t* res=malloc(sizeof(u_int16_t));
		#ifdef __FAVOR_BSD
		*res = ntohs(udp_hed->uh_ulen);
		#else
		*res = ntohs(udp_hed->len);
		#endif
		return res;
	}
	if(strncasecmp(name,"checksum",8)==0){
		u_int16_t* res=malloc(sizeof(u_int16_t));
		#ifdef __FAVOR_BSD
		*res = ntohs(udp_hed->uh_sum);
		#else
		*res = ntohs(udp_hed->check);
		#endif
		return res;
	}
	else{
		#ifdef DEBUG
		fprintf(stderr,"get_packet_field_udp : unknown option for udp : %s \n",name);
		#endif
		return NULL;
	}
}

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



