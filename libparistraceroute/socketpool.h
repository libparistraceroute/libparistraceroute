#ifndef LIBPT_SOCKETPOOL_H
#define LIBPT_SOCKETPOOL_H

#include "packet.h"
#include "use.h"

typedef struct {
#ifdef USE_IPV4
    int ipv4_sockfd; /**< File descriptor of the IPv4 raw socket */
#endif
#ifdef USE_IPV6
    int ipv6_sockfd; /**< File descriptor of the IPv6 raw socket */
#endif
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

#endif // LIBPT_SOCKETPOOL_H
