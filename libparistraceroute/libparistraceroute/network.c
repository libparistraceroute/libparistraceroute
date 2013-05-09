#include <stdlib.h>      // malloc ...
#include <stdio.h>
#include <time.h>
#include <sys/timerfd.h> // timerfd_create, timerfd_settime
#include <arpa/inet.h>   // htons

#include "network.h"
#include "packet.h"
#include "queue.h"
#include "probe.h"       // probe_extract
#include "algorithm.h"   // pt_algorithm_throw

#define SEC_TO_NSEC 1000000

void network_set_timeout(network_t * network, double new_timeout) {
    network->timeout = new_timeout;
}

double network_get_timeout(const network_t * network) {
    return network->timeout;
}

void network_sniffer_handler(network_t * network, packet_t * packet) {
    queue_push_element(network->recvq, packet);
}

network_t * network_create(void)
{
    network_t * network;

    if (!(network = malloc(sizeof(network_t))))        goto ERR_NETWORK;
    if (!(network->socketpool = socketpool_create()))  goto ERR_SOCKETPOOL;
    if (!(network->sendq      = queue_create()))       goto ERR_SENDQ;
    if (!(network->recvq      = queue_create()))       goto ERR_RECVQ;

    if ((network->timerfd = timerfd_create(CLOCK_REALTIME, 0)) == -1) {
        goto ERR_TIMERFD;
    }

    if (!(network->sniffer = sniffer_create(network, network_sniffer_handler))) {
        goto ERR_SNIFFER;
    }

    if (!(network->probes = dynarray_create())) goto ERR_PROBES;

    // Last probe ID used.
    network->last_tag = 0;
    return network;

ERR_PROBES:
    sniffer_free(network->sniffer);
ERR_SNIFFER:
    close(network->timerfd);
ERR_TIMERFD:
    queue_free(network->recvq, (ELEMENT_FREE) packet_free);
ERR_RECVQ:
    queue_free(network->sendq, (ELEMENT_FREE) probe_free);
ERR_SENDQ:
    socketpool_free(network->socketpool);
ERR_SOCKETPOOL:
    free(network);
ERR_NETWORK:
    return NULL;
}

void network_free(network_t * network)
{ 
    dynarray_free(network->probes, (ELEMENT_FREE) probe_free);
    close(network->timerfd);
    sniffer_free(network->sniffer);
    queue_free(network->sendq, (ELEMENT_FREE) probe_free);
    queue_free(network->recvq, (ELEMENT_FREE) probe_free);
    socketpool_free(network->socketpool);
    free(network);
}

inline int network_get_sendq_fd(network_t * network) {
    return queue_get_fd(network->sendq);
}

inline int network_get_recvq_fd(network_t * network) {
    return queue_get_fd(network->recvq);
}

inline int network_get_sniffer_fd(network_t * network) {
    return sniffer_get_fd(network->sniffer);
}

inline int network_get_timerfd(network_t * network) {
    return network->timerfd;
}

/* TODO we need a function to return the set of fd used by the network */

packet_t * packet_create_from_probe(probe_t * probe)
{
    char     * dst_ip;
    uint16_t   dst_port;
    packet_t * packet;
    
    // The destination IP is a mandatory field
    if (!(probe_extract(probe, "dst_ip", &dst_ip))) {
        perror("packet_create_from_probe: this probe has no 'dst_ip' field\n");
        goto ERR_EXTRACT_DST_IP;
    }

    // The destination port is a mandatory field
    if (!(probe_extract(probe, "dst_port", &dst_port))) {
        perror("packet_create_from_probe: this probe has no 'dst_port' field\n");
        goto ERR_EXTRACT_DST_PORT;
    }

    // Create the corresponding packet
    if (!(packet = packet_create())) {
        goto ERR_PACKET_CREATE;
    }
    packet_set_dip(packet, dst_ip);
    packet_set_dport(packet, dst_port);
    packet_set_buffer(packet, probe_get_buffer(probe));
    // do not free dst_ip since it is not dup in packet_set_dip
    return packet;

ERR_PACKET_CREATE:
ERR_EXTRACT_DST_PORT:
    free(dst_ip);
ERR_EXTRACT_DST_IP:
    return NULL;
}

int network_get_available_tag(network_t * network)
{
    // XXX
    return ++network->last_tag;
}

