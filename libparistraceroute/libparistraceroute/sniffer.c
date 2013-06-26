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

// Solaris/Sun
// http://livre.g6.asso.fr/index.php/L%27exemple_%C2%AB_mini-ping_%C2%BB_revisit%C3%A9
#ifdef sun // Solaris
#  define _XOPEN_SOURCE 500 // correct recvmsg/sendmsg/msg/CMSG_xx syntax
#  define __EXTENSIONS__
#  ifndef CMSG_SPACE // Solaris <= 9
#    define CMSG_SPACE(l) ((size_t)_CMSG_HDR_ALIGN(sizeof (struct cmsghdr) + (l)))
#    define CMSG_LEN(l) ((size_t)_CMSG_DATA_ALIGN(sizeof (struct cmsghdr)) + (l))
#  endif
#endif

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

/**
 *
 */

static bool rebuild_ipv6_header(
    struct ip6_hdr            * ip6_header,
    struct msghdr             * msg,
    const struct sockaddr_in6 * from,
    ssize_t                     num_bytes
) {
    bool                 ret = true;
    struct cmsghdr     * cmsg;
    struct in6_pktinfo * pktinfo;
    uint32_t             tcl;

    // Now we can rebuild the IPv6 layer
    // ip_version (hardcoded), traffic class (updated later), flow label (hardcoded) 
    ip6_header->ip6_ctlun.ip6_un1.ip6_un1_flow = htonl(0x60000000);

    // length
    ip6_header->ip6_ctlun.ip6_un1.ip6_un1_plen = htons(num_bytes);

    // src_ip
    memcpy(&ip6_header->ip6_src, &(from->sin6_addr), sizeof(struct in6_addr));

    // protocol
    ip6_header-> ip6_ctlun.ip6_un1.ip6_un1_nxt = IPPROTO_ICMPV6;

    // Fetch ancillary data (e.g last parts of the IPv6 header)
    for (cmsg = CMSG_FIRSTHDR(msg); cmsg; cmsg = CMSG_NXTHDR(msg, cmsg)) {
        if (cmsg->cmsg_level == IPPROTO_IPV6) {
            switch (cmsg->cmsg_type) {
                // Possible values: http://www.lehman.cuny.edu/cgi-bin/man-cgi?socket.h+3
                case IPV6_PKTINFO: // dst_ip
                    pktinfo = (struct in6_pktinfo *) CMSG_DATA(cmsg);
                    memcpy(&(ip6_header->ip6_dst), &(pktinfo->ipi6_addr), sizeof(struct in6_addr));
                    break;
                case IPV6_HOPLIMIT: // ttl
                    ip6_header->ip6_ctlun.ip6_un1.ip6_un1_hlim = *(uint8_t *) CMSG_DATA(cmsg);
                    break;
                case IPV6_TCLASS: // traffic class
                    tcl = *(uint8_t *) CMSG_DATA(cmsg);
                    *((uint32_t *) &(ip6_header->ip6_ctlun.ip6_un1.ip6_un1_flow)) |= htonl(tcl << 8);
                    break;
                default:
                    // This should never occur
                    fprintf(stderr, "Unhandled cmsg of type %d\n", cmsg->cmsg_type);
                    ret = false;
                    break;
            }
        } else {
            // This should never occur
            fprintf(stderr, "Ignoring msg (level = %d\n)", cmsg->cmsg_level);
            ret = false;
        }
    }

    return ret;
}

/**
 * \brief Fetch an IPv6/ICMPv6 packet from an IPv6 socket
 * \param ipv6_sockfd An IPv6 socket which is sniffing an ICMPv6 packet
 * \param bytes A preallocated buffer in which we write the full IPv6 packet.
 * \param len The size of the preallocated buffer 
 * \param flags
 */

static ssize_t recv_icmpv6(int ipv6_sockfd, void * bytes, size_t len, int flags) {
    ssize_t               num_bytes;
    char                  cmsg_buf[BUFLEN];
    struct sockaddr_in6   from;
    struct ip6_hdr      * ip6_header = (struct ip6_hdr *) bytes;

    struct iovec iov = {
        .iov_base = ((uint8_t *) bytes) + sizeof(struct ip6_hdr),
        .iov_len  = len - sizeof(struct ip6_hdr) 
    };

    struct msghdr msg = {
        .msg_name       = &from,            // socket address
        .msg_namelen    = sizeof(from),     // sizeof socket
        .msg_iov        = &iov,             // buffer (scather/gather array)
        .msg_iovlen     = 1,                // number of msg_iov elements
        .msg_control    = cmsg_buf,         // ancillary data
        .msg_controllen = sizeof(cmsg_buf), // sizeof ancillary data
        .msg_flags      = flags             // flags related to recv messages
    };

    // We do not need memset since we will explicitely set each bit of
    // the IPv6 header. Uncomment to debug.
    //memset(bytes, 0, sizeof(struct ip6_hdr));

    // Fetch the bytes nested in the IPv6 packet (in the case of traceroute,
    // we fetch ICMPv6/UDP/payload layers).
    if ((num_bytes = recvmsg(ipv6_sockfd, &msg, flags)) == -1) {
        fprintf(stderr, "recv_ipv6_header: Can't fetch data\n");
        goto ERR_RECVMSG;
    }

    if (msg.msg_flags & MSG_TRUNC) {
        fprintf(stderr, "recv_ipv6_header: data truncated\n");
        goto ERR_MSG_TRUNC;
    }

    if (msg.msg_flags & MSG_CTRUNC) {
        fprintf(stderr, "recv_ipv6_header: ancillary data truncated\n");
        goto ERR_MSG_CTRUNK;
    }

    if(!rebuild_ipv6_header(ip6_header, &msg, &from, num_bytes)) {
        fprintf(stderr, "recv_ipv6_header: error in rebuild_ipv6_header\n");
        goto ERR_REBUILD_IPV6_HEADER;
    }

    return num_bytes + sizeof(struct ip6_hdr);
ERR_REBUILD_IPV6_HEADER:
ERR_MSG_CTRUNK:
ERR_MSG_TRUNC:
ERR_RECVMSG:
    return 0;
}

void sniffer_process_packets(sniffer_t * sniffer, uint8_t protocol_id)
{
    uint8_t    recv_bytes[BUFLEN];
    ssize_t    num_bytes = 0;
    packet_t * packet;

    switch (protocol_id) {
        case IPPROTO_ICMP:
            num_bytes = recv(sniffer->icmpv4_sockfd, recv_bytes, BUFLEN, 0);
            break;
        case IPPROTO_ICMPV6:
            num_bytes = recv_icmpv6(sniffer->icmpv6_sockfd, recv_bytes, BUFLEN, 0);
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

			if (!(sniffer->recv_callback(packet, sniffer->recv_param))) {
                fprintf(stderr, "Error in sniffer's callback\n");
            }
        }
	}
}

