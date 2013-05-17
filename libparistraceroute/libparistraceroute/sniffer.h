#ifndef SNIFFER_H
#define SNIFFER_H

/**
 * \file sniffer.h
 * \brief Header file : packet sniffer
 *
 * The current implementation is based on raw sockets, but we could envisage a
 * libpcap implementation too
 */

#include <stdbool.h> // bool
#include "packet.h"  // packet_t

/**
 * \struct sniffer_t
 * \brief Structure representing a packet sniffer. The sniffer calls
 *    a function whenever a packet is sniffed. For instance
 *    sniffer->recv_param may point to a queue_t instance and
 *    sniffer->recv_callback may be used to feed this queue whenever
 *    a packet is sniffed.
 */

typedef struct {
    int     sockfd;       /**< Raw socket for listening to the network */
    void  * recv_param;   /**< This pointer is passed whenever recv_callback is called */
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

/**
 * \brief Return the file descriptor related to the raw socket manage by the sniffer.
 * \param sniffer Points to a sniffer_t instance.
 */

int sniffer_get_sockfd(sniffer_t * sniffer);

void sniffer_process_packets(sniffer_t * sniffer);

#endif
