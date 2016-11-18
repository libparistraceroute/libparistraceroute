#include "use.h"
#include "config.h"

#include <stdlib.h>             // malloc
#include <stdio.h>              // perror
#include <unistd.h>             // close
#include <sys/socket.h>         // socket, getaddrinfo
#include <sys/types.h>          // getaddrinfo
#include <netdb.h>              // getaddrinfo
#include <arpa/inet.h>          // inet_pton
#include <string.h>             // memset

#include "socketpool.h"

#include "address.h"            // address_guess_family

/*
If we send UDP packet, we could get a return error channel.
Otherwise we require a raw socket in the sniffer to retrieve ICMP layers.
TCP: see scamper
*/

// TODO use sockaddr_storage ?
// http://stackoverflow.com/questions/8835322/api-using-sockaddr-storage
union sockaddr_union {
	struct sockaddr      sa;
#ifdef USE_IPV4
	struct sockaddr_in   sin;
#endif
#ifdef USE_IPV6
	struct sockaddr_in6  sin6;
#endif
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

    if ((sockfd = socket(family, SOCK_RAW, IPPROTO_RAW)) == -1) {
        perror("Cannot create a raw socket (are you root?)");
        goto ERR_SOCKET;
    }

	*psockfd = sockfd;
    return true;

ERR_SOCKET:
    return false;
}

socketpool_t * socketpool_create() {
    socketpool_t * socketpool;
    
    if (!(socketpool = malloc(sizeof(socketpool_t))))             goto ERR_MALLOC;
#ifdef USE_IPV4
    if (!(create_raw_socket(AF_INET,  &socketpool->ipv4_sockfd))) goto ERR_CREATE_RAW_SOCKET_IPV4;
#endif
#ifdef USE_IPV6
    if (!(create_raw_socket(AF_INET6, &socketpool->ipv6_sockfd))) goto ERR_CREATE_RAW_SOCKET_IPV6;
#endif
    return socketpool;

#ifdef USE_IPV6
ERR_CREATE_RAW_SOCKET_IPV6:
#ifdef USE_IPV4
    close(socketpool->ipv4_sockfd);
#endif
#endif
#ifdef USE_IPV4
ERR_CREATE_RAW_SOCKET_IPV4:
#endif
    free(socketpool);
ERR_MALLOC:
    return NULL;
}

void socketpool_free(socketpool_t * socketpool) {
    if (socketpool) {
#ifdef USE_IPV4
        if (close(socketpool->ipv4_sockfd) == -1) {
            perror("socketpool_free: Error while closing IPv4 socket");
        }
#endif
#ifdef USE_IPV6
        if (close(socketpool->ipv6_sockfd) == -1) {
            perror("socketpool_free: Error while closing IPv6 socket");
        }
#endif
        free(socketpool);
    }
}

bool socketpool_send_packet(const socketpool_t * socketpool, const packet_t * packet)
{
	sockaddr_u              sock;
    int                     sockfd;
    socklen_t               socklen;
    const struct sockaddr * dst_addr;
    
    memset(&sock, 0, sizeof(sockaddr_u));

    // Prepare socket 
    // We don't care about the dst_port set in the packet
    switch (packet->dst_ip->family) {
#ifdef USE_IPV4
        case AF_INET:
            sock.sin.sin_family = AF_INET;
            sock.sin.sin_addr   = packet->dst_ip->ip.ipv4;
            sockfd = socketpool->ipv4_sockfd;
            socklen = sizeof(struct sockaddr_in);
            dst_addr = (struct sockaddr *) &sock.sin;
            break;
#endif
#ifdef USE_IPV6
        case AF_INET6:
            sock.sin6.sin6_family = AF_INET6;
            memcpy(&sock.sin6.sin6_addr, &packet->dst_ip->ip.ipv6, sizeof(ipv6_t));
            sockfd = socketpool->ipv6_sockfd;
            socklen = sizeof(struct sockaddr_in6);
            dst_addr = (struct sockaddr *) &sock.sin6;
            break;
#endif
        default:
            fprintf(stderr, "socketpool_send_packet: Address family not supported\n");
            goto ERR_INVALID_FAMILY;
    }

    // Send the packet
    if (sendto(sockfd, packet_get_bytes(packet), packet_get_size(packet), 0, dst_addr, socklen) == -1) {
        perror("send_data: Sending error in queue");
        goto ERR_SEND_TO;
    }

    return true;

ERR_SEND_TO:
ERR_INVALID_FAMILY:
    return false;
}
