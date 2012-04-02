#ifndef PACKET_H
#define PACKET_H

/**
 * \file packet.h
 * \brief Header for network packets
 */

#include "probe.h"

/**
 * \struct packet_t
 * \brief Structure describing a network packet
 */

typedef struct {
    buffer_t *buffer; /**< Buffer to hold the packet data */

    /* Redundant information : temporary */
    unsigned char *dip;
    unsigned short dport;
} packet_t;

/**
 * \brief Create a new packet
 * \return New packet_t structure
 */

packet_t * packet_create(void);


/**
 * \brief Create a new packet from a probe
 * \param probe Pointer to the probe to use
 * \return New packet_t structure
 */

packet_t * packet_create_from_probe(probe_t * probe);

/**
 * \brief Delete a packet
 * \param packet Pointer to the packet structure to delete
 */

void packet_free(packet_t * packet);

// Accessors

buffer_t * packet_get_buffer(packet_t *packet);
int packet_set_buffer(packet_t *packet, buffer_t *buffer);

int packet_set_dip(packet_t *packet, unsigned char *dip);
int packet_set_dport(packet_t *packet, unsigned short dport);

#endif
