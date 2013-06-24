#ifndef SOCKETPOOL_H
#define SOCKETPOOl_H

#include "packet.h"

typedef struct {
    int ipv4_sockfd; /**< File descriptor of the IPv4 raw socket */
    int ipv6_sockfd; /**< File descriptor of the IPv6 raw socket */
} socketpool_t;

/**
 * \brief Allocate a socketpool_t instance 
 * \return The address of the newly allocated socketpool_t instance,
 *    NULL in case of failure.
 */

socketpool_t * socketpool_create(void);

/**
 * \brief Release a socket pool from the memory.
 * \param socketpool Address of the socketpool we want to release
 *    from the memory. 
 */

void socketpool_free(socketpool_t * socketpool);

/**
 * \brief Sends a packet on the network using a socket from the pool
 * \param socketpool The socketpool to use
 * \param packet The packet to send
 * \return true iif successful
 */

bool socketpool_send_packet(const socketpool_t * socketpool, const packet_t * packet);

#endif
