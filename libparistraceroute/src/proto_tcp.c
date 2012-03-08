#include<stdlib.h>
#include<stdio.h>
#include<string.h>

#include "proto.h"
#include <netinet/tcp.h>
#include <arpa/inet.h>

#define NUMBER_FIELD_TCP 10
#define NUMBER_MAND_FIELD_TCP 1 //juste le port dest?
#define HEADER_SIZE_TCP 20
//#define IPPROTO_TCP 17

/*

struct tcphdr
  {
    u_int16_t th_sport;          source port 
    u_int16_t th_dport;          destination port 
    tcp_seq th_seq;              sequence number 
    tcp_seq th_ack;              acknowledgement number 
#  if __BYTE_ORDER == __LITTLE_ENDIAN
    u_int8_t th_x2:4;            (unused) 
    u_int8_t th_off:4;           data offset 
#  endif
#  if __BYTE_ORDER == __BIG_ENDIAN
    u_int8_t th_off:4;           data offset 
    u_int8_t th_x2:4;            (unused) 
#  endif
    u_int8_t th_flags;
#  define TH_FIN        0x01
#  define TH_SYN        0x02
#  define TH_RST        0x04
#  define TH_PUSH       0x08
#  define TH_ACK        0x10
#  define TH_URG        0x20
    u_int16_t th_win;            window 
    u_int16_t th_sum;            checksum 
    u_int16_t th_urp;            urgent pointer 
};


*/

char* tcp_fields[NUMBER_FIELD_TCP] = {"src_port","dest_port","seq_num","ack_num","offset","ctrl_bits","window","checksum","urg_ptr","options"};
char* tcp_mandatory_fields[NUMBER_MAND_FIELD_TCP]={"src_port"};

static void set_default_values_tcp(char** datagram){
	struct tcphdr *tcp_hed = (struct tcphdr *)  *datagram;
	tcp_hed->check=0; /* zero now, calcule it later*/
}

static int set_packet_field_tcp(void* value, char* name, char** datagram){//not directly a casted header to permit genericity
	if(value==NULL){
		#ifdef DEBUG
		fprintf(stderr,"set_packet_field_tcp : trying to set NULL value for %s \n",name);
		#endif
		return -1;
	}
	struct tcphdr *tcp_hed = (struct tcphdr *)  *datagram;
	if(strncasecmp(name,"dest_port",9)==0){
		#ifdef __FAVOR_BSD
		tcp_hed->th_dport=htons(*(( u_int16_t*)value));
		#else
		tcp_hed->dest=htons(*(( u_int16_t*)value));
		#endif
		return 1; //ok
	}
	if(strncasecmp(name,"src_port",8)==0){
		#ifdef __FAVOR_BSD
		tcp_hed->th_sport=htons(*(( u_int16_t*)value));
		#else
		tcp_hed->source=htons(*(( u_int16_t*)value));
		#endif
		return 1;
	}
	if(strncasecmp(name,"urg_ptr",7)==0){
		#ifdef __FAVOR_BSD
		tcp_hed->th_urp=htons(*(( u_int16_t*)value));
		#else
		tcp_hed->urg_ptr=htons(*(( u_int16_t*)value));
		#endif
		return 1;
	}
	if(strncasecmp(name,"checksum",8)==0){
		#ifdef __FAVOR_BSD
		if(tcp_hed->th_sum!=0){
			fprintf(stderr,"set_packet_field_tcp : old checksum not null, probably overwriting checksum or tag data");
		}
		tcp_hed->th_sum=htons(*(( u_int16_t*)value));
		#else
		if(tcp_hed->check!=0){
			fprintf(stderr,"set_packet_field_tcp : old checksum not null, probably overwriting checksum or tag data");
		}
		tcp_hed->check=htons(*(( u_int16_t*)value));
		#endif
		return 1;
	}
	else{
		return 0;
	}

}

void* get_packet_field_tcp(char* name,char** datagram){
	if(datagram==NULL||*datagram==NULL){
		#ifdef DEBUG
		fprintf(stderr,"get_packet_field_tcp :trying to read NULL packet\n");
		#endif
		return NULL;
	}
	//void* res;
	struct tcphdr *tcp_hed = (struct tcphdr *)  *datagram;
	if(strncasecmp(name,"dest_port",9)==0){
		u_int16_t* res=malloc(sizeof(u_int16_t));
		#ifdef __FAVOR_BSD
		*res = ntohs(tcp_hed->uh_dport);
		#else
		*res = ntohs(tcp_hed->dest);
		#endif
		return res; 
	}
	if(strncasecmp(name,"src_port",8)==0){
		u_int16_t* res=malloc(sizeof(u_int16_t));
		#ifdef __FAVOR_BSD
		*res = ntohs(tcp_hed->uh_sport);
		#else
		*res = ntohs(tcp_hed->source);
		#endif
		return res;
	}
	if(strncasecmp(name,"urg_ptr",12)==0){
		u_int16_t* res=malloc(sizeof(u_int16_t));
		#ifdef __FAVOR_BSD
		*res = ntohs(tcp_hed->th_urp);
		#else
		*res = ntohs(tcp_hed->urg_ptr);
		#endif
		return res;
	}
	if(strncasecmp(name,"checksum",8)==0){
		u_int16_t* res=malloc(sizeof(u_int16_t));
		#ifdef __FAVOR_BSD
		*res = ntohs(tcp_hed->uh_sum);
		#else
		*res = ntohs(tcp_hed->check);
		#endif
		return res;
	}
	else{
		#ifdef DEBUG
		fprintf(stderr,"get_packet_field_tcp :unknown option for tcp : %s \n",name);
		#endif
		return NULL;
	}
}

