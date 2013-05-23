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


/* network options */
 const double wait[3] = {5,     0,   INT_MAX};

static struct opt_spec network_cl_options[] = {
    /* action             short long      metavar     help    variable XXX */
    {opt_store_double_lim,"w",  "--wait", "waittime", HELP_w, wait},
};

/** \brief return the commandline options related to network
  * \return a pointer to an opt_spec structure 
  */
struct opt_spec * network_get_cl_options() {
    return network_cl_options;
}


/**
 * \brief Extract the probe ID (tag) from a probe
 * \param probe The queried probe
 * \param ptag_probe Address of an uint16_t in which we will write the tag
 * \return true iif successful
 */

static inline bool probe_extract_tag(const probe_t * probe, uint16_t * ptag_probe) {
    return probe_extract_ext(probe, "checksum", 1, ptag_probe);
}

/**
 * \brief Extract the probe ID (tag) from a reply
 * \param reply The queried reply
 * \param ptag_reply Address of the uint16_t in which the tag is written
 * \return true iif successful
 */

static inline bool reply_extract_tag(const probe_t * reply, uint16_t * ptag_reply) {
    return probe_extract_ext(reply, "checksum", 3, ptag_reply);
}

/**
 * \brief Set the probe ID (tag) from a probe
 * \param probe The probe we want to update
 * \param tag_probe The tag we're assigning to the probe 
 * \return true iif successful
 */

static bool probe_set_tag(probe_t * probe, uint16_t tag_probe) {
    bool      ret = false;
    field_t * field;

    if ((field = I16("checksum", tag_probe))) {
        ret = probe_set_field_ext(probe, 1, field);
        field_free(field);
    }

    return ret;
}

/**
 * \brief Handler called by the sniffer to allow the network layer
 *    to process sniffed packets.
 * \param network The network layer
 * \param packet The sniffed packet
 */

static bool network_sniffer_callback(packet_t * packet, void * recvq) {
    return queue_push_element((queue_t *) recvq, packet);
}

/**
 * \brief Retrieve a tag (probe ID) not yet used.
 * \return An available tag
 */

static uint16_t network_get_available_tag(network_t * network) {
    return ++network->last_tag;
}

/**
 * \brief Dump tags of every flying probes
 * \param network The queried network layer
 */

static void network_flying_probes_dump(network_t * network) {
    size_t     i, num_flying_probes = dynarray_get_size(network->probes);
    uint16_t   tag_probe;
    probe_t  * probe;

    printf("\n%lu flying probe(s) :\n", num_flying_probes);
    for (i = 0; i < num_flying_probes; i++) {
        probe = dynarray_get_ith_element(network->probes, i);
        probe_extract_tag(probe, &tag_probe) ?
            printf(" 0x%x", tag_probe):
            printf(" (invalid tag)");
        printf("\n");
    }
}

/**
  * \brief Get the oldest probe in network
  * \param network Pointer to network instance
  * \return a pointer to oldest probe 
  */

static probe_t * network_get_oldest_probe(const network_t * network) {
        return dynarray_get_ith_element(network->probes, 0);
}

/**
 * \brief Compute when a probe expires
 * \param network The network layer
 * \param probe A probe instance. Its sending time must be set.
 * \return The timeout of the probe. This value may be negative. If so,
 *    it means that this probe has already expired and a PROBE_TIMEOUT
 *    should be raised.
 */

static double network_get_probe_timeout(const network_t * network, const probe_t * probe) {
    return network_get_timeout(network) - (get_timestamp() - probe_get_sending_time(probe));
}

/**
 * \brief Update network->timerfd file descriptor to make it activated
 *   if a timeout occurs for oldest probe.
 * \param network The updated network layer.
 * \return true iif successful
 */

