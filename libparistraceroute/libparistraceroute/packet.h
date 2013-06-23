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
 * \return The newly allocated packet_t instance, NULL in case of failure 
 */

packet_t * packet_create(void);

/**
 * \brief Create a new packet
 * \param bytes The bytes making the packet
 * \param num_bytes Number of considered bytes 
 * \return The newly allocated packet_t instance, NULL in case of failure 
 */

packet_t * packet_create_from_bytes(uint8_t * bytes, size_t num_bytes);

/**
 * \brief Create a new packet
 * \param bytes The bytes carried by the packet
 * \param num_bytes The packet size (in bytes)
 * \return The newly allocated packet_t instance, NULL in case of failure 
 */

packet_t * packet_wrap_bytes(uint8_t * bytes, size_t num_bytes);

/**
 * \brief Resize a packet
 * \param new_size The new packet size
 * \return true iif successful
 */

bool packet_resize(packet_t * packet, size_t new_size);

/**
 * \brief Retrieve the size of a given packet
 * \param packet The queried packet
 * \return The corresponding size (in bytes)
 */

size_t packet_get_size(const packet_t * packet);

/**
 * \brief Retrieve a pointer to the begining of bytes managed
 *   by a packet_t instance
 * \param packet A packet_t instance
 * \return The corresponding pointer.
 */

uint8_t * packet_get_bytes(const packet_t * packet);

/**
 * \brief Duplicate a packet
 * \param packet The packet we're copying
 * \return The newly allocated packet, NULL in case of failure 
 */

packet_t * packet_dup(const packet_t * packet);

/**
 * \brief Delete a packet
 * \param packet Pointer to the packet structure to delete
 */

void packet_free(packet_t * packet);

/**
 * \brief Dump packet contents
 * \param packet The packet instance we want to print
 */

void packet_dump(const packet_t * packet);

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
