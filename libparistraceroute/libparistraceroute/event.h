#ifndef EVENT_H
#define EVENT_H

/**
 * \file event.h
 * \brief Header file providing a data structure for events
 */

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
    REPLY_RECEIVED
} event_type_t;

/**
 * \struct event_t
 * \brief Structure representing an event
 */
typedef struct {
    /** Enum holding the event type */
    event_type_t type;
    /** Pointer to event parameters */
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
