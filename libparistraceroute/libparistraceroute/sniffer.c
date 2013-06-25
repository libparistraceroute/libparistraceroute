#include <stdlib.h>      // malloc
#include <stdio.h>       // perror
#include <string.h>      // memcpy, memset
#include <unistd.h>      // fnctl
#include <fcntl.h>       // fnctl
#include <sys/socket.h>  // socket, bind, 
#include <sys/types.h>   // socket, bind
#include <arpa/inet.h>
#include <netinet/in.h>  // IPPROTO_ICMP, IPPROTO_ICMPV6
#include <netinet/ip6.h> // ip6_hdr

#include "sniffer.h"

#define BUFLEN 4096

// Some implementation does not respects RFC3542
// http://livre.g6.asso.fr/index.php/L'implémentation
#ifndef IPV6_RECVHOPLIMIT
#  define IPV6_RECVHOPLIMIT IPV6_HOPLIMIT
#endif
#ifndef IPV6_RECVPKTINFO
#  define IPV6_RECVPKTINFO IPV6_PKTINFO
#endif


// From /usr/include/netinet/in.h
struct in6_pktinfo
{
    struct in6_addr ipi6_addr;  /* src/dst IPv6 address */
    unsigned int ipi6_ifindex;  /* send/recv interface index */
};

/**
 * \brief Initialize an ICMPv4 raw socket in a sniffer_t instance
 * \param sniffer A pointer to a sniffer_t instance
 * \param port The listening port
 * \return true iif successful
 */

static bool create_icmpv4_socket(sniffer_t * sniffer, uint16_t port)
{
	struct sockaddr_in saddr;

	// Create a raw socket (man 7 ip) listening ICMPv4 packets
	if ((sniffer->icmpv4_sockfd = socket(AF_INET, SOCK_RAW, IPPROTO_ICMP)) == -1) {
        perror("create_icmpv4_socket: error while creating socket");
        goto ERR_SOCKET;
    }

    // Make the socket non-blocking
    if (fcntl(sniffer->icmpv4_sockfd, F_SETFD, O_NONBLOCK) == -1) {
        goto ERR_FCNTL;
    }
	
	// Bind it to 0.0.0.0
	memset(&saddr, 0, sizeof(struct sockaddr_in));
	saddr.sin_family      = AF_INET;
	saddr.sin_addr.s_addr = INADDR_ANY;
	saddr.sin_port        = htons(port);

	if (bind(sniffer->icmpv4_sockfd, (struct sockaddr *) &saddr, sizeof(struct sockaddr_in)) == -1) {
        perror("create_icmpv4_socket: error while binding the socket");
        goto ERR_BIND;
    }

    return true;

ERR_BIND:
ERR_FCNTL:
    close(sniffer->icmpv4_sockfd);
ERR_SOCKET:
    return false;
}

/**
 * \brief Initialize an ICMPv6 raw socket in a sniffer_t instance
 * \param sniffer A pointer to a sniffer_t instance
 * \param port The listening port
 * \return true iif successful
 */