static bool network_update_next_timeout(network_t * network)
{
    struct itimerspec   timeout;
    probe_t           * probe;
    double              next_timeout;
    time_t              next_timeout_sec;

    if ((probe = network_get_oldest_probe(network))) {
        // The timer will updated according to the lifetime of the oldest flying probe 
//        next_timeout = network_get_timeout(network) - (get_timestamp() - probe_get_sending_time(probe));
        next_timeout = network_get_probe_timeout(network, probe);

        if (next_timeout <= 0) {
            // This should never occurs. If so, it means that we do not have raised enough
            // PROBE_TIMEOUT event in network_drop_oldest_flying_probe
            fprintf(stderr, "The new oldest probe has already expired!\n");
            goto ERR_INVALID_TIMEOUT;
        }
    } else {
        // The timer will be disarmed since there is no more flying probes
        next_timeout = 0;
    }

    // Prepare the itimerspec structure
    next_timeout_sec            = (time_t) next_timeout;
    timeout.it_value.tv_sec     = next_timeout_sec;
    timeout.it_value.tv_nsec    = 1000000 * (next_timeout - next_timeout_sec);
    timeout.it_interval.tv_sec  = 0;
    timeout.it_interval.tv_nsec = 0;

    // Update the timer
    return (timerfd_settime(network->timerfd, 0, &timeout, NULL) != -1); 

ERR_INVALID_TIMEOUT:
    return false;
}

/**
 * \brief Matches a reply with a probe.
 * \param network The queried network layer
 * \param reply The probe_t instance related to a sniffed packet
 * \returns The corresponding probe_t instance which has provoked
 *   this reply.
 */

static probe_t * network_get_matching_probe(network_t * network, const probe_t * reply)
{
    // Suppose we perform a traceroute measurement thanks to IPv4/UDP packet
    // We encode in the IPv4 checksum the ID of the probe.
    // Then, we should sniff an IPv4/ICMP/IPv4/UDP packet.
    // The ICMP message carries the begining of our probe packet, so we can
    // retrieve the checksum (= our probe ID) of the second IP layer, which
    // corresponds to the 3rd checksum field of our probe.

    uint16_t   tag_probe, tag_reply;
    probe_t  * probe;
    size_t     i, num_flying_probes;

    // Get the 3-rd checksum field stored in the reply, since it stores our probe ID.
    if(!(reply_extract_tag(reply, &tag_reply))) {
        // This is not an IP/ICMP/IP/* reply :( 
        fprintf(stderr, "Can't retrieve tag from reply\n");
        return NULL;
    }

    num_flying_probes = dynarray_get_size(network->probes);
    for (i = 0; i < num_flying_probes; i++) {
        probe = dynarray_get_ith_element(network->probes, i);

        // Reply / probe comparison. In our probe packet, the probe ID
        // is stored in the checksum of the (first) IP layer. 
        if (probe_extract_tag(probe, &tag_probe)) {
            if (tag_reply == tag_probe) break;
        }
    }

    // No match found if we reached the end of the array
    if (i == num_flying_probes) {
        fprintf(stderr, "network_get_matching_probe: This reply has been discarded: tag = 0x%x.\n", tag_reply);
        probe_dump(reply);
        network_flying_probes_dump(network);
        return NULL;
    }

    // We delete the corresponding probe

    // TODO: ... but it should be kept, for archive purposes, and to match for duplicates...
    // But we cannot reenable it until we set the probe ID into the
    // checksum, since probes with same flow_id and different TTL have the
    // same checksum

    dynarray_del_ith_element(network->probes, i, NULL);

    // The matching probe is the oldest one, update the timer
    // according to the next probe timeout.
    if (i == 0) {
        if (!(network_update_next_timeout(network))) {
            fprintf(stderr, "Error while updating timeout\n");
        }
    }

    return probe;
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

    if (!(network->sniffer = sniffer_create(network->recvq, network_sniffer_callback))) {
        goto ERR_SNIFFER;
    }

    if (!(network->probes = dynarray_create())) goto ERR_PROBES;

    network->last_tag = 0;
    network->timeout = NETWORK_DEFAULT_TIMEOUT;
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
    if (network) {
        dynarray_free(network->probes, (ELEMENT_FREE) probe_free);
        close(network->timerfd);
        sniffer_free(network->sniffer);
        queue_free(network->sendq, (ELEMENT_FREE) probe_free);
        queue_free(network->recvq, (ELEMENT_FREE) probe_free);
        socketpool_free(network->socketpool);
        free(network);
    }
}

void network_set_timeout(network_t * network, double new_timeout) {
    network->timeout = new_timeout;
}

double network_get_timeout(const network_t * network) {
    return network->timeout;
}

inline int network_get_sendq_fd(network_t * network) {
    return queue_get_fd(network->sendq);
}

inline int network_get_recvq_fd(network_t * network) {
    return queue_get_fd(network->recvq);
}

inline int network_get_sniffer_fd(network_t * network) {
    return sniffer_get_sockfd(network->sniffer);
}

inline int network_get_timerfd(network_t * network) {
    return network->timerfd;
}

