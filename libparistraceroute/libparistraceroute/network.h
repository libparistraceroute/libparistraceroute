#ifndef NETWORK_H
#define NETWORK_H

/**
 * \file network.h
 * \brief Header for network functions.
 *
 * A network represents the interface between the "packets and socket" layer
 * and the real network / the Internet. It is responsible for maintaining
 * queues for packets to be sent, and for received packets waiting for some
 * further treatment.  A network also manages a pool of sockets that can be
 * used to send the probes.
 *
 * Currently packets are generated through a RAW socket only, and replies are
 * captured by a sniffer. We could envisage adding more types of sockets, and
 * the corresponding return channels if needed. Finally, this is also the
 * place where a packet scheduler might be implemented (rate limits, etc.).
 */

#include "queue.h"       // queue_t
#include "socketpool.h"  // socketpool_t
#include "sniffer.h"     // sniffer_t
#include "dynarray.h"    // dynarray_t
#include "optparse.h"    // opt_spec
#include "probe_group.h" // probe_group_t

// If no matching reply has been sniffed in the next 3 sec, we
// consider that we won't never sniff such a reply. The
// corresponding probe is discarded. This timer can be accessed
// thanks to network_set_timeout() and network_get_timeout().

#define NETWORK_DEFAULT_TIMEOUT 3

// network commandline options stuff
extern const double   wait[];
#define HELP_w "Set the number of seconds to wait for response to a probe (default is 5.0)"

/**
 * \struct network_t
 * \brief Structure describing a network
 */

// Two possible approaches
// We choose (1) because this is more efficient
//
// ---------------------------------------------------------------------------
// 1) Delegate probe_t and packet_t frees to upper layers
// ---------------------------------------------------------------------------
//
// *** Advantages / drawbacks
//
// + More efficient (in terms of memory, less dup, etc.)
// - Network layer can be impacted by upper layer
// - Upper layers must never push several time the same probe_t instance
//   and never alter the nested bytes once the probe is sent.
//
// *** Probes memory management:
//
// The network layer..must never free probe (e.g. probe_t instances referenced
// in network->probes and in network->sendq) since they are allocated by the
// upper layers.
//
// Upper layers must never alters probe passed to the network layer..while they
// are in flight, otherwise the network layer..won't be able to match replies
// with their corresponding probes.
//
// If needed, upper layers could duplicate these probe by using probe_dup.
//
// *** Replies memory management:
//
// The network layer..do not free by itself packet_t referenced in
// network->recvq since they'll be wrapped in probe_t by pt_loop. Freeing those
// probe_t instance will free the corresponding packet_t instance in the
// network layer..
//
// Upper layer must never alter reply raised by PROBE_REPLY event otherwise the
// corresponding packet_t instance (stored in the network layer.) will also be
// altered.
//
// If needed, upper layers could duplicate these probe by using probe_dup.
//
// ---------------------------------------------------------------------------
// 2) Duplicate probe_t instance passed to the network layer..and duplicate
// packet_t instance raised to pt_loop
// ---------------------------------------------------------------------------
//
// *** Advantages / drawbacks
//
// - Less efficient
// + Network layer cannot be impacted by upper layers. Thus, upper layers can
//   alter probe_t instances (probes & replies) passed and received from the
//   network layer..
// + Upper layer can push several times the same probe_t without using
//   probe_dup.
//
// *** Probe and replies memory management:
//
// The network layer..has to free its probe_t and packet_t by itself in
// network_free().  Each probe_t instance is only referenced once (either in
// network->sendq if it is not yet sent, or either in network->probes if it is
// in flight).
//
// Matching probes (see network_get_matching_probe) must be freed once
// duplicated and raised to the upper layers, or move in a dedicated
// dynarray for archive or duplicate detection purposes.

typedef struct network_s {
    socketpool_t  * socketpool;        /**< Pool of sockets used by this network */
    queue_t       * sendq;             /**< Queue containing packet to send  (probe_t instances) */
    queue_t       * recvq;             /**< Queue containing received packet (packet_t instances) */
    sniffer_t     * sniffer;           /**< Sniffer to use on this network */
    dynarray_t    * probes;            /**< Probes in transit, from the oldest probe_t instance to the youngest one. */
    int             timerfd;           /**< Used for probe timeouts. Linux specific. Activated when a probe timeout occurs */
    uint16_t        last_tag;          /**< Last probe ID used */
    double          timeout;           /**< The timeout value used by this network (in seconds) */
    int             scheduled_timerfd; /**< Used for probe delays. Activated when a probe delay occurs */
    probe_group_t * group_probes;      /**< Structure of probe_group may contain a group af probes or a group of groups of probes */
} network_t;

