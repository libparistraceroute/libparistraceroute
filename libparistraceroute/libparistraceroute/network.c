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

    network->last_tag = 0;

    return network;

err_probes:
    sniffer_free(network->sniffer);
err_sniffer:
    queue_free(network->recvq, (ELEMENT_FREE)packet_free);
err_recvq:
    printf("> network_create: call probe_free\n");
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
    //printf("> network_free: call probe_free(network->probes = %d)\n", network->probes);
    dynarray_free(network->probes, (ELEMENT_FREE) probe_free);
    printf("> network_free: OK\n");
    sniffer_free(network->sniffer);
    queue_free(network->sendq, (ELEMENT_FREE) probe_free);
    queue_free(network->recvq, (ELEMENT_FREE) probe_free);
    socketpool_free(network->socketpool);
    free(network);
    network = NULL;
}

inline int network_get_sendq_fd(network_t *network) {
    return queue_get_fd(network->sendq);
}

inline int network_get_recvq_fd(network_t *network) {
    return queue_get_fd(network->recvq);
}

inline int network_get_sniffer_fd(network_t *network) {
    return sniffer_get_fd(network->sniffer);
}

/* TODO we need a function to return the set of fd used by the network */

packet_t *packet_create_from_probe(probe_t *probe)
{
    //unsigned char  * payload;
    //size_t           payload_size;
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

    return packet;

}

int network_get_available_tag(network_t *network)
{
    // XXX
    return ++network->last_tag;
}

// TODO This could be replaced by watchers: FD -> action
int network_process_sendq(network_t *network)
{
    probe_t *probe;
    packet_t *packet;
    int res;

    uint16_t tag, checksum;
    buffer_t * buffer;

    probe = queue_pop_element(network->sendq);

    /* The probe gets assigned a unique tag. Currently we encode it in the UDP
     * checksum, but I guess the tag will be protocol dependent. Also, since the
     * tag changes the probe, the user has no direct control of what is sent on
     * the wire, since typically a packet is tagged just before sending, after
     * scheduling, to maximize the number of probes in flight. Available space
     * in the headers is used for tagging, + encoding some information... */
    /* XXX hardcoded XXX */

    /* 1) Set payload = tag : this is only possible if both the payload and the
     * checksum have not been set by the user.
     * We need a list of used tags = in flight... + an efficient way to get a
     * free one... Also, we need to share tags between several instances ?
     * randomized tags ? Also, we need to determine how much size we have to
     * encode information. */
    tag = network_get_available_tag(network);

    //probe_dump(probe);

    buffer = buffer_create();
    buffer_set_data(buffer, (unsigned char*)&tag, sizeof(uint16_t));

    probe_set_payload(probe, buffer);
    
    //printf("With payload:\n");
    //probe_dump(probe);

    /* 2) Compute checksum : should be done automatically by probe_set_fields */

    probe_update_fields(probe);

    //printf("After update fields:\n");
    //probe_dump(probe);

    /* 3) Swap checksum and payload : give explanations here ! */

    checksum = probe_get_field_ext(probe, "checksum", 1)->value.int16; // UDP checksum
    probe_set_field_ext(probe, I16("checksum", tag), 1);

    //printf("After setting tag as checksum:\n");
    //probe_dump(probe);

    buffer_set_data(buffer, (unsigned char*)&checksum, sizeof(uint16_t));
    probe_set_payload(probe, buffer);

    //printf("After setting checksum as payload:\n");
    //probe_dump(probe);

    /* NOTE: we must define a metafield tag, which can be parametrized, and
     * match on this metafield */

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

    //probe_dump(reply);

    reply_checksum_field = probe_get_field_ext(reply, "checksum", 3);
    if (!reply_checksum_field)
        /* we only match with ICMP */
        return NULL;
    
    size = dynarray_get_size(network->probes);
    for(i = 0; i < size; i++) {
        field_t *probe_checksum_field;

        probe = dynarray_get_ith_element(network->probes, i);

        /* Reply / probe comparison = let's start simple by only comparing the
         * checksum
         */
        probe_checksum_field = probe_get_field_ext(probe, "checksum", 1);
        if (field_compare(probe_checksum_field, reply_checksum_field) == 0)
            break;

    }

    /* No match found if we reached the end of the array */
    if (i == size) {
        printf("Probe discarded:\n");
        probe_dump(reply);
        return NULL;
    } else {
        /* We delete the corresponding probe
         * Should be kept, for archive purposes, and to match for duplicates...
         * but we cannot reenable it until we set the probe tag into the
         * checksum, since probes with same flow_id and different ttl have the
         * same checksum */
        dynarray_del_ith_element(network->probes, i);
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
    pt_algorithm_throw(NULL, instance, event_create(PROBE_REPLY, pr, NULL));

    return 0;
}

int network_process_sniffer(network_t *network)
{
    sniffer_process_packets(network->sniffer);
    return 0;
}
