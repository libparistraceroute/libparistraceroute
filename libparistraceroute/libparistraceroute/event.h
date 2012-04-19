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

typedef struct reply_received_params_s {
    probe_t * probe;       /**< Probe related to this event */
} reply_received_params_t;

/**
 * \enum event_type_t
 * \brief Enum to denote type of event
 */

typedef enum {

    // Events raised by an upper layers (caller instances, pt_loop...)
    
    ALGORITHM_INIT,       /**< Start the algorithm (allocate memory...)        */
    ALGORITHM_FREE,       /**< This instance has to free the memory            */

    // Events raised by a lower layers (called instances, network...)

    ALGORITHM_ANSWER,     /**< A specific-algorithm event has arised           */
    ALGORITHM_FAILURE,    /**< A called instance has crashed                   */
    ALGORITHM_TERMINATED, /**< A called instance has finished                  */
    PROBE_REPLY_RECEIVED  /**< Network layer has got a probe for this instance */

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
    void * params;
} event_t;

/** 
 * \brief Create a new event structure
 * \param type Event type
 * \param params Pointer to event parameters
 * \return Newly created event structure
 */

event_t *event_create(event_type_t type, void *params);

/**
 * \brief Release an event when done
 * \param event The event to destroy
 */

void event_free(event_t *event);

#endif
