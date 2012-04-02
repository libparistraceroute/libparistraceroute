#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <netpacket/packet.h>
#include <net/ethernet.h> 
#include <netdb.h>
#include <arpa/inet.h>

#include "socketpool.h"

// This was quickly hacked... TODO properly

union sockaddr_union {
	struct sockaddr sa;
	struct sockaddr_ll sll;
	struct sockaddr_in sin;
	struct sockaddr_in6 sin6;
};
typedef union sockaddr_union sockaddr_u;

int socketpool_create_raw_socket(socketpool_t *socketpool)
{
	int s = socket(AF_INET, SOCK_RAW, IPPROTO_RAW);
	int one = 1;
    if (s < 0) {
        perror("init_raw_socket :: Error creating socket :");
        return -1;
    }
	if (setsockopt(s,IPPROTO_IP,IP_HDRINCL,&one,sizeof(int)) < 0){
		perror("init_raw_socket : Cannot set IP_HDRINCL option :");
        return -1;
	}
	socketpool->socket = s;
    return 0;
}

socketpool_t *socketpool_create(void)
{
    socketpool_t *socketpool;
    unsigned int ret;
    
    socketpool = (socketpool_t*)malloc(sizeof(socketpool_t));
    ret = socketpool_create_raw_socket(socketpool);
    if (ret == -1) {
        free(socketpool);
        socketpool = NULL;
    }

    return socketpool;
}

void socketpool_free(socketpool_t *socketpool)
{
	if(close(socketpool->socket)<0){
		perror("close_socket");
	}
    free(socketpool);
    socketpool = NULL;
}

int socketpool_send_packet(socketpool_t *socketpool, packet_t *packet)
{
	struct addrinfo* addrinf;
	int get_error;
    size_t size;
	sockaddr_u sock;
    
    addrinf = malloc(sizeof(struct addrinfo));

    /* determine the type of IP address */
    get_error = getaddrinfo((char*)packet->dip, NULL, NULL, &addrinf);
	if(get_error!=0){
		fprintf(stderr, "fill_sockaddr : getaddrinfo: %s\n", gai_strerror(get_error));
		return -1; // XXX
	}

    /* Currently only IPv4 and IPv6 are supported */
	if(addrinf->ai_family==AF_INET){//IPv4
		sock.sin.sin_family=AF_INET;
		sock.sin.sin_port = htons(packet->dport);
		inet_pton(AF_INET, (char*)packet->dip, &sock.sin.sin_addr);
	} else if(addrinf->ai_family==AF_INET6){//IPv6
		sock.sin6.sin6_family=AF_INET6;
		sock.sin6.sin6_port = htons(packet->dport);
		inet_pton(AF_INET6, (char*)packet->dip, &sock.sin6.sin6_addr);
	} else {
        return -1; // error
    }
    // here we have the sockaddr

	//probe_set_sending_time( probe, get_time ());

    size = buffer_get_size(packet->buffer);
    if (sendto (socketpool->socket, buffer_get_data(packet->buffer), size, 0, (struct sockaddr *) &sock, sizeof (sock)) < 0){
        perror ("send_data : sending error in queue ");
        return -1;
    }

    return 0;
}
