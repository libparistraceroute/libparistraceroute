#include <stdlib.h>
#include <stdio.h>
#include <stddef.h> // offsetof()
#include <string.h> // memcpy()
//#include <unistd.h>
//#include <arpa/inet.h>
//#include <netdb.h>
//
#include <netinet/ip.h>

#include "../field.h"
//#include "../protocol_field.h"
#include "../protocol.h"

#define IPV4_DEFAULT_VERSION         4 
#define IPV4_DEFAULT_IHL             5
#define IPV4_DEFAULT_TOS             0
#define IPV4_DEFAULT_LENGTH          0
#define IPV4_DEFAULT_IDENTIFICATION  1
#define IPV4_DEFAULT_FRAGOFF         0
#define IPV4_DEFAULT_TTL             255 
#define IPV4_DEFAULT_PROTOCOL        17   // here TCP, http://en.wikipedia.org/wiki/List_of_IP_protocol_numbers 
#define IPV4_DEFAULT_CHECKSUM        0
#define IPV4_DEFAULT_SRC_IP          0
#define IPV4_DEFAULT_DST_IP          0

/* IPv4 fields */
static protocol_field_t ipv4_fields[] = {
//    {
//        .key = "version",
//        .type = TYPE_INT4,
//        .offset = offsetof(struct iphdr, version),
//    }, {
//        .key = "ihl",
//        .type = TYPE_INT4,
//        .offset = offsetof(struct iphdr, ihl),
//    },
    {
        .key = "tos",
        .type = TYPE_INT8,
        .offset = offsetof(struct iphdr, tos),
    }, {
        .key = "length",
        .type = TYPE_INT16,
        .offset = offsetof(struct iphdr, tot_len),
    }, {
        .key = "identification",
        .type = TYPE_INT16,
        .offset = offsetof(struct iphdr, id),
    }, {
        .key = "fragment_offset",
        .type = TYPE_INT16,
        .offset = offsetof(struct iphdr, frag_off),
    }, {
        .key = "ttl",
        .type = TYPE_INT8,
        .offset = offsetof(struct iphdr, ttl),
    }, {
        .key = "protocol",
        .type = TYPE_INT8,
        .offset = offsetof(struct iphdr, protocol),
    }, {
        .key = "checksum",
        .type = TYPE_INT16,
        .offset = offsetof(struct iphdr, check),
    }, {
        .key = "src_ip",
        .type = TYPE_INT32,
        .offset = offsetof(struct iphdr, saddr),
    }, {
        .key = "dst_ip",
        .type = TYPE_INT32,
        .offset = offsetof(struct iphdr, daddr),
    }
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

void ipv4_write_default_header(char *data)
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

void ipv4_write_checksum (iphdr * ipv4_hdr, pseudoheader_t * /* unused */){
	unsigned short checksum;
	iphdr *ip_hed = (iphdr *) datagram;
	ip_hed->check = csum((unsigned short *) ipv4_hdr, size >> 1 ); // TODO: >> 1 RLY?
}


static protocol_t ipv4 = {
    .name                 = "ipv4",
    .get_num_fields       = ipv4_get_num_fields,
  //.write_checksum       = CAST_WRITE_CHECKSUM ipv4_write_checksum,
  //.create_pseudo_header = NULL,
    .fields               = ipv4_fields,
    .write_default_header = ipv4_write_default_header, // TODO generic
  //.socket_type          = NULL,
    .get_header_size      = ipv4_get_header_size,
  //.need_ext_checksum    = ipv4_need_ext_checksum
};

PROTOCOL_REGISTER(ipv4);

// END

//
//#define NUMBER_FIELD_IPV4 9
//#define NUMBER_MAND_FIELD_IPV4 3
//#define HEADER_SIZE_IPV4 20
//#define HEADER_SIZE_OPTIONS_IPV4 60
//
//#define HD_UDP_DADDR 4
//#define HD_UDP_PAD 8
//#define HD_UDP_PROT 9
//#define HD_UDP_LEN 10
//
//typedef struct iphdr iphdr_s;
//
//char* ipv4_fields[NUMBER_FIELD_IPV4] = {"using_timestamp","tos","total_lenght","id","frag_off","TTL","next_proto","source_addr","dest_addr"};
//char* ipv4_mandatory_fields[NUMBER_MAND_FIELD_IPV4] = {/*"source_addr",*/"dest_addr","total_lenght","next_proto"/*,"id"*/};//TODO: id chosen randomly? 
//
//
////from brice augustin, to guess source address
///*char* guess_source_address_ipv4 (char* dest) {
//        int sock;
//        struct sockaddr_in addr;
//	struct sockaddr_in name;
//        int len;
//
//        sock = socket(AF_INET, SOCK_DGRAM, 0);
//        if (sock < 0) {
//                error("guess_source_address_ipv4 : cannot create datagram socket: %s", strerror(errno));
//                return NULL;
//        }
//	unsigned long* result;
//	int inet_pton(AF_INET, dest, result)
//        memset(&addr, 0, sizeof(struct sockaddr_in));
//        addr.sin_family = AF_INET;
//        addr.sin_addr.s_addr = result;
//        addr.sin_port = htons(32000);
//
//        if (connect(sock,(struct sockaddr*)&addr,sizeof(struct sockaddr_in))<0){
//                error("guess_source_address_ipv4 : cannot connect socket: %s", strerror(errno));
//                return NULL;
//        }
//
//        len = sizeof(struct sockaddr_in);
//        if (getsockname(sock,(struct sockaddr*)&name,(socklen_t*)&len) < -1) {
//                error("guess_source_address_ipv4 : cannot getsockname: %s", strerror(errno));
//                return NULL;
//        }
//
//        close(sock);
//	char address[16];
//	if( inet_ntop(AF_INET, name.sin_addr.s_addr ,address, 16)==NULL){
//                error("guess_source_address_ipv4 : could not translate IP adress : %s", strerror(errno));
//                return NULL;
//	}
//        return address;
//}*/
//
//u_int32_t guess_source_adress_ipv4 (u_int32_t dest) {
//        int sock;
//        struct sockaddr_in addr, name;
//        int len;
//        sock = socket(AF_INET, SOCK_DGRAM, 0);
//        if (sock < 0) {
//		#ifdef DEBUG
//                fprintf(stderr,"guess_source_adress_ipv4 : cannot create datagram socket\n");
//		#endif
//                return 0;
//        }
//        memset(&addr, 0, sizeof(struct sockaddr_in));
//        addr.sin_family = AF_INET;
//        addr.sin_addr.s_addr = dest;
//        addr.sin_port = htons(32000);
//        if (connect(sock,(struct sockaddr*)&addr,sizeof(struct sockaddr_in))<0){
//		#ifdef DEBUG
//                fprintf(stderr,"guess_source_adress_ipv4 : cannot connect socket\n");
//		#endif
//                return 0;
//        }
//        len = sizeof(struct sockaddr_in);
//        if (getsockname(sock,(struct sockaddr*)&name,(socklen_t*)&len) < -1) {
//		#ifdef DEBUG
//                fprintf(stderr,"guess_source_adress_ipv4 : cannot getsockname\n");
//		#endif
//                return 0;
//        }
//        close(sock);
//        return name.sin_addr.s_addr;
//}
//
//int set_packet_field_ipv4(void* value, char* name , char** datagram){
//	if(value==NULL){
//		#ifdef DEBUG
//		fprintf(stderr,"set_packet_field_ipv4 : trying to set NULL value for %s \n",name);
//		#endif
//		return -1;
//	}
//	#ifdef DEBUG
//	fprintf(stderr,"trying to set value for %s \n",name);
//	#endif
//	iphdr_s *ip_hed = (iphdr_s *)  *datagram;
//	//using_timestamp","header_lenght","tos","total_lenght","id","frag_off","TTL","protocol","source_addr","dest_addr"
//	if(strncasecmp(name,"tos",3)==0){
//		ip_hed->tos= *((u_int8_t*)value);
//		return 1;
//	}
//	if(strncasecmp(name,"using_timestamp",15)==0){
//		//short size_supp= *((short*)(field->value));
//		ip_hed->ihl = ip_hed->ihl+10; //TODO : calculates correct value to add
//		return 1;
//	}
//	if(strncasecmp(name,"id",2)==0){
//		ip_hed->id= htons(*((u_int16_t*)value));
//		return 1;
//	}
//	if(strncasecmp(name,"frag_off",8)==0){
//		ip_hed->frag_off= htons(*((u_int16_t*)value));
//		return 1;
//	}
//	if(strncasecmp(name,"TTL",3)==0){
//		ip_hed->ttl= *((u_int8_t*)value);
//		return 1;
//	}
//	if(strncasecmp(name,"next_proto",10)==0){
//		struct protoent* prot = getprotobyname(*((char**) value));
//		if(prot==NULL){
//			fprintf(stderr,"set_packet_field_ipv4 : error while trying to fill protocol field (value %s) for Ipv4\n", *((char**) value));
//			return -1;
//		}
//		#ifdef DEBUG
//		fprintf(stdout,"find n° %d for proto %s\n",(u_int8_t)prot->p_proto,*((char**) value));
//		#endif
//		ip_hed->protocol= (u_int8_t)prot->p_proto;
//		return 1;
//	}
//	if(strncasecmp(name,"total_lenght",12)==0){
//		ip_hed->tot_len= htons(*((u_int16_t*)value));
//		return 1;
//	}
//	if(strncasecmp(name,"source_addr",11)==0){
//		int verif = inet_pton(AF_INET,(char*)value,&ip_hed->saddr);
//		if(verif!=1){
//			fprintf(stderr,"set_packet_field_ipv4 : error while trying to read source adress %s\n", *((char**)value) );
//			return -1;
//		}
//		return 1; 
//	}
//	if(strncasecmp(name,"dest_addr",9)==0){
//		int verif = inet_pton(AF_INET,(char*)value,&ip_hed->daddr);
//		if(verif!=1){
//			fprintf(stderr,"set_packet_field_ipv4 : error while trying to read destination adress %s\n", *((char**)value));
//			return -1;
//		}
//		return 1;
//	}
//	return -1;
//}
//
//
//void* get_packet_field_ipv4(char* name,char** datagram){
//	if(datagram==NULL||*datagram==NULL){
//		#ifdef DEBUG
//		fprintf(stderr,"get_packet_field_ipv4 :trying to read NULL packet\n");
//		#endif
//		return NULL;
//	}
//	//void* res;
//	iphdr_s *ip_hed = (iphdr_s *)  *datagram;
//	//using_timestamp","header_lenght","tos","total_lenght","id","frag_off","TTL","protocol","source_addr","dest_addr"
//	if(strncasecmp(name,"tos",3)==0){
//		u_int8_t* res= malloc(sizeof(u_int8_t));
//		*res = ip_hed->tos;
//		return res;
//	}
//	if(strncasecmp(name,"id",2)==0){
//		u_int16_t* res=malloc(sizeof(u_int16_t));
//		*res =  ntohs(ip_hed->id);
//		return res;
//	}
//	if(strncasecmp(name,"frag_off",8)==0){
//		u_int16_t* res=malloc(sizeof(u_int16_t));
//		*res=ip_hed->frag_off;
//		return res;
//	}
//	if(strncasecmp(name,"TTL",3)==0){
//		u_int8_t* res=malloc(sizeof(u_int8_t));
//		*res=ip_hed->ttl;
//		return res;
//	}
//	if(strncasecmp(name,"next_proto",10)==0){
//		u_int8_t* res=malloc(sizeof(u_int8_t));
//		*res=ip_hed->protocol;
//		return res;
//	}
//	if(strncasecmp(name,"total_lenght",12)==0){
//		u_int16_t* res=malloc(sizeof(u_int16_t));
//		*res =  ntohs(ip_hed->tot_len);
//		return res;
//	}
//	if(strncasecmp(name,"source_addr",11)==0){
//		//res=malloc(sizeof(char*));
//		char* res=malloc(16*sizeof(char));//xxx.xxx.xxx.xxx\0
//		inet_ntop(AF_INET,&ip_hed->saddr,res,16);
//		return res;
//	}
//	if(strncasecmp(name,"dest_addr",9)==0){
//		//res=malloc(sizeof(char*));
//		char* res=malloc(16*sizeof(char));//xxx.xxx.xxx.xxx\0
//		memset(res,0,16);
//		inet_ntop(AF_INET,&ip_hed->daddr,res,16);
//		return res;
//	}
//	if(strncasecmp(name,"checksum",8)==0){
//		u_int16_t* res=malloc(sizeof(u_int16_t));
//		memset(res,0,16);
//		*res=ip_hed->check;
//		return res;
//	}
//	#ifdef DEBUG
//	fprintf(stderr,"get_packet_field_ipv4 : unknown option for ipv4 : %s \n",name);
//	#endif
//	return NULL;
//}
///*
//void set_cheksum_tot_len (char* datagram, int size){
//	unsigned short checksum;
//	iphdr_s *ip_hed = (iphdr_s *) datagram;
//	ip_hed->tot_len = size;
//	checksum = csum((unsigned short *)datagram,size >> 1 ); // TODO: >> 1 RLY?
//	ip_hed->check = checksum;	
//}
//*/
//
///*
//short unsigned get_checksum(char* datagram){
//	iphdr_s *ip_hed = (iphdr_s *)  datagram;
//	return ip_hed->check;
//}*/
//
//int after_filling_ipv4(char** datagram){
//	iphdr_s *ip_hed = (iphdr_s *) *datagram;
//	if(ip_hed->saddr==0){//has to be calculated 
//		ip_hed->saddr = guess_source_adress_ipv4 (ip_hed->daddr);
//	}
//	return 0;
//}
//
//void set_default_values_ipv4(char** datagram){
//	iphdr_s *ip_hed = (iphdr_s *) *datagram;
//	ip_hed->ihl = 5;
//	ip_hed->version=4;
//  	ip_hed->tos = 0;
//	ip_hed->frag_off = 0;
//	ip_hed->id= htons(1);
//	ip_hed->ttl = 255;
//	ip_hed->check = 0;		/* set it to 0 before computing the actual checksum later */
//	ip_hed->saddr = 0; 		/* so if not specified, after_filling will computes it by himself*/
//}
//
//int fill_packet_header_ipv4(fieldlist_s * field, char** datagram){
//	void* val = NULL;
//	//check if fields mandatory present
//	if(check_presence_list_of_fields(ipv4_mandatory_fields,NUMBER_MAND_FIELD_IPV4,field)==-1){
//		fprintf(stderr, "fill_packet_header_ipv4 : missing fields for IPv4 header \n");
//		return -1;
//	}
//	set_default_values_ipv4(datagram);
//	int i=0;
//	for(i=0; i < NUMBER_FIELD_IPV4; i++){
//		
//		val = get_field_value_from_fieldlist(field, ipv4_fields[i]);
//		set_packet_field_ipv4(val, ipv4_fields[i] , datagram);
//	}
//	return 0;
//}
//
//int generate_socket_ipv4(int af){
//	//TODO
//	//ici le af ne servirait pas?
//	return 0;
//}
//
//int get_header_size_ipv4(char** packet, fieldlist_s* fieldlist){
//	int res = 0;
//	if(*packet == NULL){
//		#ifdef DEBUG
//		fprintf(stderr,"get_header_size_ipv4 : packet is missing. Guessing size of header\n");
//		#endif
//	}
//	else{
//		iphdr_s *ip_hed = (iphdr_s *) *packet;
//		unsigned int ip_hl=0; //:4
//		ip_hl = ip_hed->ihl;
//		res = 4*ip_hl;
//	}
//	if( res == 0){//header not filled yet//packet empty
//		res = check_presence_fieldlist("using_timestamp",fieldlist);
//		if(res== -1){//no option
//			res = HEADER_SIZE_IPV4;
//		}
//		else{
//			res = HEADER_SIZE_OPTIONS_IPV4; //TODO is that true? ->count the timestamp option
//		}
//		return res;
//	}
//	else{
//		return res;
//	}
//	return -1;
//}
//
//int create_pseudo_header_ipv4(char** datagram, char* proto, unsigned char** pseudo_header, int* size){
//	iphdr_s *ip_hed = (iphdr_s *) *datagram;
//	if(*datagram==NULL){
//		#ifdef DEBUG
//		fprintf(stderr,"create_pseudo_header_ipv4 : packet is empty, could not create pseudo header.end\n");
//		#endif
//		return -1;
//	}
//	if(strncasecmp(proto,"udp",3)==0||strncasecmp(proto,"tcp",3)==0){
//		#ifdef DEBUG
//		fprintf(stderr,"create_pseudo_header_ipv4 : creating a pseudo header for %s \n",proto);
//		#endif
//		*size = 12;
//		* pseudo_header = malloc(12*sizeof(char));
//		*(( u_int32_t *)*pseudo_header) = (u_int32_t) ip_hed->saddr;
//		*(( u_int32_t *)(*pseudo_header + HD_UDP_DADDR)) = (u_int32_t) ip_hed->daddr;
//		*((u_int8_t *)(*pseudo_header + HD_UDP_PAD)) =0;
//		*((u_int8_t *)(*pseudo_header + HD_UDP_PROT)) = (u_int8_t) ip_hed->protocol;
//		*((u_int16_t*)(*pseudo_header + HD_UDP_LEN)) = (u_int16_t) htons(ntohs(ip_hed->tot_len)-4*ip_hed->ihl);
//		#ifdef DEBUG
//		int i =0;
//		fprintf(stdout,"create_pseudo_header_ipv4 : pseudo header created : \n");
//		for(i=0;i<12;i++){
//			fprintf(stdout," %02x ",(*pseudo_header)[i]);
//		}
//		fprintf(stdout,"\n");
//		#endif
//		return 0;
//	}
//	//else case
//	#ifdef DEBUG
//	fprintf(stderr,"create_pseudo_header_ipv4 : IPv4 has no pseudo header found for protocol %s. parameters left untouched.\n",proto);
//	#endif
//	return -1;
//}
//
//
//
//unsigned short compute_checksum_ipv4(char** packet, int packet_size, unsigned char** pseudo_header, int size){//TODO : unsgined short or u_int16_t ?
//	if(size!=0){
//		#ifdef DEBUG
//		fprintf(stderr,"compute_checksum_ipv4 : passing a non NULL pseudo header for IPv4 protocol that does not need it. Ignoring pseudo header.\n");
//		#endif
//	}
//	iphdr_s *ip_hed = (iphdr_s *) *packet;
//	unsigned short head_size = 4*((int)ip_hed->ihl); //to know the size to read
//	//ip_hed->check = sum_calc(head_size, (unsigned short**) packet);
//	ip_hed->check = csum(*(unsigned short**) packet, head_size >> 1);
//	#ifdef DEBUG
//	fprintf(stdout,"compute_checksum_ipv4 : value %d found for IPv4 checksum (%04x) \n",ip_hed->check, ip_hed->check);
//	#endif
//	return ip_hed->check;
//}
//
//int check_datagram_ipv4(char** datagram, int packet_size){
//	if(datagram==NULL||*datagram==NULL){
//		return 0;
//	}
//	char* data = *datagram;
//	if(packet_size<HEADER_SIZE_IPV4){
//		#ifdef DEBUG
//		fprintf(stdout,"check_datagram_ipv4 : packet too small to be Ipv4\n");
//		#endif
//		return 0;
//	}
//	iphdr_s *ip_hed = (iphdr_s *) data;
//	if(ip_hed->version!=4){
//	#ifdef DEBUG
//		fprintf(stdout,"check_datagram_ipv4 : value %d found for Ipv4 header; not an ipv4 \n",ip_hed->version);
//		#endif
//		return 0;
//	}
//	unsigned short checksum = compute_checksum_ipv4(datagram, packet_size, NULL,0);
//	if(checksum != ip_hed->check){
//		#ifdef DEBUG
//		fprintf(stdout,"check_datagram_ipv4 : value %d found for Ipv4 checksum instead of %d \n", checksum, ip_hed->check);
//		#endif
//		return 0;
//	}
//	#ifdef DEBUG
//	fprintf(stdout,"header ipv4 detected\n");
//	#endif
//	return 1;
//}
//
///*
//unsigned short get_checksum_ipv4 (char** datagram){
//		if(datagram==NULL||*datagram==NULL){
//		#ifdef DEBUG
//		fprintf(stderr,"get_checksum_ipv4 : passing a null packet to get checksum\n");
//		#endif
//		return 0;
//	}
//	iphdr_s *ip_hed = (iphdr_s *) *datagram;
//	return ip_hed->check;
//}*/
//
//static proto_s ipv4_ops = {
//	.next=NULL,
//	.name="IPv4",
//	.name_size=4,
//	.proto_code=AF_INET,
//	.fields = ipv4_fields,
//	.number_fields = NUMBER_FIELD_IPV4,
//	.mandatory_fields = ipv4_mandatory_fields,
//	.number_mandatory_fields = NUMBER_MAND_FIELD_IPV4,
//	.need_ext_checksum=0,
//	.set_packet_field=set_packet_field_ipv4,
//	.get_packet_field=get_packet_field_ipv4,
//	.set_default_values=set_default_values_ipv4,
//	.check_datagram=check_datagram_ipv4,
//	.fill_packet_header=fill_packet_header_ipv4,
//	.after_filling=after_filling_ipv4,
//	.get_header_size = get_header_size_ipv4,
//	.create_pseudo_header=create_pseudo_header_ipv4,
//	.compute_checksum=compute_checksum_ipv4,
//	.after_checksum=NULL,
//	.generate_socket = generate_socket_ipv4,
//};
//
//
//ADD_PROTO (ipv4_ops);
//
//
//
