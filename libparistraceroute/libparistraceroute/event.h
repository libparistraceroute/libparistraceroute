#ifndef EVENT_H
#define EVENT_H

/**
 * \file event.h
 * \brief Header file providing a data structure for events
 */

#include "probe.h"

typedef struct reply_received_params_s {
    probe_t * probe;       /**< Probe related to this event */
 // reply_t * reply;       /**< The reply related to this probe */
} reply_received_params_t;


/**
 * \enum event_type_t
 * \brief Enum to denote type of event
 */

typedef enum {
    /** Algorithm initialisation event */
    ALGORITHM_INIT,
    /** Algorithm terminated event */
    ALGORITHM_TERMINATED,
    /** Reply received event */
    PROBE_REPLY_RECEIVED
} event_type_t;

/**
 * \struct event_t
 * \brief Structure representing an event
 */

typedef struct {
    /** Enum holding the event type */
    event_type_t type;
    /** Pointer to event parameters. REPLY_RECEIVED: reply_received_params_t */
    void *params;
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
