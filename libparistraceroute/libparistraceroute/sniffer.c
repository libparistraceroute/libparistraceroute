#include <stdlib.h>      // malloc
#include <stdio.h>       // perror
#include <string.h>      // memcpy, memset
#include <unistd.h>      // fnctl
#include <fcntl.h>       // fnctl
#include <sys/socket.h>  // socket, bind
#include <sys/types.h>   // socket, bind
#include <arpa/inet.h>
#include <netinet/in.h>  // IPPROTO_ICMP

#include "sniffer.h"

#define BUFLEN 4096

/**
 * \brief Initialize a raw socket in a sniffer_t instance
 * \param sniffer A pointer to a sniffer_t instance
 * \param port The listening port
 * \return true iif successful
 */

static bool sniffer_create_raw_socket(sniffer_t * sniffer, uint16_t port)
{
	struct sockaddr_in saddr;

	// Create a raw socket (man 7 ip)
	if ((sniffer->sockfd = socket(AF_INET, SOCK_RAW, IPPROTO_ICMP)) == -1) {
        perror("sniffer_create_raw_socket: error while creating socket");
        goto ERR_SOCKET;
    }

    // Make the socket non-blocking
    if (fcntl(sniffer->sockfd, F_SETFD, O_NONBLOCK) != 0) {
        goto ERR_FCNTL;
    }
	
	// Bind it to 0.0.0.0
	memset(&saddr, 0, sizeof(struct sockaddr_in));
	saddr.sin_family      = AF_INET;
	saddr.sin_addr.s_addr = INADDR_ANY;
	saddr.sin_port        = htons(port);

	if (bind(sniffer->sockfd, (struct sockaddr *) &saddr, sizeof(struct sockaddr_in)) != 0) {
        perror("sniffer_create_raw_socket: error while binding the socket");
        goto ERR_BIND;
    }

    return true;

ERR_BIND:
ERR_FCNTL:
    close(sniffer->sockfd);
ERR_SOCKET:
    return false;
}

sniffer_t * sniffer_create(void * recv_param, bool (*recv_callback)(packet_t *, void *))
{
    sniffer_t * sniffer;

    // TODO: We currently only listen for ICMP thanks to a raw socket which
    // requires root privileges
	// Can we set port to 0 to capture all packets wheter ICMP, UDP or TCP?
    if (!(sniffer = malloc(sizeof(sniffer_t)))) goto ERR_MALLOC;
    if (!sniffer_create_raw_socket(sniffer, 0)) goto ERR_CREATE_RAW_SOCKET;
    sniffer->recv_param = recv_param;
    sniffer->recv_callback = recv_callback;
    return sniffer;

ERR_CREATE_RAW_SOCKET:
    free(sniffer);
ERR_MALLOC:
    return NULL;
}

void sniffer_free(sniffer_t * sniffer)
{
    if (sniffer) {
        close(sniffer->sockfd);
        free(sniffer);
    }
}

int sniffer_get_sockfd(sniffer_t *sniffer) {
    return sniffer->sockfd;
}

void sniffer_process_packets(sniffer_t * sniffer)
{
    uint8_t    recv_bytes[BUFLEN];
    ssize_t    num_bytes;
    packet_t * packet;

	num_bytes = recv(sniffer->sockfd, recv_bytes, BUFLEN, 0);
	if (num_bytes >= 4) {
		// We have to make some modifications on the datagram
		// received because the raw format varies between
		// OSes:
		//  - Linux: the whole packet is in network endianess
		//  - NetBSD: the packet is in network endianess except
		//  IP total length and frag ofs(?) are in host-endian
		//  - FreeBSD: same as NetBSD?
		//  - Apple: same as NetBSD?
		//  Bug? On NetBSD, the IP length seems incorrect
#if defined __APPLE__ || __NetBSD__ || __FreeBSD__
		uint16_t ip_len = read16(recv_bytes, 2);
		writebe16(recv_bytes, 2, ip_len);
#endif
		if (sniffer->recv_callback != NULL) {
            packet = packet_create_from_bytes(recv_bytes, num_bytes);
			if (!(sniffer->recv_callback(packet, sniffer->recv_param))) {
                fprintf(stderr, "Error in sniffer's callback\n");
            }
        }
	}
}

