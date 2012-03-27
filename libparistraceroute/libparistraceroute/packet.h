#ifndef PACKET_H
#define PACKET_H

/**
 * \file packet.h
 * \brief Header for network packets
 */

#include "probe.h"

#define MAXBUF 10000

/**
 * \struct packet_t
 * \brief Structure describing a network packet
 */
typedef struct {
	/** Buffer to hold the packet data */
    char data[MAXBUF];
	/** Size of buffer */
    unsigned int size;
} packet_t;

/**
 * \brief Create a new packet
 * \return New packet_t structure
 */
packet_t* packet_create(void);
/**
 * \brief Create a new packet from a probe
 * \param probe Pointer to the probe to use
 * \return New packet_t structure
 */
packet_t* packet_create_from_probe(probe_t *probe);
/**
 * \brief Delete a packet
 * \param packet Pointer to the packet structure to delete
 */
void packet_free(packet_t *packet);

#endif
