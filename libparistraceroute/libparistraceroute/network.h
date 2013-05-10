#ifndef NETWORK_H
#define NETWORK_H

/**
 * \file network.h
 * \brief Header for network functions.
 *
 * A network represents the interface between the "packets and socket" layer and
 * the real network / the Internet. It is responsible for maintaining queues for
 * packets to be sent, and for received packets waiting for some further treatment.
 * A network also manages a pool of sockets that can be used to send the probes.
 *
 * Currently packets are generated through a RAW socket only, and replies are
 * captured by a sniffer. We could envisage adding more types of sockets, and
 * the corresponding return channels if needed. Finally, this is also the place 
 * where a packet scheduler might be implemented (rate limits, etc.).
 */

#include <stdint.h>

#include "packet.h"
#include "queue.h"
#include "socketpool.h"
#include "sniffer.h"
#include "dynarray.h"

// If no matching reply has been sniffed in the next 3 sec, we
// consider that we won't never sniff such a reply. The
// corresponding probe is discarded. This timer can be accessed
// thanks to network_set_timeout() and network_get_timeout().

#define NETWORK_DEFAULT_TIMEOUT 3

/**
 * \struct network_t
 * \brief Structure describing a network
 */

typedef struct network_s {
    socketpool_t * socketpool; /**< Pool of sockets used by this network */
    queue_t      * sendq;      /**< Queue containing packet to send  (probe_t instances) */
    queue_t      * recvq;      /**< Queue containing received packet (packet_t instances) */
    sniffer_t    * sniffer;    /**< Sniffer to use on this network */
    dynarray_t   * probes;     /**< Probes in transit, from the oldest probe_t instance to the youngest one */
    int            timerfd;    /**< Used for probe timeouts. Linux specific. Activated when a probe timeout occurs */
    uint16_t       last_tag;   /**< Last probe ID used */
    double         timeout;    /**< The timeout value used by this network (in seconds) */
} network_t;

/**
 * \brief Set a new timeout for the network structure
 * \param network a Pointer to the network structure to use
 * \param new_timeout value of the new timeout to set
 */

void network_set_timeout(network_t * network, double new_timeout);

/**
 * \brief get the timeout used by a network structure
 * \param network a Pointer to the used network structure
 * \return the value of the timeout used by the network
 */

double network_get_timeout(const network_t * network);

/**
 * \brief Create a new network structure
 * \return A newly created network_t structure
 */

network_t* network_create(void);

/**
 * \brief Delete a network structure
 * \param network Pointer to the network structure to delete
 */

void network_free(network_t * network);

/**
 * \brief Get file descriptor for sending to network
 * \param network Pointer to the network to use
 * \return SendQ file descriptor
 */

int network_get_sendq_fd(network_t * network);

/**
 * \brief Get file descriptor for receiving from network
 * \param network Pointer to the network to use
 * \return RecvQ file descriptor
 */

int network_get_recvq_fd(network_t * network);

/**
 * \brief Get file description for the sniffer.
 * \param network Pointer to the network to use
 * \return sniffer file descriptor
 */

int network_get_sniffer_fd(network_t * network);

int network_get_timerfd(network_t *network);

/**
 * \brief Send the next packet on the queue
 * \param network The network to use for the queue and for sending
 * \return true iif successfull 
 */

bool network_process_sendq(network_t * network);

/**
 * \brief Process received packets: match them with a probe, or discard them.
 * \param network Pointer to a network structure
 * In practice, the receive queue stores all the packets received by the sniffer.
 * \return true iif successful 
 */

bool network_process_recvq(network_t * network);

void network_process_sniffer(network_t *network);

/**
 * \brief Drop the oldest probe attached to a network instance.
 *    The oldest probe if any is removed from network->probe
 *    and network->timerfd is refreshed to manage the next timeout
 *    if there is still at least one flying probe. 
 * \param network Pointer to a network structure
 * \return true iif successful
 */

bool network_process_timeout(network_t * network);

// XXX
uint16_t network_get_available_tag(network_t *network);

#endif