static bool create_icmpv6_socket(sniffer_t * sniffer, uint16_t port)
{
    struct in6_addr anyaddr = IN6ADDR_ANY_INIT;
    struct sockaddr_in6 saddr;
    int    on = 1;

	// Create a raw socket (man 7 ip) listening ICMPv6 packets
    if ((sniffer->icmpv6_sockfd = socket(AF_INET6, SOCK_RAW, IPPROTO_ICMPV6)) == -1) {
        perror("create_icmpv6_socket: error while creating socket");
        goto ERR_SOCKET;
    }

    // Make the socket non-blocking
    if (fcntl(sniffer->icmpv6_sockfd, F_SETFD, O_NONBLOCK) == -1) {
        goto ERR_FCNTL;
    }

    // IPV6 socket options we actually need this for reconstruction of an IPv6 Packet lateron
    // - dst_ip + arriving interface
    // - TCL
    // - Hoplimit
    // http://h71000.www7.hp.com/doc/731final/tcprn/v53_relnotes_025.html
    // http://livre.g6.asso.fr/index.php/L'impl%C3%A9mentation

    if ((setsockopt(sniffer->icmpv6_sockfd, IPPROTO_IPV6, IPV6_RECVPKTINFO,  &on, sizeof(on)) == -1) // struct in6_pktinfo
    ||  (setsockopt(sniffer->icmpv6_sockfd, IPPROTO_IPV6, IPV6_RECVHOPLIMIT, &on, sizeof(on)) == -1) // int
    ||  (setsockopt(sniffer->icmpv6_sockfd, IPPROTO_IPV6, IPV6_RECVRTHDR,    &on, sizeof(on)) == -1) // struct ip6_rthdr
    ||  (setsockopt(sniffer->icmpv6_sockfd, IPPROTO_IPV6, IPV6_RECVHOPOPTS,  &on, sizeof(on)) == -1) // struct ip6_hbh
    ||  (setsockopt(sniffer->icmpv6_sockfd, IPPROTO_IPV6, IPV6_RECVDSTOPTS,  &on, sizeof(on)) == -1) // struct ip6_dest
    ||  (setsockopt(sniffer->icmpv6_sockfd, IPPROTO_IPV6, IPV6_RECVTCLASS,   &on, sizeof(on)) == -1) // int
    ) {
        perror("create_icmpv6_socket: error in setsockopt");
        goto ERR_SETSOCKOPT;
    }

    // Bind to ::1
    saddr.sin6_family = AF_INET6;
    saddr.sin6_addr   = anyaddr;
    saddr.sin6_port   = htons(port);

    if (bind(sniffer->icmpv6_sockfd, (struct sockaddr *) &saddr, sizeof(struct sockaddr_in6)) == -1) {
        perror("create_icmpv6_socket: error while binding the socket");
        goto ERR_BIND;
    }

    return true;

ERR_BIND:
ERR_SETSOCKOPT:
ERR_FCNTL:
    close(sniffer->icmpv6_sockfd);
ERR_SOCKET:
    return false;
}


sniffer_t * sniffer_create(void * recv_param, bool (*recv_callback)(packet_t *, void *))
{
    sniffer_t * sniffer;

    // TODO: We currently only listen for ICMP thanks to raw sockets which
    // requires root privileges
	// Can we set port to 0 to capture all packets wheter ICMP, UDP or TCP?
    if (!(sniffer = malloc(sizeof(sniffer_t)))) goto ERR_MALLOC;
    if (!create_icmpv4_socket(sniffer, 0))      goto ERR_CREATE_ICMPV4_SOCKET;
    if (!create_icmpv6_socket(sniffer, 0))      goto ERR_CREATE_ICMPV6_SOCKET;
    sniffer->recv_param = recv_param;
    sniffer->recv_callback = recv_callback;
    return sniffer;

ERR_CREATE_ICMPV6_SOCKET:
    close(sniffer->icmpv4_sockfd);
ERR_CREATE_ICMPV4_SOCKET:
    free(sniffer);
ERR_MALLOC:
    return NULL;
}

void sniffer_free(sniffer_t * sniffer)
{
    if (sniffer) {
        close(sniffer->icmpv4_sockfd);
        close(sniffer->icmpv6_sockfd);
        free(sniffer);
    }
}

int sniffer_get_icmpv4_sockfd(sniffer_t *sniffer) {
    return sniffer->icmpv4_sockfd;
}

int sniffer_get_icmpv6_sockfd(sniffer_t *sniffer) {
    return sniffer->icmpv6_sockfd;
}

