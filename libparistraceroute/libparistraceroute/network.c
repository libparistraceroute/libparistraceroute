#include "config.h"

#include <stdlib.h>      // malloc ...
#include <string.h>      // memset
#include <stdio.h>       // fprintf
#include <stdbool.h>     // bool
#include <time.h>        // time_t
#include <unistd.h>      // close
#include <sys/timerfd.h> // timerfd_create, timerfd_settime
#include <arpa/inet.h>   // htons
#include <limits.h>      // INT_MAX

#include "network.h"
#include "common.h"
#include "packet.h"
#include "queue.h"
#include "options.h"     // option_t
#include "probe.h"       // probe_extract_ext, probe_set_field_ext
#include "algorithm.h"   // pt_algorithm_throw

// TODO static variable as timeout. Control extra_delay and timeout values consistency
#define EXTRA_DELAY 0.01 // this extra delay provokes a probe timeout event if a probe will expires in less than EXTRA_DELAY seconds. Must be less than network->timeout.


//---------------------------------------------------------------------------
// Network options
//---------------------------------------------------------------------------

static double timeout[3] = OPTIONS_NETWORK_WAIT;

static option_t network_options[] = {
    // action              short long          metavar         help    variable
    {opt_store_double_lim, "w",  "--wait",     "TIMEOUT",      HELP_w, timeout},
    END_OPT_SPECS
};

/**
 * \brief return the commandline options related to network
 * \return A pointer to an opt_spec structure
 */

const option_t * network_get_options() {
    return network_options;
}

double options_network_get_timeout() {
    return timeout[0];
}

void network_set_is_verbose(network_t * network, bool verbose) {
     network->is_verbose = verbose;
}

void options_network_init(network_t * network, bool verbose)
{
    network_set_is_verbose(network, verbose);
    network_set_timeout(network, options_network_get_timeout());
}

//---------------------------------------------------------------------------
// Private functions
//---------------------------------------------------------------------------

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
 * \param tag_probe The tag we're assigning to the probe (host-side endianness)
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
 * \brief Debug function. Dump tags of every flying probes
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
 * \return A pointer to oldest probe if any, NULL otherwise
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
 * \brief Update a itimerspec instance according to a delay
 * \param itimerspec The itimerspec instance we want to update
 * \param delay A delay in seconds
 */

static void itimerspec_set_delay(struct itimerspec * timer, double delay) {
    time_t delay_sec = (time_t) delay;

    timer->it_value.tv_sec     = delay_sec;
    timer->it_value.tv_nsec    = 1000000 * (delay - delay_sec);
    timer->it_interval.tv_sec  = 0;
    timer->it_interval.tv_nsec = 0;
}

/**
 * \brief Update a timer in order to expire at a given moment .
 * \param timerfd The file descriptor related to the timer.
 * \param delay The delay (in micro seconds).
 * \return true iif successful.
 */

bool update_timer(int timerfd, double delay) {
    struct itimerspec    delay_timer;

    memset(&delay_timer, 0, sizeof(struct itimerspec));
    if (delay < 0) goto ERR_INVALID_DELAY;

    // Prepare the itimerspec structure
    itimerspec_set_delay(&delay_timer, delay);

    // Update the timer
    return (timerfd_settime(timerfd, 0, &delay_timer, NULL) != -1);

ERR_INVALID_DELAY:
    fprintf(stderr, "update_timer: invalid delay (delay = %lf)\n", delay);
    return false;
}

/**
 * \brief Update network->timerfd file descriptor to make it activated
 *   if a timeout occurs for oldest probe.
 * \param network The updated network layer.
 * \return true iif successful
 */

