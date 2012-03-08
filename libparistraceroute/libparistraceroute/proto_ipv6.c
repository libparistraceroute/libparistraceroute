#include<stdlib.h>
#include<unistd.h>
#include<string.h>
#include<stdio.h>
#include <arpa/inet.h>
#include <netdb.h>

#include "proto.h"
#include <netinet/ip6.h>

#define NUMBER_FIELD_IPV6 8
#define NUMBER_MAND_FIELD_IPV6 3
#define HEADER_SIZE_IPV6 40
#define HEADER_SIZE_OPTIONS_IPV6 60

//size in bytes
#define HD_UDP_DADDR 16
#define HD_UDP_LEN 32
#define HD_UDP_PAD 36
#define HD_UDP_PAD2 38
#define HD_UDP_PROTO 39


/*


struct ip6_hdr
  {
    union
      {
	struct ip6_hdrctl
	  {
	    uint32_t ip6_un1_flow;   // 4 bits version, 8 bits TC,
					//20 bits flow-ID 
	    uint16_t ip6_un1_plen;   // payload length 
	    uint8_t  ip6_un1_nxt;    // next header 
	    uint8_t  ip6_un1_hlim;   // hop limit 
	  } ip6_un1;
	uint8_t ip6_un2_vfc;       // 4 bits version, top 4 bits tclass 
      } ip6_ctlun;
    struct in6_addr ip6_src;      // source address 
    struct in6_addr ip6_dst;      // destination address 
  };
*/

typedef struct ip6_hdr ip6hdr_s;

char* ipv6_fields[NUMBER_FIELD_IPV6] = {"traffic_class", "flow", "payload_size", "next_proto", "TTL", "source_addr", "dest_addr","options"};
char* ipv6_mandatory_fields[NUMBER_MAND_FIELD_IPV6] = {"dest_addr","payload_size","next_proto"};


//from brice augustin, to guess source address
/*char* guess_source_address_ipv6 (char* dest) {
        int sock;
        struct sockaddr_in addr;
	struct sockaddr_in name;
        int len;

        sock = socket(AF_INET, SOCK_DGRAM, 0);
        if (sock < 0) {
                error("guess_source_address_ipv6 : cannot create datagram socket: %s", strerror(errno));
                return NULL;
        }
	unsigned long* result;
	int inet_pton(AF_INET, dest, result)
        memset(&addr, 0, sizeof(struct sockaddr_in));
        addr.sin_family = AF_INET;
        addr.sin_addr.s_addr = result;
        addr.sin_port = htons(32000);

        if (connect(sock,(struct sockaddr*)&addr,sizeof(struct sockaddr_in))<0){
                error("guess_source_address_ipv6 : cannot connect socket: %s", strerror(errno));
                return NULL;
        }

        len = sizeof(struct sockaddr_in);
        if (getsockname(sock,(struct sockaddr*)&name,(socklen_t*)&len) < -1) {
                error("guess_source_address_ipv6 : cannot getsockname: %s", strerror(errno));
                return NULL;
        }

        close(sock);
	char address[16];
	if( inet_ntop(AF_INET, name.sin_addr.s_addr ,address, 16)==NULL){
                error("guess_source_address_ipv6 : could not translate IP adress : %s", strerror(errno));
                return NULL;
	}
        return address;
}*/

struct in6_addr guess_source_adress_ipv6 (struct in6_addr dest) {
        int sock;
        struct sockaddr_in6 addr, name;
        int len;
        sock = socket(AF_INET6, SOCK_DGRAM, 0);
        if (sock < 0) {
		#ifdef DEBUG
                fprintf(stderr,"guess_source_adress_ipv6 : cannot create datagram socket\n");
		#endif
                exit (0);
        }
        memset(&addr, 0, sizeof(struct sockaddr_in6));
        addr.sin6_family = AF_INET6;
        addr.sin6_addr = dest;
        addr.sin6_port = htons(32000);
        if (connect(sock,(struct sockaddr*)&addr,sizeof(struct sockaddr_in6))<0){
		#ifdef DEBUG
                fprintf(stderr,"guess_source_adress_ipv6 : cannot connect socket\n");
		#endif
                exit(0);
        }
        len = sizeof(struct sockaddr_in);
        if (getsockname(sock,(struct sockaddr*)&name,(socklen_t*)&len) < -1) {
		#ifdef DEBUG
                fprintf(stderr,"guess_source_adress_ipv6 : cannot getsockname\n");
		#endif
                exit(0);
        }
        close(sock);
        return name.sin6_addr;
}