/*
#include <netinet/ip6.h> // ip6_hdr
static ssize_t recv_ipv6(int ipv6_sockfd, uint8_t * recv_bytes, size_t len, int flags) {
    // Here the IPv6 Madness starts. Unlike IPv4 we will NOT receive full packets, but only the payload of the ICMP6 packet.
    // Therefore we need to get a bunch of auxilliary data to keep in touch with the IPv4 structure paris-traceroute mainly has.
    // The whole structure needs to be seriously changed to become protocol agnostic :(
    ssize_t          num_bytes;
    struct ip6_hdr * ip6_hed = malloc(sizeof(struct ip6_hdr));
    struct in6_addr  src_ip;

    ip6_hed->ip6_ctlun.ip6_un1.ip6_un1_flow = (uint32_t) 6; // setting IPv6 Version, TCL, Flow... No chance to get flow info from incoming packet? Seriously?
    ip6_hed->ip6_ctlun.ip6_un1.ip6_un1_nxt  = (uint8_t) IPPROTO_ICMPV6; // We already know it is ICMP6, due to filter options

    struct iovec iov[1];
    iov[0].iov_base = recv_bytes;
    iov[0].iov_len  = len;

    uint8_t          cmsgbuf[BUFLEN];
    struct cmsghdr * cmsg;
    struct msghdr    msg;

    msg.msg_name = &src_ip;
    msg.msg_namelen = sizeof(src_ip);
    msg.msg_iov = iov;
    msg.msg_iovlen = 1;
    msg.msg_control = cmsgbuf;
    msg.msg_controllen = sizeof(cmsgbuf);

    num_bytes = recvmsg(ipv6_sockfd, &msg, 0);
    printf("num_bytes = %ld\n", num_bytes);

    ////////////////
    ip6_hed->ip6_ctlun.ip6_un1.ip6_un1_plen = htons(num_bytes);

    for (cmsg = CMSG_FIRSTHDR(&msg); cmsg != NULL; cmsg = CMSG_NXTHDR(&msg, cmsg)) {
        if (cmsg->cmsg_type == IPV6_PKTINFO) {
            memcpy(&ip6_hed->ip6_dst, CMSG_DATA(cmsg), sizeof(struct in6_addr));
        } else if ( cmsg->cmsg_type == IPV6_HOPLIMIT) {
            memcpy(&ip6_hed->ip6_ctlun.ip6_un1.ip6_un1_hlim, CMSG_DATA(cmsg), sizeof(uint8_t));
        }
    }

    //  Need to get
    struct sockaddr_in6 *in6 = msg.msg_name;
    memcpy(&ip6_hed->ip6_src, &in6->sin6_addr , sizeof(struct in6_addr));
    ///////////////////
    uint8_t *tmpbuf = malloc(num_bytes + sizeof(struct ip6_hdr));
    memcpy(tmpbuf, ip6_hed, sizeof(struct ip6_hdr));
    memcpy(tmpbuf + sizeof(struct ip6_hdr), recv_bytes, num_bytes);
    ///////////////////

    return num_bytes;
}
*/