static bool network_update_next_timeout(network_t * network)
{
    probe_t * probe;
    double    next_timeout;

    if ((probe = network_get_oldest_probe(network))) {
        next_timeout = network_get_probe_timeout(network, probe);
    } else {
        // The timer will be disarmed since there is no more flying probes
        next_timeout = 0;
    }
    return update_timer(network->timerfd, next_timeout);
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

    // Fetch the tag from the reply. Its the 3rd checksum field.
    if (!(reply_extract_tag(reply, &tag_reply))) {
        // This is not an IP/ICMP/IP/* reply :(
        if (network->is_verbose) fprintf(stderr, "Can't retrieve tag from reply\n");
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
        if (network->is_verbose) {
            fprintf(stderr, "network_get_matching_probe: This reply has been discarded: tag = 0x%x.\n", tag_reply);
            network_flying_probes_dump(network);
        }
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

//---------------------------------------------------------------------------
// Public functions
//---------------------------------------------------------------------------

network_t * network_create()
{
    network_t * network;

    if (!(network = malloc(sizeof(network_t))))          goto ERR_NETWORK;
    if (!(network->socketpool   = socketpool_create()))  goto ERR_SOCKETPOOL;
    if (!(network->sendq        = queue_create()))       goto ERR_SENDQ;
    if (!(network->recvq        = queue_create()))       goto ERR_RECVQ;

    if ((network->timerfd = timerfd_create(CLOCK_REALTIME, 0)) == -1) {
        goto ERR_TIMERFD;
    }

#ifdef USE_SCHEDULING
    if ((network->scheduled_timerfd = timerfd_create(CLOCK_REALTIME, 0)) == -1) {
        goto ERR_GROUP_TIMERFD;
    }
    if (!(network->scheduled_probes = probe_group_create(network->scheduled_timerfd))) {
        goto ERR_GROUP;
    }
#endif
    if (!(network->sniffer = sniffer_create(network->recvq, network_sniffer_callback))) {
        goto ERR_SNIFFER;
    }

    if (!(network->probes = dynarray_create())) goto ERR_PROBES;

    network->last_tag = 0;
    network->timeout = NETWORK_DEFAULT_TIMEOUT;
    network->is_verbose = false;
    return network;

ERR_PROBES:
    sniffer_free(network->sniffer);
ERR_SNIFFER:
#ifdef USE_SCHEDULING
    probe_group_free(network->scheduled_probes);
ERR_GROUP :
    close(network->scheduled_timerfd);
ERR_GROUP_TIMERFD :
#endif
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
#ifdef USE_SCHEDULING
        probe_group_free(network->scheduled_probes);
#endif
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

#ifdef USE_IPV4
inline int network_get_icmpv4_sockfd(network_t * network) {
    return sniffer_get_icmpv4_sockfd(network->sniffer);
}
#endif

#ifdef USE_IPV6
inline int network_get_icmpv6_sockfd(network_t * network) {
    return sniffer_get_icmpv6_sockfd(network->sniffer);
}
#endif

inline int network_get_timerfd(network_t * network) {
    return network->timerfd;
}

#ifdef USE_SCHEDULING
inline int network_get_group_timerfd(network_t * network) {
    return network->scheduled_timerfd;
}

probe_group_t * network_get_scheduled_probes(network_t * network) {
    return network->scheduled_probes;
}
#endif

bool network_tag_probe(network_t * network, probe_t * probe)
{
    uint16_t   tag,         // Network-side endianness
               checksum;    // Host-side endianness
    size_t     payload_size = probe_get_payload_size(probe);
    size_t     tag_size     = sizeof(uint16_t);
    size_t     num_layers   = probe_get_num_layers(probe);

    // For probes having a payload of size 0 and a "body" field (like icmp)
    layer_t  * last_layer;
    bool       tag_in_body = false;

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

    if (num_layers < 2 || !(last_layer = probe_get_layer(probe, num_layers - 2))) {
        fprintf(stderr, "network_tag_probe: not enough layer (num_layers = %ld)\n", num_layers);
        goto ERR_GET_LAYER;
    }

    // The last layer is the payload, the previous one is the last protocol layer.
    // If this layer has a "body" field like icmp, we have no payload and
    // we use the body field.
    if (last_layer->protocol && protocol_get_field(last_layer->protocol, "body")) {
        tag_in_body = true;
    }

    tag = htons(network_get_available_tag(network));

    // Write the tag at offset zero of the payload
    if (tag_in_body) {
        probe_write_field(probe, "body", &tag, tag_size);
    } else {
        if (payload_size < tag_size) {
            fprintf(stderr, "Payload too short (payload_size = %lu tag_size = %lu)\n", payload_size, tag_size);
            goto ERR_INVALID_PAYLOAD;
        }

        if (!(probe_write_payload(probe, &tag, tag_size))) {
            goto ERR_PROBE_WRITE_PAYLOAD;
        }
    }

    // Fix checksum to get a well-formed packet
    if (!(probe_update_checksum(probe))) {
        fprintf(stderr, "Can't update fields\n");
        goto ERR_PROBE_UPDATE_FIELDS;
    }

    // Retrieve the checksum of UDP/TCP/ICMP checksum (host-side endianness)
    if (!(probe_extract_tag(probe, &checksum))) {
        fprintf(stderr, "Can't extract tag\n");
        goto ERR_PROBE_EXTRACT_CHECKSUM;
    }

    // Write the probe ID in the UDP/TCP/ICMP checksum
    if (!(probe_set_tag(probe, ntohs(tag)))) {
        fprintf(stderr, "Can't set tag\n");
        goto ERR_PROBE_SET_TAG;
    }

    // Write the tag using the network-side endianness in the payload
    checksum = htons(checksum);
    if (tag_in_body) {
        if (!(probe_write_field(probe, "body", &checksum, tag_size))) {
            fprintf(stderr, "Can't set body\n");
            goto ERR_PROBE_SET_FIELD;
        }
    } else {
        if (!probe_write_payload(probe, &checksum, tag_size)) {
            fprintf(stderr, "Can't write payload (2)\n");
            goto ERR_BUFFER_WRITE_BYTES2;
        }
    }

    return true;

ERR_PROBE_SET_FIELD:
ERR_BUFFER_WRITE_BYTES2:
ERR_PROBE_SET_TAG:
ERR_PROBE_EXTRACT_CHECKSUM:
ERR_PROBE_UPDATE_FIELDS:
ERR_PROBE_WRITE_PAYLOAD:
ERR_INVALID_PAYLOAD:
ERR_GET_LAYER:
    return false;
}

bool network_send_probe(network_t * network, probe_t * probe)
{
    // - Best effort probes are directly pushed in our sendq.
    // - Scheduled probes are scheduled, stored in the probe group, and
    // pushed in the sendq when the probe_group notify pt_loop.

#ifdef USE_SCHEDULING
    if (probe_get_delay(probe) == DELAY_BEST_EFFORT) {
#endif
        probe_set_queueing_time(probe, get_timestamp());
        return queue_push_element(network->sendq, probe);
#ifdef USE_SCHEDULING
    } else {
       return probe_group_add(network->scheduled_probes, probe);
    }
#endif
}

// TODO This could be replaced by watchers: FD -> action
bool network_process_sendq(network_t * network)
{
    probe_t           * probe;
    packet_t          * packet;
    size_t              num_flying_probes;
    struct itimerspec   new_timeout;

    // Probe skeleton when entering the network layer.
    // We have to duplicate the probe since the same address of skeleton
    // may have been passed to pt_send_probe.
    // => We duplicate this probe in the
    // network layer registry (network->probes) and then tagged.

    // Do not free probe at the end of this function.
    // Its address will be saved in network->probes and freed later.
    probe = queue_pop_element(network->sendq, NULL);

    // Tag the probe
    if (!network_tag_probe(network, probe)) {
        fprintf(stderr, "Can't tag probe\n");
        goto ERR_TAG_PROBE;
    }

    if (network->is_verbose) {
        printf("Sending probe packet:\n");
        probe_dump(probe);
    }

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

    // We've just sent a probe and currently, this is the only one in transit.
    // So currently, there is no running timer, prepare timerfd.
    num_flying_probes = dynarray_get_size(network->probes);
    if (num_flying_probes == 1) {
        itimerspec_set_delay(&new_timeout, network_get_timeout(network));
        if (timerfd_settime(network->timerfd, 0, &new_timeout, NULL) == -1) {
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

    if (network->is_verbose) {
        printf("Got reply:\n");
        probe_dump(reply);
    }

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

    // Notify the instance which has build the probe that we've got the
    // corresponding reply. No need to explicitly reference probe_reply
    // since it is just created and no any other entity refer to it.
    pt_algorithm_throw(NULL, probe->caller,
        event_create(PROBE_REPLY, probe_reply, NULL, NULL,
                     (ELEMENT_FREE) probe_reply_free));
    return true;

ERR_PROBE_REPLY_CREATE:
ERR_PROBE_DISCARDED:
    probe_free(reply);
ERR_PROBE_WRAP_PACKET:
    //packet_free(packet); TODO provoke segfault in case of stars
ERR_PACKET_POP:
    return false;
}

void network_process_sniffer(network_t * network, uint8_t protocol_id) {
    sniffer_process_packets(network->sniffer, protocol_id);
}

bool network_drop_expired_flying_probe(network_t * network)
{
    // Drop every expired probes
    size_t    i, num_flying_probes = dynarray_get_size(network->probes);
    bool      ret = false;
    probe_t * probe;

    // Is there flying probe(s) ?
    if (num_flying_probes > 0) {

        // Iterate on each expired probes (at least the oldest one has expired)
        for (i = 0 ;i < num_flying_probes; i++) {
            probe = dynarray_get_ith_element(network->probes, i);

            // Some probe may expires very soon and may expire before the next probe timeout
            // update. If so, the timer will be disarmed and libparistraceroute may freeze.
            // To avoid this kind of deadlock, we provoke a probe timeout for each probe
            // expiring in less that EXTRA_DELAY seconds.
            if (network_get_probe_timeout(network, probe) - EXTRA_DELAY > 0) break;

            // This probe has expired, raise a PROBE_TIMEOUT event.
            pt_algorithm_throw(NULL, probe->caller,
                event_create(PROBE_TIMEOUT, probe, NULL,
                             (ELEMENT_REF) probe_inc_ref_count,
                             (ELEMENT_FREE) probe_free));
        }

        // Delete the i oldest probes, which have expired.
        if (i != 0) {
            dynarray_del_n_elements(network->probes, 0, i, (ELEMENT_FREE) probe_free);
        }

        ret = network_update_next_timeout(network);
    } else {
        fprintf(stderr, "network_drop_expired_flying_probe: a probe has expired, but there is no more flying probes!");
    }

    return ret;
}

//------------------------------------------------------------------------------------
// Scheduling
//------------------------------------------------------------------------------------

#ifdef USE_SCHEDULING

static bool network_process_probe_node(network_t * network, tree_node_t * node, size_t i)
{
    tree_node_probe_t * tree_node_probe = get_node_data(node);
    probe_t           * probe;

    if (!(tree_node_probe->tag == PROBE))                               goto ERR_TAG;
    probe = (probe_t *) (tree_node_probe->data.probe);
    //TODO packet_from_probe must manage generator

    probe_set_queueing_time(probe, get_timestamp());
    if (!(queue_push_element(network->sendq, probe)))                   goto ERR_QUEUE_PUSH;
    /*
    probe_set_left_to_send(probe, probe_get_left_to_send(probe) - 1);
    num_probes_to_send = probe_get_left_to_send(probe);

    if (num_probes_to_send == 0) {
    */
    if (--(probe->left_to_send) == 0) {
        if (!(probe_group_del(network->scheduled_probes, node->parent, i)))  goto ERR_PROBE_GROUP_DEL;
    } else {
        get_node_next_delay(node);
        probe_group_update_delay(network->scheduled_probes, node);
        return true;
    }

ERR_PROBE_GROUP_DEL:
ERR_QUEUE_PUSH:
ERR_TAG:
    return false;
}

void network_process_scheduled_probe(network_t * network) {
    tree_node_t * root;

    if ((root = probe_group_get_root(network->scheduled_probes))) {
        // Handle every probe that must be sent right now
        probe_group_iter_next_scheduled_probes(
            probe_group_get_root(network->scheduled_probes),
            (void (*) (void *, tree_node_t *, size_t)) network_process_probe_node,
            (void *) network
        );
    }
}

bool network_update_scheduled_timer(network_t * network, double delay) {
    return update_timer(network->scheduled_timerfd, delay);
}

#endif // USE_SCHEDULING
