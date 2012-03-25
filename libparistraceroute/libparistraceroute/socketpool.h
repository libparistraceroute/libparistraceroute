#ifndef SOCKETPOOL_H
#define SOCKETPOOl_H

#include "packet.h"

typedef struct {
    int socket;
} socketpool_t;

socketpool_t *socketpool_create(void);
void socketpool_free(socketpool_t *socketpool);

int socketpool_send_packet(socketpool_t *socketpool, packet_t *packet);

#endif
