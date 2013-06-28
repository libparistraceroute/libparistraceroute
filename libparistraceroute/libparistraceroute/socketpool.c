#include <stdlib.h>             // malloc
#include <stdio.h>              // perror
#include <unistd.h>             // close
#include <sys/socket.h>         // socket, getaddrinfo
#include <sys/types.h>          // getaddrinfo
#include <netdb.h>              // getaddrinfo
#include <arpa/inet.h>          // inet_pton
#include <string.h>             // memset

#include "socketpool.h"
#include "address.h"


/*
If we send UDP packet, we could get a return error channel.
Otherwise we require a raw socket in the sniffer to retrieve ICMP layers.
TCP: see scamper
*/

// TODO use sockaddr_storage ?
// http://stackoverflow.com/questions/8835322/api-using-sockaddr-storage
union sockaddr_union {
	struct sockaddr      sa;
//	struct sockaddr_ll   sll;
	struct sockaddr_in   sin;
	struct sockaddr_in6  sin6;
};

typedef union sockaddr_union sockaddr_u;

/**
 * \brief Create a raw socket.
 * \param family Internet address family (AF_INET, AF_INET6).
 * \param psockfd Address of an integer, where the resulting socket
 *    file descriptor is written if successful
 * \return true iif successfull, false otherwise.
 */

static bool create_raw_socket(int family, int * psockfd) {
	int       sockfd;
	//int       optval = 1; // ???
    //socklen_t optlen = sizeof(optval);

    if ((sockfd = socket(family, SOCK_RAW, IPPROTO_RAW)) == -1) {
        perror("Cannot create a raw socket (are you root?)");
        goto ERR_SOCKET;
    }

    /*
	if (setsockopt(sockfd, IPPROTO_IP, IP_HDRINCL, &optval, optlen) < 0){
		perror("Cannot set socket options");
        goto ERR_SETSOCKOPT;
	}
    */

	*psockfd = sockfd;
    return true;

//ERR_SETSOCKOPT:
ERR_SOCKET:
    return false;
}

socketpool_t * socketpool_create() {
    socketpool_t * socketpool;
    
    if (!(socketpool = malloc(sizeof(socketpool_t))))             goto ERR_MALLOC;
    if (!(create_raw_socket(AF_INET,  &socketpool->ipv4_sockfd))) goto ERR_CREATE_RAW_SOCKET_IPV4;
    if (!(create_raw_socket(AF_INET6, &socketpool->ipv6_sockfd))) goto ERR_CREATE_RAW_SOCKET_IPV6;
    return socketpool;

ERR_CREATE_RAW_SOCKET_IPV6:
    close(socketpool->ipv4_sockfd);
ERR_CREATE_RAW_SOCKET_IPV4:
    free(socketpool);
ERR_MALLOC:
    return NULL;
}

void socketpool_free(socketpool_t * socketpool) {
    if (socketpool) {
        if (close(socketpool->ipv4_sockfd) == -1) {
            perror("socketpool_free: Error while closing IPv4 socket");
        }
        if (close(socketpool->ipv6_sockfd) == -1) {
            perror("socketpool_free: Error while closing IPv6 socket");
        }
        free(socketpool);
    }
}

bool socketpool_send_packet(const socketpool_t * socketpool, const packet_t * packet)
{
	sockaddr_u              sock;
    int                     family;
    int                     sockfd;
    socklen_t               socklen;
    const struct sockaddr * dst_addr;
    
    if (!(address_guess_family(packet->dst_ip, &family))) {
        return false;
    }
    memset(&sock, 0, sizeof(sockaddr_u));

    // Prepare socket 
    switch (family) {
        case AF_INET:
            sock.sin.sin_family = AF_INET;
            sock.sin.sin_port   = htons(packet->dst_port);
            inet_pton(AF_INET, packet->dst_ip, &sock.sin.sin_addr);
            sockfd = socketpool->ipv4_sockfd;
            socklen = sizeof(struct sockaddr_in);
            dst_addr = &sock.sin;
            break;
        case AF_INET6:
            sock.sin6.sin6_family = AF_INET6;
            // TODO mmmh we need to set port to 0... why?
            sock.sin6.sin6_port   = 0; // htons(packet->dst_port);
            inet_pton(AF_INET6, packet->dst_ip, &sock.sin6.sin6_addr);
            sockfd = socketpool->ipv6_sockfd;
            socklen = sizeof(struct sockaddr_in6);
            dst_addr = &sock.sin6;
            break;
        default:
            fprintf(stderr, "socketpool_send_packet: Address family not supported");
            return false; 
    }

    // Send the packet
    if (sendto(sockfd, packet_get_bytes(packet), packet_get_size(packet), 0, dst_addr, socklen) == -1) {
        perror("send_data: Sending error in queue");
        return false; 
    }

    return true;
}