int set_packet_field_ipv6(void* value, char* name , char** datagram){
	if(value==NULL){
		#ifdef DEBUG
		fprintf(stderr,"set_packet_field_ipv6 : trying to set NULL value for %s \n",name);
		#endif
		return -1;
	}
	#ifdef DEBUG
	fprintf(stderr,"trying to set value for %s \n",name);
	#endif
	ip6hdr_s *ip_hed = (ip6hdr_s *)  *datagram;
	//using_timestamp","header_lenght","tos","total_lenght","id","frag_off","TTL","protocol","source_addr","dest_addr"
	if(strncasecmp(name,"traffic_class",13)==0){
		u_int8_t val= *((u_int8_t*)value);
		u_int32_t mask_or = 0 & val;
		mask_or = mask_or << 20;
		u_int32_t mask_and = 0xF00FFFFF;
		ip_hed->ip6_ctlun.ip6_un1.ip6_un1_flow = ip_hed->ip6_ctlun.ip6_un1.ip6_un1_flow & mask_and;//put the class value at 0
		ip_hed->ip6_ctlun.ip6_un1.ip6_un1_flow = ip_hed->ip6_ctlun.ip6_un1.ip6_un1_flow | mask_or;//put the new value
		return 1;
	}
	if(strncasecmp(name,"flow",4)==0){
		//TODO : hton?
		u_int32_t flow= *((u_int32_t*)value);//mask_or
		u_int32_t mask_and = 0xFFF00000;
		ip_hed->ip6_ctlun.ip6_un1.ip6_un1_flow = ip_hed->ip6_ctlun.ip6_un1.ip6_un1_flow & mask_and;//put the class value at 0
		ip_hed->ip6_ctlun.ip6_un1.ip6_un1_flow = ip_hed->ip6_ctlun.ip6_un1.ip6_un1_flow | flow;//put the new value
		return 1;
	}
	if(strncasecmp(name,"payload_size",12)==0){
		ip_hed->ip6_ctlun.ip6_un1.ip6_un1_plen= htons(*((u_int16_t*)value));
		return 1;
	}
	if(strncasecmp(name,"TTL",3)==0){
		ip_hed->ip6_ctlun.ip6_un1.ip6_un1_hlim= *((u_int8_t*)value);
		return 1;
	}
	if(strncasecmp(name,"next_proto",10)==0){
		struct protoent* prot = getprotobyname(*((char**) value));
		if(prot==NULL){
			fprintf(stderr,"set_packet_field_ipv6 : error while trying to fill protocol field (value %s) for Ipv6\n", *((char**) value));
			return -1;
		}
		#ifdef DEBUG
		fprintf(stdout,"find n° %d for proto %s\n",(u_int8_t)prot->p_proto,*((char**) value));
		#endif
		ip_hed->ip6_ctlun.ip6_un1.ip6_un1_nxt = (u_int8_t)prot->p_proto;
		return 1;
	}
	if(strncasecmp(name,"source_addr",11)==0){
		int verif = inet_pton(AF_INET6,(char*)value,&ip_hed->ip6_src);
		if(verif!=1){
			fprintf(stderr,"set_packet_field_ipv6 : error while trying to read source adress %s\n", *((char**)value) );
			return -1;
		}
		return 1; 
	}
	if(strncasecmp(name,"dest_addr",9)==0){
		int verif = inet_pton(AF_INET6,(char*)value,&ip_hed->ip6_dst);
		if(verif!=1){
			fprintf(stderr,"set_packet_field_ipv6 : error while trying to read destination adress %s\n", *((char**)value));
			return -1;
		}
		return 1;
	}
	return -1;
}