int fill_packet_header_tcp(fieldlist_s * fieldlist, char** datagram){
	//check if fields mandatory present
	void* val = NULL;
	if(check_presence_list_of_fields(tcp_mandatory_fields,NUMBER_MAND_FIELD_TCP,fieldlist)==-1){
		fprintf(stderr, "fill_packet_header_tcp : missing fields for tcp header \n");
		return -1;
	}
	set_default_values_tcp(datagram);
	int i=0;
	for(i=0; i < NUMBER_FIELD_TCP; i++){
		
		val = get_field_value_from_fieldlist(fieldlist, tcp_fields[i]);
		set_packet_field_tcp(val, tcp_fields[i] , datagram);
	}
	return 0;
}

/*
field_s* create_packet_fields_tcp(){
	int i=0;
	field_s* next_el=NULL;
	for(i=0;i<NUMBER_FIELD_TCP;i++){
		field_s* field = malloc(sizeof(field_s));
		field->name = tcp_fields[i];
		field->next =next_el;
		next_el=field;
	}
	return next_el;
}*/

static int generate_socket_tcp(int type_af){
	//TODO
	return 0;
}

int get_header_size_tcp(char** packet, fieldlist_s* fieldlist){
	int size=HEADER_SIZE_TCP; //2* 32-byte words, no possible options
	return size;
}

int create_pseudo_header_tcp(char** datagram, char* proto, unsigned char** pseudo_header, int* size){
	#ifdef DEBUG
	fprintf(stderr,"create_pseudo_header_tcp : tcp has no pseudo_header found for protocol %s. parameters left untouched.\n",proto);
	#endif
	return -1;
}

unsigned short compute_checksum_tcp(char** packet, int packet_size, unsigned char** pseudo_header, int size){
	//TODO : difference entre erreur et checksum
	if(size==0||pseudo_header==NULL){
		#ifdef DEBUG
		fprintf(stderr,"compute_checksum_tcp : passing a NULL pseudo header when tcp needs one. end\n");
		#endif
		return 0;
	}
	unsigned char * total_header=malloc((packet_size+size)*sizeof(char));
	memset(total_header, 0, packet_size+size);
	memcpy(total_header,*pseudo_header,size);//tcp checksum n all the packet
	memcpy(total_header+size,*packet,packet_size);
	#ifdef DEBUG
	int i =0;
	fprintf(stdout,"compute_checksum_tcp : pseudo header for tcp checksum : \n");
	for(i=0;i<size+packet_size;i++){
		fprintf(stdout," %02x ",total_header[i]);
	}
	fprintf(stdout,"\n");
	#endif
	//unsigned short res = sum_calc((unsigned short) size+packet_size, (unsigned short**)&total_header);
	unsigned short res = csum(*(unsigned short**)&total_header, ((size+packet_size) >> 1));
	#ifdef DEBUG
	fprintf(stdout,"compute_checksum_tcp : value %d found for tcp checksum (%04x) \n",res, res);
	#endif
	struct tcphdr *tcp_hed = (struct tcphdr *)  *packet;
	tcp_hed->check=res;
	return res;
}

int check_datagram_tcp(char** datagram, int packet_size){
	if(datagram==NULL||*datagram==NULL){
		return 0;
	}
	char* data = *datagram;
	if(packet_size<HEADER_SIZE_TCP){
		return 0;
	}
	//struct tcphdr *tcp_hed = (struct tcphdr *) data;
	//if(tcp_hed->len!=htons(HEADER_SIZE_TCP)){
	//	return 0;
	//}
	return 1;
}

/*
unsigned short get_checksum_tcp(char** datagram){
		if(datagram==NULL||*datagram==NULL){
		#ifdef DEBUG
		fprintf(stderr,"get_checksum_tcp : passing a null packet to get checksum\n");
		#endif
		return 0;
	}
	struct tcphdr *tcp_hed = (struct tcphdr *) *datagram;
	return tcp_hed->check;
}*/

static proto_s tcp_ops = {
	.next=NULL,
	.name="TCP",
	.name_size=3,
	.proto_code=IPPROTO_TCP,
	.fields = tcp_fields,
	.number_fields=NUMBER_FIELD_TCP,
	.mandatory_fields = tcp_mandatory_fields,
	.number_mandatory_fields = NUMBER_MAND_FIELD_TCP,
	.need_ext_checksum=1,
	.set_packet_field=set_packet_field_tcp,
	.get_packet_field=get_packet_field_tcp,
	.set_default_values=set_default_values_tcp,
	.check_datagram=check_datagram_tcp,
	.fill_packet_header=fill_packet_header_tcp,
	.after_filling=NULL,
	.get_header_size= get_header_size_tcp,
	.create_pseudo_header = create_pseudo_header_tcp,
	.compute_checksum=compute_checksum_tcp,
	.after_checksum=NULL,
	.generate_socket = generate_socket_tcp,
};


ADD_PROTO (tcp_ops);



