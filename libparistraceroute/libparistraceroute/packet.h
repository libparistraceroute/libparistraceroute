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
    buffer_t * buffer; /**< Buffer to hold the packet data */
    // TODO Redundant information : We use this temporary hack 
    // while we are not sure that the dst_ip and dst_port
    // have been explicitly set in the buffer (see probe) */
    char     * dst_ip;   /**< Destination IP (string format) */
    uint16_t   dst_port; /**< Destination port */
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

buffer_t * packet_get_buffer(packet_t * packet);

void packet_set_buffer(packet_t * packet, buffer_t * buffer);

/**
 * \brief Create a packet_t instance according to probe contents.
 * \param probe The probe skeleton we used to build the packet.
 * \return A pointer to the corresponding allocated packet_t
 *    instance, NULL in case of failure.
 */

packet_t * packet_create_from_probe(probe_t * probe);

#endif
