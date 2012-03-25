#ifndef NETWORK_H
#define NETWORK_H

#include "packet.h"
#include "queue.h"
#include "socketpool.h"

/******************************************************************************
 * Network
 ******************************************************************************/

typedef struct {
    socketpool_t *socketpool;
    queue_t *sendq;
    queue_t *recvq;
} network_t;

network_t* network_create(void);
void network_free(network_t *network);

int network_get_sendq_fd(network_t *network);
int network_get_recvq_fd(network_t *network);

int network_send_probe(network_t *network, probe_t *probe, void (*callback)); //(pt_loop_t *loop, probe_t *probe, probe_t *reply));

int network_process_sendq(network_t *network);
int network_process_recvq(network_t *network);

#endif
