#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/types.h>
#include <errno.h>
#include <string.h>
#include <stdio.h>
#include <arpa/inet.h>
#include <unistd.h>

#include <stdlib.h>
#include <fcntl.h>

#include "sniffer.h"
#include "network.h"

#define BUFLEN 4096

int sniffer_create_raw_socket(sniffer_t *sniffer)
{
	struct sockaddr_in saddr;
    unsigned short port = 0; /* could be a parameter */
    int res;

	// Create a raw socket (man 7 ip)
    // TODO we currently only listen for ICMP
	sniffer->socket  = socket(AF_INET, SOCK_RAW, IPPROTO_ICMP);
	if (sniffer->socket < 0) {
        perror("ERROR creating socket");
		return -1;
    }

    /* Make the socket non-blocking */
    res = fcntl(sniffer->socket, F_SETFD, O_NONBLOCK);
    if (res != 0) {
        printf("ERROR making it non blocking\n");
        return -1;
    }
	
	// Bind it to 0.0.0.0
	// Can we set port to 0 to capture all packets wheter ICMP, UDP or TCP?
	memset(&saddr, 0, sizeof(struct sockaddr_in));
	saddr.sin_family      = AF_INET;
	saddr.sin_addr.s_addr = INADDR_ANY;
	saddr.sin_port        = htons(port);
	res = bind(sniffer->socket, (struct sockaddr*)&saddr,
			sizeof(struct sockaddr_in));
	if (res < 0) {
        printf("ERROR binding\n");
		return -1;
    }

    printf("SNIFFER SUCCESSFULLY CREATED %d\n", sniffer->socket);
	
    return 0;
}

sniffer_t * sniffer_create(network_t *network, void (*callback)(network_t *network, packet_t *packet))
{
    sniffer_t *sniffer;
    int res;

    sniffer = malloc(sizeof(sniffer_t));
    sniffer->network = network;
    sniffer->callback = callback;
    printf("create raw\n");
    res = sniffer_create_raw_socket(sniffer);
    if (res == -1)
        goto error;
    printf("RAW OK\n");
    return sniffer;

error:
    printf("RAW ERROR\n");
    free(sniffer);
    sniffer = NULL;
    return NULL;
}

void sniffer_free(sniffer_t *sniffer)
{
    close(sniffer->socket);

    free(sniffer);
    sniffer = NULL;
}

int sniffer_get_fd(sniffer_t *sniffer)
{
    printf("sniffer_get_fd\n");
    return sniffer->socket;
}

void sniffer_process_packets(sniffer_t *sniffer)
{
    unsigned char data[BUFLEN];
    int data_len;

    printf("sniffer: before recv\n");
	data_len = recv(sniffer->socket, data, BUFLEN, 0);
    printf("sniffer: after recv (data_len = %d\n", data_len);
	if (data_len >= 4) {
		// We have to make some modifications on the datagram
		// received because the raw format varies between
		// OSes:
		//  - Linux: the whole packet is in network endianess
		//  - NetBSD: the packet is in network endianess ecxept
		//  IP total length and frag ofs(?) are in host-endian
		//  - FreeBSD: same as NetBSD?
		//  - Apple: same as NetBSD?
		//  Bug? On NetBSD, the IP length seems incorrect
#if defined __APPLE__ || __NetBSD__ || __FreeBSD__
		uint16_t ip_len = read16(data, 2);
		writebe16(data, 2, ip_len);
#endif
		if (sniffer->callback != NULL) {
            buffer_t *buffer;
            packet_t *packet;

            buffer = buffer_create();
            buffer_set_data(buffer, data);
            buffer_set_size(buffer, data_len);

            packet = packet_create();
            packet_set_buffer(packet, buffer);
			sniffer->callback(sniffer->network, packet); 
        }
	}

}
