#ifndef EVENT_H
#define EVENT_H

/**
 * \file event.h
 * \brief
 *   Event structures are used to notify an algorithm when
 *   a particular event arises and to carry data between each
 *   instances.
 *
 *   Events type are mainly here to manage libparistraceroute loop
 *   (memory, network ...) and are thus independant of the what
 *   does the algorithm.
 *
 *   Specific-algorithm event are nested in a ALGORITHM_ANSWER event.
 */

#include "probe.h"

/**
 * \enum event_type_t
 * \brief Enum to denote type of event
 */

typedef enum {

    PROBE_REPLY,
    PROBE_TIMEOUT,

    ALGORITHM_INIT,
    ALGORITHM_TERMINATED,
    ALGORITHM_EVENT,
    ALGORITHM_ERROR
} event_type_t;

/**
 * \struct event_t
 * \brief Structure representing an event
 */

typedef struct {
    event_type_t type;    /**< Enum holding the event type */
    
    /**
     * Pointer to event parameters.
     * PROBE_REPLY_RECEIVED : reply_received_params_t *
     * ALGORITHM_ANSWER     : see called algorithm implementation
     */
    void * data;

    struct algorithm_instance_s * issuer;
} event_t;

/** 
 * \brief Create a new event structure
 * \param type Event type
 * \param data Data that must be carried by this event 
 * \param issuer
 * \return Newly created event structure
 */

event_t *event_create(event_type_t type, void * data, struct algorithm_instance_s * issuer);

/**
 * \brief Release an event when done
 * \param event The event to destroy
 */

void event_free(event_t *event);

#endif
