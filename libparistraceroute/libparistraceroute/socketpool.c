#include <stdlib.h>             // malloc
#include <stdio.h>              // perror
#include <unistd.h>             // close
#include <sys/socket.h>         // socket, getaddrinfo
#include <sys/types.h>          // getaddrinfo
#include <netdb.h>              // getaddrinfo
#include <arpa/inet.h>          // inet_pton

#include "socketpool.h"
#include "address.h"


/*
If we send UDP packet, we could get a return error channel.
Otherwise we require a raw socket in the sniffer to retrieve ICMP layers.
TCP: see scamper
*/

union sockaddr_union {
	struct sockaddr      sa;
//	struct sockaddr_ll   sll;
	struct sockaddr_in   sin;
	struct sockaddr_in6  sin6;
};

typedef union sockaddr_union sockaddr_u;

bool socketpool_create_raw_socket(socketpool_t *socketpool)
{
	int sockfd;
	int one = 1; // ???

    if ((sockfd = socket(AF_INET, SOCK_RAW, IPPROTO_RAW)) == -1) {
        perror("Cannot create a raw socket (are you root?)");
        goto ERR_SOCKET;
    }

	if (setsockopt(sockfd, IPPROTO_IP, IP_HDRINCL, &one, sizeof(int)) < 0){
		perror("Cannot set socket options");
        goto ERR_SETSOCKOPT;
	}

	socketpool->socket = sockfd;
    return true;

ERR_SETSOCKOPT:
ERR_SOCKET:
    return false;
}

socketpool_t * socketpool_create(void)
{
    socketpool_t * socketpool;
    
    if (!(socketpool = malloc(sizeof(socketpool_t)))) goto ERR_MALLOC;
    if (!(socketpool_create_raw_socket(socketpool)))  goto ERR_CREATE_RAW_SOCKET;
    return socketpool;

ERR_CREATE_RAW_SOCKET:
    free(socketpool);
ERR_MALLOC:
    return NULL;
}

void socketpool_free(socketpool_t * socketpool)
{
	if (close(socketpool->socket) < 0) {
		perror("socketpool_free: error while closing socket");
	}
    free(socketpool);
}

bool socketpool_send_packet(const socketpool_t * socketpool, const packet_t * packet)
{
    size_t            size;
	sockaddr_u        sock;
    int               family;
    
    if (!(address_guess_family(packet->dst_ip, &family))) {
        return false;
    }

    // Prepare socket 
    switch (family) {
        case AF_INET:
            sock.sin.sin_family = AF_INET;
            sock.sin.sin_port   = htons(packet->dst_port);
            inet_pton(AF_INET, packet->dst_ip, &sock.sin.sin_addr);
            break;
        case AF_INET6:
            sock.sin6.sin6_family = AF_INET6;
            sock.sin6.sin6_port   = htons(packet->dst_port);
            inet_pton(AF_INET6, packet->dst_ip, &sock.sin6.sin6_addr);
            break;
        default:
            perror("Invalid address family");
            return false; 
    }

    // Send the packet
    size = packet_get_size(packet);
    if (sendto(socketpool->socket, packet_get_bytes(packet), size, 0, (struct sockaddr *) &sock, sizeof(sock)) < 0){
        perror("send_data: sending error in queue");
        return false; 
    }

    return true;
}
