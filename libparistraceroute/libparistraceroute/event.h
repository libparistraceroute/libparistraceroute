#ifndef EVENT_H
#define EVENT_H

// Do not include "algorithm.h" to avoid mutual inclusion

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

/**
 * \enum event_type_t
 * \brief Enum to denote type of event
 */

typedef enum {
    // Events raised by the network layer
    // Such events are dispatched to the appropriate algorithm instances
    PROBE_REPLY,           /**< A reply has been sniffed           */
    PROBE_TIMEOUT,         /**< No reply sniffed for a given probe */

    // Events handled the algorithm layer
    ALGORITHM_INIT,        /**< An algorithm can start             */
    ALGORITHM_TERMINATED,  /**< An algorithm must terminate        */

    // Events raised by the algorithm layer
    ALGORITHM_EVENT,       /**< An algorithm has raised an event   */
    ALGORITHM_ERROR        /**< An error has occured               */
} event_type_t;

/**
 * \struct event_t
 * \brief Structure representing an event
 */

typedef struct {
    event_type_t                  type;               /**< Event type */
    void                        * data;               /**< Data carried by the event */
    void                       (* data_free)(void *); /**< Called in event_free to release data. Ignored if NULL. */
    struct algorithm_instance_s * issuer;             /**< Instance which has raised the event. NULL if raised by pt_loop. */
} event_t;

/** 
 * \brief Create a new event structure
 * \param type Event type
 * \param data Data that must be carried by this event 
 * \param issuer
 * \return Newly created event structure
 */

event_t * event_create(
    event_type_t type,
    void * data,
    struct algorithm_instance_s * issuer,
    void (*data_free) (void * data)
);

/**
 * \brief Release an event when done
 * \param event The event to destroy
 */

void event_free(event_t * event);

#endif