bool network_tag_probe(network_t * network, probe_t * probe)
{
    uint16_t   tag, checksum;
    buffer_t * payload;

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

    // TODO: The UDP checksum is the place where the probe ID is currently written.
    //       but we must define a metafield "tag", which can be parametrized,
    //       and match on this metafield

    tag = network_get_available_tag(network);
    if (!(payload = buffer_create())) {
        goto ERR_BUFFER_CREATE;
    }

    // Write the probe ID in the buffer 
    if (!(buffer_set_data(payload, &tag, sizeof(uint16_t)))) {
        goto ERR_BUFFER_SET_DATA;
    }

    // Write the tag at offset zero of the payload
    if (!(probe_write_payload(probe, payload, 0))) {
        goto ERR_PROBE_WRITE_PAYLOAD;
    }
    
    // Update checksum (TODO: should be done automatically by probe_set_fields)
    if (!(probe_update_fields(probe))) {
        goto ERR_PROBE_UPDATE_FIELDS;
    }

    // Retrieve the checksum of UDP checksum 
    if (!(probe_extract_ext(probe, "checksum", 1, &checksum))) {
        goto ERR_PROBE_EXTRACT_CHECKSUM;
    }

    // Write the probe ID in the UDP checksum
    probe_set_field_ext(probe, 1, I16("checksum", htons(tag)));

    // Write the old checksum in the payload
    buffer_set_data(payload, &checksum, sizeof(uint16_t));
    probe_write_payload(probe, payload, 0);
    return true;

ERR_PROBE_EXTRACT_CHECKSUM:
ERR_PROBE_UPDATE_FIELDS:
ERR_PROBE_WRITE_PAYLOAD:
ERR_BUFFER_SET_DATA:
    buffer_free(payload);
ERR_BUFFER_CREATE:
    return false;
}

// TODO This could be replaced by watchers: FD -> action
bool network_process_sendq(network_t * network)
{
    probe_t  * probe;
    packet_t * packet;
    size_t     num_probes;

    probe = queue_pop_element(network->sendq, NULL); // TODO pass cb element_free
    //probe_update_fields(probe);

    printf("222222222222222222222222222222222");
    probe_dump(probe);
    
    if (!network_tag_probe(network, probe)) {
        perror("network_process_sendq: Can't tag probe");
        goto ERR_TAG_PROBE;
    }

    printf("333333333333333333333333333333333");
    probe_dump(probe);
    printf("444444444444444444444444444444444");
    // DEBUG
    {
        probe_t * probe_should_be;
        if ((probe_should_be = probe_dup(probe))) {
            probe_update_fields(probe_should_be);
            probe_dump(probe_should_be);
        }
    }

    // Make a packet from the probe structure 
    if (!(packet = packet_create_from_probe(probe))) {
        perror("network_process_sendq: Can't create packet");
    	goto ERR_CREATE_PACKET;
    }

    // Send the packet 
    if (socketpool_send_packet(network->socketpool, packet) < 0) {
        perror("network_process_sendq: Can't send packet");
        goto ERR_SEND_PACKET;
    }

    // Update the sending time
    probe_set_sending_time(probe, get_timestamp());

    // Register this probe in the list of flying probes
    if (!(dynarray_push_element(network->probes, probe))) {
        perror("network_process_sendq: Can't register probe");
        goto ERR_PUSH_PROBE;
    }

    num_probes = dynarray_get_size(network->probes); // TODO this is strange
    if (num_probes == 1) {
        // There is no running timer, let's set one for this probe 
        struct itimerspec new_value;

        new_value.it_value.tv_sec = (time_t) network->timeout; 
        new_value.it_value.tv_nsec = 0;
        new_value.it_interval.tv_sec = 0;
        new_value.it_interval.tv_nsec = 0;

        if (timerfd_settime(network->timerfd, 0, &new_value, NULL) == -1) {
            perror("network_process_sendq: Can't set timerfd");
            goto ERR_TIMERFD;
        }
    }

    return true;
ERR_TIMERFD:
ERR_PUSH_PROBE:
ERR_SEND_PACKET:
    packet_free(packet);
ERR_CREATE_PACKET:
ERR_TAG_PROBE:
    perror("network_process_sendq: error\n");
    return false;
}

bool network_schedule_probe_timeout(network_t * network, probe_t * probe)
{
    struct itimerspec new_value;
    if (probe) {
        double d;
        d = network_get_timeout(network) - (get_timestamp() - probe_get_sending_time(probe));
        new_value.it_value.tv_sec = (time_t) d;
        new_value.it_value.tv_nsec = (d - (time_t) d) * SEC_TO_NSEC;
    } else {
        /* timer disarmed */
        new_value.it_value.tv_sec = 0;
        new_value.it_value.tv_nsec = 0;
    }
    new_value.it_interval.tv_sec = 0;
    new_value.it_interval.tv_nsec = 0;

    if (timerfd_settime(network->timerfd, 0, &new_value, NULL) == -1) {
        goto ERROR;
    }
    return true;

ERROR:
    return false;
}

