#include "queue.h"

#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <netinet/ip6.h>
#include <arpa/inet.h>
#include <netpacket/packet.h>
#include <net/ethernet.h> 
#include <unistd.h>


int bit_per_send = 0;

void set_flow_rate(int new_bit_per_send){
	bit_per_send = new_bit_per_send;
}


int init_raw_socket(){
	//int s = socket(AF_PACKET, SOCK_DGRAM, htons(ETH_P_ALL));
	int s = socket(AF_INET, SOCK_RAW, IPPROTO_RAW);
    if (s < 0) {
        perror("init_raw_socket :: Error creating socket :");
        return -1;
    }
    printf("created socket %d\n", s);
	int one = 1;
	if (setsockopt(s,IPPROTO_IP,IP_HDRINCL,&one,sizeof(int)) < 0){
		perror("init_raw_socket : Cannot set IP_HDRINCL option :");
		return -1;
	}
	return s;

}
int send_data(char* data, int data_size, int socket, sockaddr_u addr){
	#ifdef DEBUG
	fprintf(stdout,"send_data : sending %d data in socket %d\n",data_size,socket);
	#endif
	if(bit_per_send==0){
		if (sendto (socket, data, data_size,0,(struct sockaddr *) &addr,sizeof (addr)) < 0){
			#ifdef DEBUG
			perror ("send_data : sending error in queue ");
			#endif
			return -1;
		}
	}
	else{//send to this rate TODO : keep this?
		int count = data_size;
		while(count > bit_per_send){
			if (sendto (socket, data+(data_size-count), bit_per_send,0,(struct sockaddr *) &addr,sizeof (addr)) < 0){
				#ifdef DEBUG
				perror("send_data : sending error in queue ");
				#endif
				return -1;
				count = 0;//to stop
			}
			else{
				count = count - bit_per_send;
			}
		}
		if (sendto (socket, data+(data_size-count), count,0,(struct sockaddr *) &addr,sizeof (addr)) < 0){
			#ifdef DEBUG
			perror("send_data : sending error in queue ");
			#endif
			return -1;
		}
	}
	return 1;

}

 /*    struct addrinfo {                                                  
          int     ai_flags;      //AI_PASSIVE, AI_CANONNAME, AI_NUMERICHOST 
          int     ai_family;     //PF_xxx 
          int     ai_socktype;  // SOCK_xxx 
          int     ai_protocol;  // 0 or IPPROTO_xxx for IPv4 and IPv6 
          size_t  ai_addrlen;   // length of ai_addr 
          char   *ai_canonname; // canonical name for nodename 
          struct sockaddr  *ai_addr; // binary address 
          struct addrinfo  *ai_next; // next structure in linked list 
     };*/

sockaddr_u* fill_sockaddr_from_sockaddrinfo(struct addrinfo* addrinf,short dest_port,char* dest_addr){
	if(addrinf==NULL){
		#ifdef DEBUG
			fprintf(stderr,"fill_sockaddr : error : getting information from address failed. returning NULL sockaddr.\n");
		#endif
	}
	sockaddr_u* sock = malloc(sizeof(sockaddr_u));
	if(addrinf->ai_family==AF_INET){//IPv4
		#ifdef DEBUG
			fprintf(stdout,"fill_sockaddr : IPv4 address.\n");
		#endif
		sock->sin.sin_family=AF_INET;
		sock->sin.sin_port = htons(dest_port);
		inet_pton(AF_INET, dest_addr, &sock->sin.sin_addr);
		return sock;
	}
	if(addrinf->ai_family==AF_INET6){//IPv6
		#ifdef DEBUG
			fprintf(stdout,"fill_sockaddr : IPv6 address.\n");
		#endif
		sock->sin6.sin6_family=AF_INET6;
		sock->sin6.sin6_port = htons(dest_port);
		inet_pton(AF_INET6, dest_addr, &sock->sin6.sin6_addr);
		return sock;
	}
	#ifdef DEBUG
		fprintf(stderr,"fill_sockaddr : error : unknown kind of family : %d. returning NULL sockaddr.\n",addrinf->ai_family);
	#endif
	return NULL;
}

sockaddr_u* fill_sockaddr_from_fields(fieldlist_s* fields){
	char* dest_addr=NULL;
	dest_addr = (char*) get_field_value_from_fieldlist(fields, "dest_addr");
	if(dest_addr==NULL){
		#ifdef DEBUG
		fprintf(stderr,"fill_sockaddr : error : destination address not found\n");
		#endif
		return NULL;
	}
	else{
		#ifdef DEBUG
		fprintf(stdout,"fill_sockaddr : destination address : %s\n",dest_addr);
		#endif
	}
	short* dest_port = (short*) get_field_value_from_fieldlist(fields, "dest_port");
	char dest_port_char[6] = "";//TODO : max 65535?
	if(dest_port==NULL){
		#ifdef DEBUG
		fprintf(stderr,"fill_sockaddr : warning : destination port not found\n");
		#endif
	}
	else{
		sprintf(dest_port_char, "%d",*dest_port);
	}
	struct addrinfo* res = malloc(sizeof(struct addrinfo));
	#ifdef DEBUG
	fprintf(stdout, "fill_sockaddr : getting info with getaddrinfo\n");
	#endif
	int get_error = getaddrinfo(dest_addr, dest_port_char,NULL, &res);
	if(get_error!=0){
		#ifdef DEBUG
		fprintf(stderr, "fill_sockaddr : getaddrinfo: %s\n", gai_strerror(get_error));
		#endif
		return NULL;
	}
	#ifdef DEBUG
	fprintf(stdout,"fill_sockaddr : using addrinfo to fill sockaddr\n");
	#endif
	sockaddr_u* sockaddr = fill_sockaddr_from_sockaddrinfo(res,*dest_port,dest_addr);
	if (sockaddr==NULL){
		return NULL;
	}
	return sockaddr;

}

//When you send packets it is enough to specify sll_family, sll_addr, sll_halen, sll_ifindex. 
sockaddr_u* fill_sockaddr_ll(){
	sockaddr_u* sock_ll = malloc(sizeof(sockaddr_u));
	#ifdef DEBUG
	fprintf(stdout,"fill_sockaddr_ll : filling sockaddr info.\n");
	#endif
	sock_ll->sll.sll_family = AF_PACKET;
	sock_ll->sll.sll_protocol = htons(ETH_P_ALL);
	sock_ll->sll.sll_ifindex = 0;//TODO verif
	sock_ll->sll.sll_pkttype = PACKET_OTHERHOST|PACKET_BROADCAST|PACKET_MULTICAST|PACKET_HOST;
	sock_ll->sll.sll_halen = ETH_ALEN;
	sock_ll->sll.sll_hatype = 0x0000;
	return sock_ll;
}

int close_socket(int socket){
	if(close(socket)<0){
		perror("close_socket");
		return -1;
	}
	return 1;
}
