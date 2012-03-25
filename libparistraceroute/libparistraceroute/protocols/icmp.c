//#include<stdlib.h>
//#include<stdio.h>
//#include<string.h>
//
//#include "proto.h"
//#include <netinet/ip_icmp.h>
//#include <arpa/inet.h>
//
//#define NUMBER_FIELD_ICMP 3
//#define NUMBER_MAND_FIELD_ICMP 2
//#define HEADER_SIZE_ICMP 8 // 8 ?
////#define IPPROTO_ICMP 17
//
//
//
//char* icmp_fields[NUMBER_FIELD_ICMP] = {"type","code","checksum"};
//char* icmp_mandatory_fields[NUMBER_MAND_FIELD_ICMP]={"type","code"};
//
//static void set_default_values_icmp(char** datagram){
//	struct icmphdr *icmp_hed = (struct icmphdr *)  *datagram;
//	icmp_hed->checksum=0; /* zero now, calcule it later*/
//}
//
//static int set_packet_field_icmp(void* value, char* name, char** datagram){//not directly a casted header to permit genericity
//	if(value==NULL){
//		#ifdef DEBUG
//		fprintf(stderr,"set_packet_field_icmp : trying to set NULL value for %s \n",name);
//		#endif
//		return -1;
//	}
//	struct icmphdr *icmp_hed = (struct icmphdr *)  *datagram;
//	if(strncasecmp(name,"code",4)==0){
//		icmp_hed->code=*(( u_int8_t*)value);
//		return 1; //ok
//	}
//	if(strncasecmp(name,"type",4)==0){
//		icmp_hed->type= *(( u_int8_t*)value);
//		return 1;
//	}
//	if(strncasecmp(name,"checksum",8)==0){
//		icmp_hed->checksum=htons(*(( u_int16_t*)value));
//		return 1;
//	}
//	else{
//		return 0;
//	}
//
//}
//
//void* get_packet_field_icmp(char* name,char** datagram){
//	if(datagram==NULL||*datagram==NULL){
//		#ifdef DEBUG
//		fprintf(stderr,"get_packet_field_icmp : trying to read NULL packet\n");
//		#endif
//		return NULL;
//	}
//	//void* res = NULL;
//	struct icmphdr *icmp_hed = (struct icmphdr *)  *datagram;
//	if(strncasecmp(name,"code",4)==0){
//		u_int8_t* res = malloc(sizeof(u_int8_t));
//		*res=icmp_hed->code;
//		return res;
//	}
//	if(strncasecmp(name,"type",4)==0){
//		u_int8_t* res = malloc(sizeof(u_int8_t));
//		*res=icmp_hed->type;
//		return res;
//	}
//	if(strncasecmp(name,"checksum",8)==0){
//		u_int16_t* res = malloc(sizeof(u_int16_t));
//		*res=icmp_hed->checksum;
//		return res;
//	}
//	else{
//		#ifdef DEBUG
//		fprintf(stderr,"get_packet_field_icmp : unknown parameter for icmp : %s\n",name);
//		#endif
//		return NULL;
//	}
//}
//
//int fill_packet_header_icmp(fieldlist_s * fieldlist, char** datagram){
//	//check if fields mandatory present
//	void* val = NULL;
//	if(check_presence_list_of_fields(icmp_mandatory_fields,NUMBER_MAND_FIELD_ICMP,fieldlist)==-1){
//		fprintf(stderr, "fill_packet_header_icmp : missing fields for icmp header \n");
//		return -1;
//	}
//	set_default_values_icmp(datagram);
//	int i=0;
//	for(i=0; i < NUMBER_FIELD_ICMP; i++){
//		val = get_field_value_from_fieldlist(fieldlist, icmp_fields[i]);
//		set_packet_field_icmp(val, icmp_fields[i] , datagram);
//	}
//	return 0;
//}
//
///*
//field_s* create_packet_fields_icmp(){
//	int i=0;
//	field_s* next_el=NULL;
//	for(i=0;i<NUMBER_FIELD_ICMP;i++){
//		field_s* field = malloc(sizeof(field_s));
//		field->name = icmp_fields[i];
//		field->next =next_el;
//		next_el=field;
//	}
//	return next_el;
//}*/
//
//static int generate_socket_icmp(int type_af){
//	//TODO
//	return 0;
//}
//
//int get_header_size_icmp(char** packet, fieldlist_s* fieldlist){
//	int size=HEADER_SIZE_ICMP; 
//	return size;
//}
//
//int create_pseudo_header_icmp(char** datagram, char* proto, unsigned char** pseudo_header, int* size){
//	#ifdef DEBUG
//	fprintf(stderr,"create_pseudo_header_icmp : icmp has no pseudo_header found for protocol %s. parameters left untouched.\n",proto);
//	#endif
//	return -1;
//}
//
//
//
//unsigned short compute_checksum_icmp(char** packet, int packet_size, unsigned char** pseudo_header, int size){
//	//TODO : difference entre erreur et checksum
//	if(size!=0||pseudo_header!=NULL){
//		#ifdef DEBUG
//		fprintf(stderr,"compute_checksum_icmp : passing a non NULL pseudo header for icmp protocol that does not need it. Ignoring pseudo header.\n");
//		#endif
//		return 0;
//	}
//	struct icmphdr *icmp_hed = (struct icmphdr *)  *packet;
//	unsigned short head_size = HEADER_SIZE_ICMP; //to know the size to read
//	//ip_hed->check = sum_calc(head_size, (unsigned short**) packet);
//	icmp_hed->checksum = csum(*(unsigned short**) packet, head_size >> 1);
//	#ifdef DEBUG
//	fprintf(stdout,"compute_checksum_icmp : value %d found for icmp checksum (%04x) \n",icmp_hed->checksum, icmp_hed->checksum);
//	#endif
//	return icmp_hed->checksum;
//}
//
//int check_datagram_icmp(char** datagram, int packet_size){
//	if(datagram==NULL||*datagram==NULL){
//		return 0;
//	}
//	char* data = *datagram;
//	if(packet_size<HEADER_SIZE_ICMP){
//		return 0;
//	}
//	struct icmphdr *icmp_hed = (struct icmphdr *) data;
//	unsigned short checksum = compute_checksum_icmp(datagram, strlen(*datagram), NULL,0);
//	if(checksum != icmp_hed->checksum){
//		return 0;
//	}
//	return 1;
//}
//
///*
//unsigned short get_checksum_icmp(char** datagram){
//		if(datagram==NULL||*datagram==NULL){
//		#ifdef DEBUG
//		fprintf(stderr,"get_checksum_icmp : passing a null packet to get checksum\n");
//		#endif
//		return 0;
//	}
//	struct icmphdr *icmp_hed = (struct icmphdr *)  *datagram;
//	return icmp_hed->checksum;
//}*/
//
//static proto_s icmp_ops = {
//	.next=NULL,
//	.name="ICMP",
//	.name_size=4,
//	.proto_code=IPPROTO_ICMP,
//	.fields = icmp_fields,
//	.number_fields=NUMBER_FIELD_ICMP,
//	.mandatory_fields = icmp_mandatory_fields,
//	.number_mandatory_fields = NUMBER_MAND_FIELD_ICMP,
//	.need_ext_checksum=0,
//	.set_packet_field=set_packet_field_icmp,
//	.get_packet_field=get_packet_field_icmp,
//	.set_default_values=set_default_values_icmp,
//	.check_datagram=check_datagram_icmp,
//	.fill_packet_header=fill_packet_header_icmp,
//	.after_filling=NULL,
//	.get_header_size= get_header_size_icmp,
//	.create_pseudo_header = create_pseudo_header_icmp,
//	.compute_checksum=compute_checksum_icmp,
//	.after_checksum=NULL,
//	.generate_socket = generate_socket_icmp,
//};
//
//
//ADD_PROTO (icmp_ops);
//
//
//
