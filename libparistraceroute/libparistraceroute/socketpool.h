#ifndef SOCKETPOOL_H
#define SOCKETPOOl_H

#include "packet.h"

typedef struct {
    int socket;
} socketpool_t;

socketpool_t *socketpool_create(void);
void socketpool_free(socketpool_t *socketpool);

/**
 * \brief Sends a packet on the network using a socket from the pool
 * \param socketpool The socketpool to use
 * \param packet The packet to send
 * \return 0 if successful, -1 otherwise
 */
int socketpool_send_packet(socketpool_t *socketpool, packet_t *packet);

#endif
