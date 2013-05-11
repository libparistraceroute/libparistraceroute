#ifndef PACKET_H
#define PACKET_H

/**
 * \file packet.h
 * \brief Header for network packets
 */

#include "buffer.h"

/**
 * \struct packet_t
 * \brief Structure describing a network packet
 */

typedef struct packet_s {
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
 * \brief Delete a packet
 * \param packet Pointer to the packet structure to delete
 */

void packet_free(packet_t * packet);

/**
 * \brief Guess the IP version of a packet stored in a buffer
 *   according to the 4 first bits.
 * \param buffer The buffer storing an (IP) packet
 * \return 4 for IPv4, 6 for IPv6, another value if the
 *   buffer is not well-formed.
 */

int packet_guess_address_family(const packet_t * packet);

// Accessors

buffer_t * packet_get_buffer(packet_t * packet);

void packet_set_buffer(packet_t * packet, buffer_t * buffer);

#endif
