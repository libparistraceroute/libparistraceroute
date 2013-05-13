#ifndef SNIFFER_H
#define SNIFFER_H

/**
 * \file sniffer.h
 * \brief Header file : packet sniffer
 *
 * The current implementation is based on raw sockets, but we could envisage a
 * libpcap implementation too
 */

#include "packet.h"

/**
 * \struct sniffer_t
 * \brief Structure representing a packet sniffer
 */

typedef struct {
    int                socket;                                    /**< Raw socket for listening to the network */
    struct network_s * network;                                   /**< Pointer to the associated network structure */
    void            (* callback)(struct network_s *, packet_t *); /**< Callback for received packets */
} sniffer_t;

/**
 * \brief Creates a new sniffer.
 * \param callback This function is called whenever a packet is sniffed.
 * \return Pointer to a sniffer_t structure representing a packet sniffer
 */

sniffer_t * sniffer_create(struct network_s * network, void (*callback)(struct network_s * network, packet_t * packet));

/**
 * \brief Free a sniffer_t structure.
 * \param sniffer Pointer to a sniffer_t structure representing a packet sniffer
 */

void sniffer_free(sniffer_t *sniffer);

/**
 * \brief Return a file description associated to the sniffer.
 * \param sniffer Pointer to a sniffer_t structure representing a packet sniffer 
 */

int sniffer_get_fd(sniffer_t *sniffer);

void sniffer_process_packets(sniffer_t * sniffer);

#endif
