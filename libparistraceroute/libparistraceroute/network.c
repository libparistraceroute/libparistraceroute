#include <stdlib.h>
#include "network.h"
#include "queue.h"
#include "sniffer.h"

/******************************************************************************
 * Network
 ******************************************************************************/

network_t* network_create(void)
{
    network_t *network;

    network = malloc(sizeof(network_t));
    network->socketpool = socketpool_create();
    if (!(network->socketpool)) {
        free(network);
        network = NULL;
        return NULL;
    }
    network->sendq = queue_create();
    if (!(network->sendq)) {
        socketpool_free(network->socketpool);
        free(network);
        network = NULL;
    }

    network->recvq = queue_create();
    if (!(network->recvq)) {
        queue_free(network->sendq);
        socketpool_free(network->socketpool);
        free(network);
        network = NULL;
    }
    
    return network;
}

void network_free(network_t *network)
{
    queue_free(network->sendq);
    queue_free(network->recvq);
    socketpool_free(network->socketpool);
    free(network);
    network = NULL;
}

int network_get_sendq_fd(network_t *network)
{
    return queue_get_fd(network->sendq);
}

int network_get_recvq_fd(network_t *network)
{
    return queue_get_fd(network->recvq);
}

// not the right callback here
int network_send_probe(network_t *network, probe_t *probe, void (*callback)) //(pt_loop_t *loop, probe_t *probe, probe_t *reply))
{
    packet_t *packet;
    
    packet = packet_create_from_probe(probe);
    queue_push_packet(network->sendq, packet);

    return 0;
}

int network_process_sendq(network_t *network)
{
    packet_t *packet = queue_pop_packet(network->sendq);
    if (packet)
        socketpool_send_packet(network->socketpool, packet);
    return 0;
}

int network_process_recvq(network_t *network)
{
    return 0;
}
