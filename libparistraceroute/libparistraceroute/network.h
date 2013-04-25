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

/******************************************************************************
 * Network
 ******************************************************************************/

/**
 * \struct network_t
 * \brief Structure describing a network
 */

typedef struct network_s {
	
    socketpool_t * socketpool; /**< Pool of sockets used by this network */
    queue_t      * sendq;      /**< Queue for packets to send */
    queue_t      * recvq;      /**< Queue for packets to be received */
    sniffer_t    * sniffer;    /**< Sniffer to use on this network */
    dynarray_t   * probes;     /**< List of probes in transit */
    
    // Timer data
    int            timerfd; // used for probe timeouts, Linux specific

    /* Temp */
    uintmax_t      last_tag;
} network_t;

/**
 *
 */

void network_set_timeout(double new_timeout);

/**
 *
 */

double network_get_timeout(void);

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
 * \return 0
 */

int network_process_sendq(network_t * network);

/**
 * \brief Recieve the next packet on the queue
 * \param network The network to use for the queue and for receiving
 * \return 0
 */

int network_process_recvq(network_t * network);

int network_process_sniffer(network_t *network);

int network_process_timeout(network_t * network);

// XXX
int network_get_available_tag(network_t *network);

#endif
