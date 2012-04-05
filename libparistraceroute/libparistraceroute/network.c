#include <stdlib.h>
#include <stdio.h>
#include "network.h"
#include "packet.h"
#include "queue.h"

#include "probe.h" // test
#include "algorithm.h"

/******************************************************************************
 * Network
 ******************************************************************************/

void network_sniffer_handler(network_t *network, packet_t *packet)
{
    queue_push_element(network->recvq, packet);
}

network_t* network_create(void)
{
    network_t *network;

    network = malloc(sizeof(network_t));
    if (!network)
        goto err_network;

    /* create a socket pool */
    network->socketpool = socketpool_create();
    if (!network->socketpool)
        goto err_socketpool;

    /* create the send queue: probes */
    network->sendq = queue_create();
    if (!network->sendq)
        goto err_sendq;

    /* create the receive queue: packets */
    network->recvq = queue_create();
    if (!network->recvq)
        goto err_recvq;

    /* create the sniffer */
    network->sniffer = sniffer_create(network, network_sniffer_handler);
    if (!network->sniffer) 
        goto err_sniffer;

    /* create a list to store probes */
    network->probes = dynarray_create();
    if (!network->probes)
        goto err_probes;
    return network;

err_probes:
    sniffer_free(network->sniffer);
err_sniffer:
    queue_free(network->recvq, (ELEMENT_FREE)packet_free);
err_recvq:
    queue_free(network->sendq, (ELEMENT_FREE)probe_free);
err_sendq:
    socketpool_free(network->socketpool);
err_socketpool:
    free(network);
    network = NULL;
err_network:
    return NULL;
}

void network_free(network_t *network)
{
    dynarray_free(network->probes, (ELEMENT_FREE)probe_free);
    sniffer_free(network->sniffer);
    queue_free(network->sendq, (ELEMENT_FREE)probe_free);
    queue_free(network->recvq, (ELEMENT_FREE)probe_free);
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

int network_get_sniffer_fd(network_t *network)
{
    return sniffer_get_fd(network->sniffer);
}

/* TODO we need a function to return the set of fd used by the network */

packet_t *packet_create_from_probe(probe_t *probe)
{
    //unsigned char  * payload;
    //size_t           payload_size;
    layer_t        * layer;
    size_t           size;
    int              i;
    char           * dip;
    unsigned short   dport;
    packet_t *packet;
    field_t *field;
    
    // XXX assert ipv4/udp/no payload (encode tag into checksum) 
    
    field = probe_get_field(probe, "dst_ip");
    if (!field)
        return NULL; // missing dst ip
    dip = field->value.string;

    field = probe_get_field(probe, "dst_port");
    if (!field)
        return NULL; // missing dst port
    dport = field->value.int16;

    packet = packet_create();
    packet_set_dip(packet, dip);
    packet_set_dport(packet, dport);
    packet_set_buffer(packet, probe_get_buffer(probe));

    size = dynarray_get_size(probe->layers);

    /* Sets the payload */

    /* Allow the protocol to do some processing before checksumming */
    for (i = 0; i<size; i++) {
        layer = dynarray_get_ith_element(probe->layers, i);

        /* finalize callback */
        if (layer->protocol->finalize)
            layer->protocol->finalize(layer->buffer);

    }

    // 1) Write payload (need to have enough buffer space)
    //payload      = probe_get_payload(probe);
    //payload_size = probe_get_payload_size(probe);

    // 2) Go though the layers of the probe in the reverse order to write
    // checksums
    // XXX layer_t will require parent layer, and probe_t bottom_layer
    for (i = size - 1; i >= 0; i--) {
        layer = dynarray_get_ith_element(probe->layers, i);
        /* Does the protocol require a pseudoheader ? */
        if (layer->protocol->need_ext_checksum) {
            //layer_t        * layer_prev;
            //protocol_t     * protocol_prev;
            //pseudoheader_t * psh; // variant of a buffer

            if (i == 0)
                return NULL;

            //layer_prev = dynarray_get_ith_element(probe->layers, i-1);
            //protocol_prev = layer_prev->protocol;

            // TODO not implemented
            // psh = protocol->create_pseudo_header(protocol_prev, offsets[i-1]);
            // if (!psh)
            //     return NULL;
            //    protocol->write_checksum(data, psh);
            //    free(psh); 
            //    psh = NULL;

        } else {
            // could be a function in layer ?
            layer->protocol->write_checksum(layer->buffer, NULL);
        }
    }

    // 3) swap payload and IPv4 checksum

    //probe_dump(probe);

    return packet;

}


// not the right callback here
int network_send_probe(network_t *network, probe_t *probe)
{
    queue_push_element(network->sendq, probe);

    return 0;
}

// TODO This could be replaced by watchers: FD -> action

int network_process_sendq(network_t *network)
{
    probe_t *probe;
    packet_t *packet;
    int res;
    
    probe = queue_pop_element(network->sendq);

    /* Make a packet from the probe structure */
    packet = packet_create_from_probe(probe);
    if (!packet)
        return -1;

    /* Send the packet */
    res = socketpool_send_packet(network->socketpool, packet);
    if (res < 0)
        return -2;

    probe_set_sending_time(probe, get_timestamp());

    /* Add the probe the the probes_in_flight list */
    dynarray_push_element(network->probes, probe);

    return 0;
}

/**
 * \brief matches a reply with a probe
 */
probe_t *network_match_probe(network_t *network, probe_t *reply)
{
    field_t *reply_checksum_field;
    probe_t *probe;
    size_t size;
    unsigned int i;

    // probe_dump(probe);

    reply_checksum_field = probe_get_field(reply, "checksum");
    
    size = dynarray_get_size(network->probes);
    for(i = 0; i < size; i++) {
        field_t *probe_checksum_field;

        probe = dynarray_get_ith_element(network->probes, i);

        /* Reply / probe comparison = let's start simple by only comparing the
         * checksum
         */
        probe_checksum_field = probe_get_field(reply, "checksum");
        if (field_compare(probe_checksum_field, reply_checksum_field) == 0)
            break;

    }

    /* No match found if we reached the end of the array */
    if (i == size) {
        printf("No match found\n");
        return NULL;
    }

    return probe;
}

/**
 * \brief Process received packets: match them with a probe, or discard them.
 * \param network Pointer to a network structure
 *
 * Typically, the receive queue stores all the packets received by the sniffer.
 */
int network_process_recvq(network_t *network)
{
    probe_t *probe, *reply;
    packet_t *packet;
    algorithm_instance_t *instance;
    probe_reply_t *pr;
    
    packet = queue_pop_element(network->recvq);
    reply = probe_create();
    probe_set_buffer(reply, packet->buffer);

    //probe_dump(probe);
    
    probe = network_match_probe(network, reply);
    if (!probe) {
        printf("probe discarded\n");
        return -1; // Discard the probe
    }
    /* We have a match: probe has generated reply
     * Let's notify the caller that it has received a reply
     */
    instance = probe->caller;

    /* TODO we need a probe/reply pair */
    pr = probe_reply_create();
    if (!pr)
        return -1; // Could not create probe_reply
    probe_reply_set_probe(pr, probe);
    probe_reply_set_reply(pr, reply);
    algorithm_instance_add_event(instance, event_create(PROBE_REPLY_RECEIVED, pr));

    return 0;
}

int network_process_sniffer(network_t *network)
{
    sniffer_process_packets(network->sniffer);
    return 0;
}