ssize_t recv_ipv6_header(int sockfd, void * bytes, size_t len, int flags) {
    ssize_t           num_bytes;
    char              cmsg_buf[BUFLEN];
    struct cmsghdr  * cmsg;
    struct in6_addr   src_ip; // sockaddr_in6
    struct ip6_hdr  * ip6_header = (struct ip6_hdr *) bytes;

    struct iovec iov = {
        .iov_base = bytes,
        .iov_len  = len 
    };

    struct msghdr msg = {
        .msg_name       = &src_ip,          // socket address
        .msg_namelen    = sizeof(src_ip),   // sizeof socket
        .msg_iov        = &iov,             // buffer (scather/gather array)
        .msg_iovlen     = 1,                // number of msg_iov elements
        .msg_control    = cmsg_buf,         // auxiliary data
        .msg_controllen = sizeof(cmsg_buf), // sizeof auxiliary data
        .msg_flags      = flags             // flags related to recv messages
    };

    memset(bytes, 0, sizeof(struct ip6_hdr)); // DEBUG
    buffer_t buffer;
    buffer.data = bytes;
    buffer.size = sizeof(struct ip6_hdr);
    buffer_dump(&buffer);
    printf("\n");

    if ((num_bytes = recvmsg(sockfd, &msg, flags)) == -1) {
        fprintf(stderr, "recv_ipv6_header: Can't fetch data\n");
        goto ERR_RECVMSG;
    }

    buffer_dump(&buffer);
    printf("\n");
    printf("num_bytes = %ld size expected = %ld\n", num_bytes, sizeof(struct ip6_hdr)); 

    // len
    printf("> len\n");
    ip6_header->ip6_ctlun.ip6_un1.ip6_un1_plen = htons(num_bytes);
    buffer_dump(&buffer);
    printf("\n");

    // src_ip
    printf("> src_ip\n");
    struct sockaddr_in6 *in6 = msg.msg_name;
    memcpy(&ip6_header->ip6_src, &in6->sin6_addr , sizeof(struct in6_addr));
    buffer_dump(&buffer);
    printf("\n");

    if (msg.msg_flags & MSG_TRUNC) {
        fprintf(stderr, "recv_ipv6_header: data truncated\n");
        goto ERR_MSG_TRUNC;
    }

    if (msg.msg_flags & MSG_CTRUNC) {
        fprintf(stderr, "recv_ipv6_header: auxiliary data truncated\n");
        goto ERR_MSG_CTRUNK;
    }


    // Fetch each auxiliary data
    struct in6_pktinfo * pi;

    printf("Fetching auxiliary data\n");
    for (cmsg = CMSG_FIRSTHDR(&msg); cmsg; cmsg = CMSG_NXTHDR(&msg, cmsg)) {
        if (cmsg->cmsg_level == IPPROTO_IPV6) {
            switch(cmsg->cmsg_type) {
                case IPV6_PKTINFO: // dst_ip
                    printf("> dst_ip IPV6_PKTINFO\n"); 
                    pi = (struct in6_pktinfo *) CMSG_DATA(cmsg);
                    memcpy(&(ip6_header->ip6_dst), &(pi->ipi6_addr), sizeof(struct in6_addr));
                    break;
                case IPV6_HOPLIMIT: // ttl
                    printf("> ttl IPV6_HOPLIMIT\n"); 
                    ip6_header->ip6_ctlun.ip6_un1.ip6_un1_hlim = *(int *) CMSG_DATA(cmsg);
                    break;
                default:
                    printf("> Ignoring cmsg %d\n", cmsg->cmsg_type);
                    break;
            }
            buffer_dump(&buffer);
            printf("\n");
        } else {
            printf("Ignoring msg (level = %d\n)", cmsg->cmsg_level);
        }
    }

    return num_bytes;
ERR_MSG_CTRUNK:
ERR_MSG_TRUNC:
ERR_RECVMSG:
    return 0;
}

void sniffer_process_packets(sniffer_t * sniffer, uint8_t protocol_id)
{
    uint8_t    recv_bytes[BUFLEN];
    ssize_t    num_bytes;
    packet_t * packet;

    switch (protocol_id) {
        case IPPROTO_ICMP:
            num_bytes = recv(sniffer->icmpv4_sockfd, recv_bytes, BUFLEN, 0);
            break;
        case IPPROTO_ICMPV6:
            num_bytes =  recv_ipv6_header(sniffer->icmpv6_sockfd, recv_bytes, BUFLEN, 0);
            num_bytes += recv(sniffer->icmpv6_sockfd + num_bytes, recv_bytes, BUFLEN - num_bytes, 0);
            break;
    }

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

            ///////// DEBUG
            if (protocol_id == IPPROTO_ICMPV6) {
                printf(">>>>>>>>>>>> Packet sniffed\n");
                // In IPv6 recv only fetches bytes starting from the ICMPv6 layer! 
                // http://h71000.www7.hp.com/doc/731final/tcprn/v53_relnotes_025.html
                printf("num_bytes = %ld\n", num_bytes);
                packet_dump(packet);
                printf(">>>>>>>>>>>>> Calling sniffer callback\n");
            }
            ///////// DEBUG

			if (!(sniffer->recv_callback(packet, sniffer->recv_param))) {
                fprintf(stderr, "Error in sniffer's callback\n");
            }
        }
	}
}