void* get_packet_field_ipv6(char* name,char** datagram){
	if(datagram==NULL||*datagram==NULL){
		#ifdef DEBUG
		fprintf(stderr,"get_packet_field_ipv6 :trying to read NULL packet\n");
		#endif
		return NULL;
	}
	//void* res;
	ip6hdr_s *ip_hed = (ip6hdr_s *)  *datagram;
	//using_timestamp","header_lenght","tos","total_lenght","id","frag_off","TTL","protocol","source_addr","dest_addr"
	if(strncasecmp(name,"traffic_class",13)==0){
		u_int8_t* res= malloc(sizeof(u_int8_t));
		u_int32_t traf = ip_hed->ip6_ctlun.ip6_un1.ip6_un1_flow;
		traf = traf & 0x0FF00000;
		traf = traf >> 20;
		*res = (u_int8_t) traf;
		return res;
	}
	if(strncasecmp(name,"flow",4)==0){
		u_int32_t* res=malloc(sizeof(u_int32_t));
		u_int32_t flow = ip_hed->ip6_ctlun.ip6_un1.ip6_un1_flow;
		flow = flow & 0x000FFFFF;
		*res = flow;
		return res;
	}
	if(strncasecmp(name,"TTL",3)==0){
		u_int8_t* res=malloc(sizeof(u_int8_t));
		*res=ip_hed->ip6_ctlun.ip6_un1.ip6_un1_hlim;
		return res;
	}
	if(strncasecmp(name,"next_proto",10)==0){
		u_int8_t* res=malloc(sizeof(u_int8_t));
		*res=ip_hed->ip6_ctlun.ip6_un1.ip6_un1_nxt;
		return res;
	}
	if(strncasecmp(name,"payload_size",12)==0){
		u_int16_t* res=malloc(sizeof(u_int16_t));
		*res =  ntohs(ip_hed->ip6_ctlun.ip6_un1.ip6_un1_plen);
		return res;
	}
	if(strncasecmp(name,"source_addr",11)==0){
		//res=malloc(sizeof(char*));
		char* res=malloc(40*sizeof(char));//xxxx:xxxx:xxxx:xxxx:xxxx:xxxx:xxxx:xxxx\0
		memset(res,0,40);
		inet_ntop(AF_INET6,&ip_hed->ip6_src,res,40);
		return res;
	}
	if(strncasecmp(name,"dest_addr",9)==0){
		//res=malloc(sizeof(char*));
		char* res=malloc(40*sizeof(char));//xxxx:xxxx:xxxx:xxxx:xxxx:xxxx:xxxx:xxxx\0
		memset(res,0,40);
		inet_ntop(AF_INET6,&ip_hed->ip6_dst,res,40);
		return res;
	}
	#ifdef DEBUG
	fprintf(stderr,"get_packet_field_ipv6 : unknown option for ipv6 : %s \n",name);
	#endif
	return NULL;
}
/*
void set_cheksum_tot_len (char* datagram, int size){
	unsigned short checksum;
	ip6hdr_s *ip_hed = (ip6hdr_s *) datagram;
	ip_hed->tot_len = size;
	checksum = csum((unsigned short *)datagram,size >> 1 ); // TODO: >> 1 RLY?
	ip_hed->check = checksum;	
}*/

/*
short unsigned get_checksum(char* datagram){
	ip6hdr_s *ip_hed = (ip6hdr_s *)  datagram;
	return ip_hed->check;
}*/

