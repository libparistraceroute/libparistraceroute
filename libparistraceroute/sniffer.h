#ifndef LIBPT_SNIFFER_H
#define LIBPT_SNIFFER_H

/**
 * \file sniffer.h
 * \brief Header file : packet sniffer
 *
 * The current implementation is based on raw sockets, but we could envisage a
 * libpcap implementation too
 */

#include <stdbool.h> // bool
#include "packet.h"  // packet_t
#include "use.h"

/**
 * \struct sniffer_t
 * \brief Structure representing a packet sniffer. The sniffer calls
 *    a function whenever a packet is sniffed. For instance
 *    sniffer->recv_param may point to a queue_t instance and
 *    sniffer->recv_callback may be used to feed this queue whenever
 *    a packet is sniffed.
 */

typedef struct {
#ifdef USE_IPV4
    int     icmpv4_sockfd;  /**< Raw socket for sniffing ICMPv4 packets */
#endif
#ifdef USE_IPV6
    int     icmpv6_sockfd;  /**< Raw socket for sniffing ICMPv6 packets */
#endif
    void  * recv_param;     /**< This pointer is passed whenever recv_callback is called */
    bool (* recv_callback)(packet_t * packet, void * recv_param); /**< Callback for received packets */
} sniffer_t;

/**
 * \brief Creates a new sniffer.
 * \param callback This function is called whenever a packet is sniffed.
 * \return Pointer to a sniffer_t structure representing a packet sniffer
 */

sniffer_t * sniffer_create(void * recv_param, bool (*recv_callback)(packet_t *, void *));

/**
 * \brief Free a sniffer_t structure.
 * \param sniffer Points to a sniffer_t instance. 
 */

void sniffer_free(sniffer_t * sniffer);

#ifdef USE_IPV4
/**
 * \brief Return the file descriptor related to the ICMPv4 raw socket
 *    managed by the sniffer.
 * \param sniffer Points to a sniffer_t instance.
 * \return The corresponding socket file descriptor.
 */

int sniffer_get_icmpv4_sockfd(sniffer_t * sniffer);
#endif

#ifdef USE_IPV6
/**
 * \brief Return the file descriptor related to the ICMPv6 raw socket
 *    managed by the sniffer.
 * \param sniffer Points to a sniffer_t instance.
 * \return The corresponding socket file descriptor.
 */

int sniffer_get_icmpv6_sockfd(sniffer_t * sniffer);
#endif

/**
 * \brief Fetch a packet from the listening socket. The sniffer then
 *   call recv_callback and pass to this function this packet and
 *   eventual data stored in sniffer->recv_packet. If this callback
 *   returns false, a message is printed. 
 * \param sniffer Points to a sniffer_t instance.
 * \param protocol_id The family of the packet to fetch (IPPROTO_ICMP, IPPROTO_ICMPV6)
 */

void sniffer_process_packets(sniffer_t * sniffer, uint8_t protocol_id);

#endif // LIBPT_SNIFFER_H