/* TODO we need a function to return the set of fd used by the network */

bool network_tag_probe(network_t * network, probe_t * probe)
{
    uint16_t   tag, checksum;
    buffer_t * payload;
    size_t     payload_size = probe_get_payload_size(probe);
    size_t     tag_size = sizeof(uint16_t);

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

    if (payload_size < tag_size) {
        fprintf(stderr, "Payload too short (payload_size = %lu tag_size = %lu)\n", payload_size, tag_size);
        goto ERR_INVALID_PAYLOAD;
    }

    tag = network_get_available_tag(network);
    if (!(payload = buffer_create())) {
        fprintf(stderr, "Can't create buffer\n");
        goto ERR_BUFFER_CREATE;
    }

    // Write the probe ID in the buffer 
    if (!(buffer_write_bytes(payload, &tag, tag_size))) {
        fprintf(stderr, "Can't set data\n");
        goto ERR_BUFFER_SET_DATA;
    }

    // Write the tag at offset zero of the payload
    if (!(probe_write_payload(probe, payload))) {
        fprintf(stderr, "Can't write payload\n");
        goto ERR_PROBE_WRITE_PAYLOAD;
    }
    
    // Update checksum (TODO: should be done automatically by probe_set_fields)
    if (!(probe_update_fields(probe))) {
        fprintf(stderr, "Can't update fields\n");
        goto ERR_PROBE_UPDATE_FIELDS;
    }

    // Retrieve the checksum of UDP checksum 
    if (!(probe_extract_tag(probe, &checksum))) {
        fprintf(stderr, "Can't extract tag\n");
        goto ERR_PROBE_EXTRACT_CHECKSUM;
    }

    // Write the probe ID in the UDP checksum
    if (!(probe_set_tag(probe, htons(tag)))) {
        fprintf(stderr, "Can't set tag\n");
        goto ERR_PROBE_SET_TAG;
    }

    // Write the old checksum in the payload
    buffer_write_bytes(payload, &checksum, tag_size);
    if (!(probe_write_payload(probe, payload))) {
        fprintf(stderr, "Can't write payload (2)\n");
        goto ERR_PROBE_WRITE_PAYLOAD2;
    }
    return true;

ERR_PROBE_WRITE_PAYLOAD2:
ERR_PROBE_SET_TAG:
ERR_PROBE_EXTRACT_CHECKSUM:
ERR_PROBE_UPDATE_FIELDS:
ERR_PROBE_WRITE_PAYLOAD:
ERR_BUFFER_SET_DATA:
    buffer_free(payload);
ERR_BUFFER_CREATE:
ERR_INVALID_PAYLOAD:
    return false;
}