int after_filling_ipv6(char** datagram){
	ip6hdr_s *ip_hed = (ip6hdr_s *) *datagram;
	struct in6_addr ip_check;
	inet_pton(AF_INET6,"0:0:0:0:0:0:0:0",&ip_check);
	char check[40];//xxxx:xxxx:xxxx:xxxx:xxxx:xxxx:xxxx:xxxx\0
	char src[40];
	memset(check,0,40);
	memset(src,0,40);
	inet_ntop(AF_INET6,&ip_hed->ip6_src,src,40);
	inet_ntop(AF_INET6,&ip_check,check,40); 
	if(strcmp(src,check)==0){//has to be calculated 
		ip_hed->ip6_dst = guess_source_adress_ipv6 (ip_hed->ip6_src);
	}
	return 0;
}

void set_default_values_ipv6(char** datagram){
	ip6hdr_s *ip_hed = (ip6hdr_s *) *datagram;
	ip_hed->ip6_ctlun.ip6_un1.ip6_un1_flow = 0x60000000;//IP v6 , rest 0
	ip_hed->ip6_ctlun.ip6_un1.ip6_un1_hlim = 255;
	//inet_pton(AF_INET6,"::/128",&ip_hed->ip6_src);
	inet_pton(AF_INET6,"0:0:0:0:0:0:0:0",&ip_hed->ip6_src); 	/* so if not specified, after_filling will computes it by himself*/
	
}

int fill_packet_header_ipv6(fieldlist_s * field, char** datagram){
	void* val = NULL;
	//check if fields mandatory present
	if(check_presence_list_of_fields(ipv6_mandatory_fields,NUMBER_MAND_FIELD_IPV6,field)==-1){
		fprintf(stderr, "fill_packet_header_ipv6 : missing fields for IPv6 header \n");
		return -1;
	}
	set_default_values_ipv6(datagram);
	int i=0;
	for(i=0; i < NUMBER_FIELD_IPV6; i++){
		
		val = get_field_value_from_fieldlist(field, ipv6_fields[i]);
		set_packet_field_ipv6(val, ipv6_fields[i] , datagram);
	}
	return 0;
}

int generate_socket_ipv6(int af){
	//TODO
	//ici le af ne servirait pas?
	return 0;
}

int get_header_size_ipv6(char** packet, fieldlist_s* fieldlist){
	int res = 0;
	if(*packet == NULL){
		#ifdef DEBUG
		fprintf(stderr,"get_header_size_ipv6 : packet is missing. Guessing size of header\n");
		#endif
	}
	else{
		ip6hdr_s *ip_hed = (ip6hdr_s *) *packet;
		unsigned int ip_hl=0; //:4
		ip_hl = ip_hed->ip6_ctlun.ip6_un2_vfc;
		ip_hl = ip_hl >> 4;
		res = 4*ip_hl;
	}
	if( res == 0){//header not filled yet//packet empty
		res = check_presence_fieldlist("options",fieldlist);
		if(res== -1){//no option
			res = HEADER_SIZE_IPV6;
		}
		else{
			res = HEADER_SIZE_OPTIONS_IPV6; //TODO is that true? ->count the options?
		}
		return res;
	}
	else{
		return res;
	}
	return -1;
}