/**
 * \brief Get the commandline options related to the layer network
 * \returna pointer to a tructure containing the options
 */

// TODO rename network_get_opt_spec
struct opt_spec * network_get_cl_options();

/**
 * \brief Create a new network structure
 * \return The newly created network layer.
 */

network_t* network_create(void);

/**
 * \brief Delete a network structure
 * \param network The network layer..
 */

void network_free(network_t * network);

/**
 * \brief Retrieve the timeout set in a network_t instance.
 * \param network The network layer..
 * \return The value of the timeout used by the network.
 */

double network_get_timeout(const network_t * network);

/**
 * \brief Set a new timeout for the network structure.
<<<<<<< HEAD
 * \param network The network_t instance we're updating.
=======
 * \param network The network layer..
>>>>>>> origin/akram
 * \param new_timeout The new timeout.
 */

void network_set_timeout(network_t * network, double new_timeout);

/**
 * \brief Retrieve the file descriptor activated whenever a
 *   packet is ready to be sent.
 * \param network The network layer..
 * \return The corresponding file descriptor.
 */

int network_get_sendq_fd(network_t * network);

/**
 * \brief Retrieve the file descriptor activated whenever a
 *   packet_t instance has been sniffed
 * \param network The network layer..
 * \return The corresponding file descriptor
 */

int network_get_sniffer_fd(network_t * network);

/**
 * \brief Retrieve the file descriptor activated whenever a
 *   packet_t instance has been pushed in the recv queue.
 * \param network The network layer..
 * \return The corresponding file descriptor
 */

int network_get_recvq_fd(network_t * network);

/**
 * \brief Retrieve the file descriptor activated whenever a
 *   timeout occurs.
 * \param network The network layer..
 * \return The corresponding file descriptor
 */

int network_get_timerfd(network_t * network);

/**
 * \brief Retrieve the file descriptor activated whenever a
 *   delay occurs.
 * \param network The network layer..
 * \return The corresponding file descriptor
 */

int network_get_group_timerfd(network_t * network);

/**
 * \brief Retrieve the tree of probes handled by this
 *   network instance
 * \param network The network layer..
 * \return a Pointer to the tree of group of probes
 */

probe_group_t * network_get_group_probes(network_t * network);

/**
 * \brief Send the next packet stored network->sendq
 * \param network The network layer..
 * \return true iif successfull
 */

bool network_process_sendq(network_t * network);

/**
 * \brief Process received packets: match them with a probe, or discard them.
 * In practice, the receive queue stores all the packets handled by the sniffer.
 * \param network The network layer.
 * \return true iif successful
 */

bool network_process_recvq(network_t * network);

/**
 * \brief Make the network layer..query its embedded sniffer instance in order
 *   to fetch a received packet.
 * \param network The network layer..
 * \param protocol_id The family of the packet to fetch (IPPROTO_ICMP, IPPROTO_ICMPV6)
 */

void network_process_sniffer(network_t * network, uint8_t protocol_id);

/**
 * \brief Drop the oldest flying probe (if any) attached to a network_t
 *    instance. The oldest probe is removed from network->probes
 *    and network->timerfd is refreshed to manage the next timeout
 *    if there is still at least one flying probe.
 * \param network The network layer.
 * \return true iif successful
 */

bool network_drop_expired_flying_probe(network_t * network);

/**
 * \brief handle the scheduled probes when network->scheduled_timerfd is activated
 * \param network The network layer.
 */

void network_process_scheduled_probe(network_t * network);

/**
 * \brief Retrieve the next delay to send scheduled probes
 * \param network The network layer.
 * \return the next delay.
 */

double network_get_next_scheduled_probe_delay(const network_t * network);

/**
 * \brief Refresh the newtork->scheduled_timerfd to next delay value
 * \param network Pointer to handled network instance
 * \param delay The new delay
 * return true iif successful
 */

bool network_update_scheduled_timer(network_t * network, double delay);

/**
 * \brief Refresh timerfd to a new  delay value
 * \param timerfd the timer file descriptor to update
 * \param delay The new delay
 * return true iif successful
 */

bool update_timer(int timerfd, double delay);

/**
 * \brief Retrieve the socket file descriptor related to the ICMPv4
 *    raw socket managed by network->sniffer.
 * \param network The network layer..
 * \return The corresponding socket file descriptor.
 */

int network_get_icmpv4_sockfd(network_t * network);

/**
 * \brief Retrieve the socket file descriptor related to the ICMPv6
 *    raw socket managed by network->sniffer.
 * \param network The network layer..
 * \return The corresponding socket file descriptor.
 */

int network_get_icmpv6_sockfd(network_t * network);

#endif