bool network_schedule_next_probe_timeout(network_t * network)
{
    probe_t * next = NULL;

    if (dynarray_get_size(network->probes) > 0)
        next = dynarray_get_ith_element(network->probes, 0);

    return network_schedule_probe_timeout(network, next);
}

/**
 * \brief Matches a reply with a probe.
 * \param network The network layer managed by libparistraceroute
 * \param reply The probe_t instance related to a sniffed packet
 * \returns The corresponding probe_t instance which has provoked
 *   this reply.
 */

probe_t * network_match_probe(network_t * network, probe_t * reply)
{
    // Suppose we perform a traceroute measurement thanks to IPv4/UDP packet
    // We encode in the IPv4 checksum the ID of the probe.
    // Then, we should sniff an IPv4/ICMP/IPv4/UDP packet.
    // The ICMP message carries the begining of our probe packet, so we can
    // retrieve the checksum (= our probe ID) of the second IP layer, which
    // corresponds to the 3rd checksum field of our probe.

    uint16_t   tag_probe, tag_reply;
    probe_t  * probe;
    size_t     i, size;

    // Get the 3-rd checksum field stored in the reply, since it stores our probe ID.
    if (!(probe_extract_ext(reply, "checksum", 3, &tag_reply))) {
        // This is not an IP/ICMP/IP/* reply :( 
        perror("Can't retrieve tag from reply");
        return NULL;
    }

    size = dynarray_get_size(network->probes);
    for (i = 0; i < size; i++) {
        probe = dynarray_get_ith_element(network->probes, i);

        // Reply / probe comparison. In our probe packet, the probe ID
        // is stored in the checksum of the (first) IP layer. 
        if (probe_extract_ext(probe, "checksum", 1, &tag_probe)) {
            if (tag_reply == tag_probe) break;
        }
    }

    // No match found if we reached the end of the array
    if (i == size) {
        fprintf(stderr, "network_match_probe: This reply has been discarded: tag = %x. Known tags are\n", tag_reply);
        for (i = 0; i < size; i++) {
            probe = dynarray_get_ith_element(network->probes, i);
            probe_extract_ext(probe, "checksum", 1, &tag_probe) ?
                fprintf(stderr, " %d", tag_probe):
                fprintf(stderr, " (invalid tag)");
            fprintf(stderr, "\n");
        }
        return NULL;
    }

    // We delete the corresponding probe

    // TODO: ... but it should be kept, for archive purposes, and to match for duplicates...
    // But we cannot reenable it until we set the probe ID into the
    // checksum, since probes with same flow_id and different TTL have the
    // same checksum

    dynarray_del_ith_element(network->probes, i);

    if (i == 0) {
        network_schedule_next_probe_timeout(network);
    }

    return probe;
}

int network_process_recvq(network_t * network)
{
    probe_t       * probe,
                  * reply;
    packet_t      * packet;
    probe_reply_t * probe_reply;
    
    // Pop the packet from the queue
    packet = queue_pop_element(network->recvq, NULL); // TODO pass cb element_free

    // Transform the reply into a probe_t instance
    if (!(reply = probe_create())) {
        goto ERR_PROBE_CREATE;
    }
    probe_set_buffer(reply, packet->buffer);

    // Find the probe corresponding to this reply
    if (!(probe = network_match_probe(network, reply))) {
        perror("Reply discarded\n");
        goto ERR_PROBE_DISCARDED;
    }

    // Build a pair made of the probe and its corresponding reply
    if (!(probe_reply = probe_reply_create())) {
        goto ERR_PROBE_REPLY_CREATE;
    }
    probe_reply_set_probe(probe_reply, probe);
    probe_reply_set_reply(probe_reply, reply);

    // Notify the instance which has build the probe that we've got the corresponding reply
    pt_algorithm_throw(NULL, probe->caller, event_create(PROBE_REPLY, probe_reply, NULL, NULL));
    return 0;

ERR_PROBE_REPLY_CREATE:
ERR_PROBE_DISCARDED:
    probe_free(probe);
ERR_PROBE_CREATE:
    return -1;
}

void network_process_sniffer(network_t * network) {
    sniffer_process_packets(network->sniffer);
}

bool network_process_timeout(network_t * network)
{
    size_t    num_probes = dynarray_get_size(network->probes);
    probe_t * probe;

    if (num_probes == 0) return -1;

    probe = dynarray_get_ith_element(network->probes, 0);
    dynarray_del_ith_element(network->probes, 0);
    pt_algorithm_throw(NULL, probe->caller, event_create(PROBE_TIMEOUT, probe, NULL, NULL));
    return network_schedule_next_probe_timeout(network);
}