int create_pseudo_header_ipv6(char** datagram, char* proto, unsigned char** pseudo_header, int* size){
	ip6hdr_s *ip_hed = (ip6hdr_s *) *datagram;
	if(*datagram==NULL){
		#ifdef DEBUG
		fprintf(stderr,"create_pseudo_header_ipv6 : packet is empty, could not create pseudo header.end\n");
		#endif
		return -1;
	}
	if(strncasecmp(proto,"udp",3)==0 || strncasecmp(proto,"tcp",3)==0){
		#ifdef DEBUG
		fprintf(stderr,"create_pseudo_header_ipv6 : creating a pseudo header for %s \n",proto);
		#endif
		*size = 40;
		* pseudo_header = malloc(40*sizeof(char));
		*(( struct in6_addr *)*pseudo_header) = (struct in6_addr) ip_hed->ip6_src;
		*(( struct in6_addr *)(*pseudo_header + HD_UDP_DADDR)) = (struct in6_addr) ip_hed->ip6_dst;
		*((u_int32_t *)(*pseudo_header + HD_UDP_LEN)) = (u_int32_t) ip_hed->ip6_ctlun.ip6_un1.ip6_un1_plen;
		*((u_int16_t *)(*pseudo_header + HD_UDP_PAD)) = 0;
		*((u_int8_t *)(*pseudo_header + HD_UDP_PAD2)) = 0;
		*((u_int8_t *)(*pseudo_header + HD_UDP_PROTO)) = (u_int8_t) ip_hed->ip6_ctlun.ip6_un1.ip6_un1_nxt;
		#ifdef DEBUG
		int i =0;
		fprintf(stdout,"create_pseudo_header_ipv6 : pseudo header created : \n");
		for(i=0;i<12;i++){
			fprintf(stdout," %02x ",(*pseudo_header)[i]);
		}
		fprintf(stdout,"\n");
		#endif
		return 0;
	}
	//else case
	#ifdef DEBUG
	fprintf(stderr,"create_pseudo_header_ipv6 : IPv6 has no pseudo header found for protocol %s. parameters left untouched.\n",proto);
	#endif
	return -1;
}



unsigned short compute_checksum_ipv6(char** packet, int packet_size, unsigned char** pseudo_header, int size){//TODO : unsgined short or u_int16_t ?
	#ifdef DEBUG
	fprintf(stdout,"compute_checksum_ipv6 : IPv6 has no checksum field\n");
	#endif
	return 0;
}

int check_datagram_ipv6(char** datagram, int packet_size){
	if(datagram==NULL||*datagram==NULL){
		return 0;
	}
	char* data = *datagram;
	if(packet_size<HEADER_SIZE_IPV6){
		#ifdef DEBUG
		fprintf(stdout,"check_datagram_ipv6 : packet too small to be IPv6\n");
		#endif
		return 0;
	}
	unsigned int ip_hl=0; //:4
	ip_hl = ip_hed->ip6_ctlun.ip6_un2_vfc;
	ip_hl = ip_hl >> 4;
	if(ip_hl!=6){
	#ifdef DEBUG
		fprintf(stdout,"check_datagram_ipv6 : value %d found for IP version; not an ipv6 \n",ip_hl);
		#endif
		return 0;
	}
	#ifdef DEBUG
	fprintf(stdout,"header ipv6 detected\n");
	#endif
	return 1;
}

/*
unsigned short get_checksum_ipv6 (char** datagram){
		if(datagram==NULL||*datagram==NULL){
		#ifdef DEBUG
		fprintf(stderr,"get_checksum_ipv6 : passing a null packet to get checksum\n");
		#endif
		return 0;
	}
	ip6hdr_s *ip_hed = (ip6hdr_s *) *datagram;
	return ip_hed->check;
}*/

static proto_s ipv6_ops = {
	.next=NULL,
	.name="IPv6",
	.name_size=4,
	.proto_code=AF_INET6,
	.fields = ipv6_fields,
	.number_fields = NUMBER_FIELD_IPV6,
	.mandatory_fields = ipv6_mandatory_fields,
	.number_mandatory_fields = NUMBER_MAND_FIELD_IPV6,
	.need_ext_checksum=0,
	.set_packet_field=set_packet_field_ipv6,
	.get_packet_field=get_packet_field_ipv6,
	.set_default_values=set_default_values_ipv6,
	.check_datagram=check_datagram_ipv6,
	.fill_packet_header=fill_packet_header_ipv6,
	.after_filling=after_filling_ipv6,
	.get_header_size = get_header_size_ipv6,
	.create_pseudo_header=create_pseudo_header_ipv6,
	.compute_checksum=compute_checksum_ipv6,
	.after_checksum=NULL,
	.generate_socket = generate_socket_ipv6,
};


ADD_PROTO (ipv6_ops);