// TODO This could be replaced by watchers: FD -> action
bool network_process_sendq(network_t * network)
{
    probe_t  * probe;
    packet_t * packet;
    size_t     num_flying_probes;

    // Probe skeleton when entering the network layer.
    // We have to duplicate the probe since the same address of skeleton
    // may have been passed to pt_send_probe. 
    // => We duplicate this probe in the
    // network layer registry (network->probes) and then tagged.

    // Do not free probe at the end of this function.
    // Its address will be saved in network->probes and freed later.
    probe = queue_pop_element(network->sendq, NULL);

    /*
    //printf("222222222222222222222222222222222");
    //probe_dump(probe);
    */
    
    if (!network_tag_probe(network, probe)) {
        fprintf(stderr, "Can't tag probe\n");
        goto ERR_TAG_PROBE;
    }

    /*
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
    */

    // Make a packet from the probe structure 
    if (!(packet = probe_create_packet(probe))) {
        fprintf(stderr, "Can't create packet\n");
    	goto ERR_CREATE_PACKET;
    }

    // Send the packet 
    if (!(socketpool_send_packet(network->socketpool, packet))) {
        fprintf(stderr, "Can't send packet\n");
        goto ERR_SEND_PACKET;
    }

    // Update the sending time
    probe_set_sending_time(probe, get_timestamp());

    // Register this probe in the list of flying probes
    if (!(dynarray_push_element(network->probes, probe))) {
        fprintf(stderr, "Can't register probe\n");
        goto ERR_PUSH_PROBE;
    }

    //network_flying_probes_dump(network);

    // We've just sent a probe and currently, this is the only one in transit.
    // So currently, there is no running timer, prepare timerfd. 
    num_flying_probes = dynarray_get_size(network->probes);
    if (num_flying_probes == 1) {
        struct itimerspec new_value;

        new_value.it_value.tv_sec = (time_t) network->timeout; 
        new_value.it_value.tv_nsec = 0;
        new_value.it_interval.tv_sec = 0;
        new_value.it_interval.tv_nsec = 0;

        if (timerfd_settime(network->timerfd, 0, &new_value, NULL) == -1) {
            fprintf(stderr, "Can't set timerfd\n");
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
    return false;
}

bool network_process_recvq(network_t * network)
{
    probe_t       * probe,
                  * reply;
    packet_t      * packet;
    probe_reply_t * probe_reply;
    
    // Pop the packet from the queue
    if (!(packet = queue_pop_element(network->recvq, NULL))) {
        goto ERR_PACKET_POP;
    }

    // Transform the reply into a probe_t instance
    if(!(reply = probe_wrap_packet(packet))) {
        goto ERR_PROBE_WRAP_PACKET;
    }
    probe_set_recv_time(reply, get_timestamp());

    // Find the probe corresponding to this reply
    // The corresponding pointer (if any) is removed from network->probes
    if (!(probe = network_get_matching_probe(network, reply))) {
        goto ERR_PROBE_DISCARDED;
    }

    // Build a pair made of the probe and its corresponding reply
    if (!(probe_reply = probe_reply_create())) {
        goto ERR_PROBE_REPLY_CREATE;
    }

    // We're pass to the upper layer the probe and the reply to the upper layer.
    probe_reply_set_probe(probe_reply, probe);
    probe_reply_set_reply(probe_reply, reply);

    // Notify the instance which has build the probe that we've got the corresponding reply
    pt_algorithm_throw(NULL, probe->caller, event_create(PROBE_REPLY, probe_reply, NULL, NULL)); // TODO probe_reply_free frees only the reply
    return true;

ERR_PROBE_REPLY_CREATE:
ERR_PROBE_DISCARDED:
    probe_free(reply);
ERR_PROBE_WRAP_PACKET:
    packet_free(packet);
ERR_PACKET_POP:
    return false;
}

void network_process_sniffer(network_t * network) {
    sniffer_process_packets(network->sniffer);
}

bool network_drop_oldest_flying_probe(network_t * network)
{
    /*
    bool      ret = false;
    size_t    num_flying_probes = dynarray_get_size(network->probes);
    probe_t * probe;

    // Is there flying probe(s) ?
    if (num_flying_probes) { 

        // Find the oldest probe
        probe = dynarray_get_ith_element(network->probes, 0);

        // Erase the pointer of the lost probe from the flying probe list
        dynarray_del_ith_element(network->probes, 0, NULL);

        // We raise an event to notify the caller. 
        // The lost probe will be freed once the raised event will be freed.
        pt_algorithm_throw(NULL, probe->caller, event_create(PROBE_TIMEOUT, probe, NULL, NULL)); //(ELEMENT_FREE) probe_free));
        ret = network_update_next_timeout(network);
    }
    return ret;
    */

    // Drop every expired probes
    size_t    i, num_flying_probes = dynarray_get_size(network->probes);
    bool      ret = false;
    probe_t * probe;

    // Is there flying probe(s) ?
    if (num_flying_probes) { 
        // Iterate on each expired probes (at least the oldest one has expired)
        probe = network_get_oldest_probe(network);
        // TODO use get_oldest_probe
        for(i = 0 ; probe = dynarray_get_ith_element(network->probes, i) && network_get_probe_timeout(network, probe) <= 0; i++) {
            ///// DEBUG
            printf("This probe has expired\n");
            //probe_dump(probe);
            printf("probe timeout : %f\n", network_get_probe_timeout(network, probe));
            ///// DEBUG
            // This probe has expired, raise a PROBE_TIMEOUT event.
            pt_algorithm_throw(NULL, probe->caller, event_create(PROBE_TIMEOUT, probe, NULL, NULL)); //(ELEMENT_FREE) probe_free));
        }

        // Delete the n oldest probes
        dynarray_del_n_elements(network->probes, 0, i, NULL);

        ///// DEBUG
        // The oldest probe (if any) is used to update the next timeout
        // TODO use get_oldest_probe
        if (probe = network_get_oldest_probe(network)) {
            probe_dump(probe);
        }
        ///// DEBUG
        ret = network_update_next_timeout(network);
    }

    return ret;
}

