#ifndef ALGORITHM_H
#define ALGORITHM_H

/** 
 * \file algorithm.h
 * \brief Header file for algorithms and algorithm instances.
 */

#include <search.h>
#include "probe.h"
#include "event.h"
#include "dynarray.h"
#include "pt_loop.h"

/**
 * \enum status_t
 * \brief Algorithm status constants
 */

typedef enum {
    STARTED,    /*!< Algorithm instance is started */
    STOPPED,    /*!< Algorithm instance is stopped */
    ERROR       /*!< An error has occurred */
} status_t;

/**
 * \struct algorithm_instance_t
 * \brief Structure describing a running instance of an algorithm.
 */

typedef struct algorithm_instance_s {
    unsigned int                  id;         /*!< Unique identifier */
    struct algorithm_s          * algorithm;  /*!< Pointer to the type of algorithm */
    void                        * options;    /*!< Pointer to an option structure specific to the algorithm */
    probe_t                     * probe_skel; /*!< Skeleton for probes forged by this algorithm instance */
    void                        * data;       /*!< Internal algorithm data */
    void                        * outputs;    /*!< Data exposed to the caller and filled by the instance */ 
    dynarray_t                  * events;     /*!< An array of events received by the algorithm */
    struct algorithm_instance_s * caller;     /*!< Reference to the entity that called the algorithm (NULL if called by pt_loop) */
    struct pt_loop_s            * loop;       /*!< Pointer to a library context */
} algorithm_instance_t;

/**
 * \struct algorithm_t
 * \brief Structure representing an algorithm.
 * The handler is called everytime an event concerning this instance is raised.
 */

typedef struct algorithm_s {
    char* name;                                                                 /*!< Algorithm name */
    int (*handler)(pt_loop_t *loop, event_t *event, void **pdata, probe_t *skel); /*!< Main handler function */
} algorithm_t;

//--------------------------------------------------------------------
// algorithm_t 
//--------------------------------------------------------------------

/**
 * \brief Search for available algorithms by name.
 * \param name Name of the algorithm
 * \return An algorithm_t structure
 */

algorithm_t* algorithm_search(char * name);

/**
 * \brief Register an algorithm to be used by the library.
 * \param algorithm Pointer to a structure representing the algorithm
 */

void algorithm_register(algorithm_t *algorithm);

#define ALGORITHM_REGISTER(MOD)	\
static void __init_ ## MOD (void) __attribute__ ((constructor));	\
static void __init_ ## MOD (void) {	\
	algorithm_register(&MOD); \
}

//--------------------------------------------------------------------
// algorithm_instance_t
//--------------------------------------------------------------------

void    *  algorithm_instance_get_options   (algorithm_instance_t * instance);
probe_t *  algorithm_instance_get_probe_skel(algorithm_instance_t * instance);
void    *  algorithm_instance_get_data      (algorithm_instance_t * instance);
event_t ** algorithm_instance_get_events    (algorithm_instance_t * instance);
unsigned   algorithm_instance_get_num_events(algorithm_instance_t * instance);
void       algorithm_instance_set_data      (algorithm_instance_t * instance, void * data);
void       algorithm_instance_clear_events  (algorithm_instance_t * instance);

//--------------------------------------------------------------------
// pt_loop: user interface 
//--------------------------------------------------------------------

/**
 * \brief Throw an event to the libparistraceroute loop or to an instance
 * \param loop The libparistraceroute loop.
 *    Pass NULL if this event is raised for an instance.
 * \param instance The instance that must receives the event.
 *    Pass NULL if this event has to be sent to the loop.
 * \param event The event that must be raised
 */

void pt_algorithm_throw(
    struct pt_loop_s     * loop, 
    algorithm_instance_t * instance,
    event_t              * event
);

/**
 * \brief Notify an instance to make it release memory
 * \param instance The instance we are freeing
 */

void pt_algorithm_free(algorithm_instance_t * instance);

/**
 * \brief Add a new algorithm instance in the libparistraceroute loop.
 * \param loop The libparistraceroute loop
 * \param name Name of the algorithm (for instance 'traceroute').
 * \param options Options passed to this instance.
 * \param probe_skel Probe skeleton that constrains the way the packets
 *   produced by this instance will be forged.
 * \return A pointer to the instance, NULL otherwise. 
 */

algorithm_instance_t * pt_algorithm_add(
    struct pt_loop_s * loop,
    char             * name,
    void             * options,
    probe_t          * probe_skel
);

//--------------------------------------------------------------------
// Internal usage (see pt_loop.c) 
//--------------------------------------------------------------------

/**
 * \brief process algorithm events (internal usage, see visitor for twalk)
 * \param node Current instance
 * \param visit Unused
 * \param level Unused
 */

void pt_process_algorithms_instance(
    const void * node,
    VISIT        visit,
    int          level
);

/**
 * \brief process algorithm events (internal usage, see visitor for twalk)
 * \param loop The libparistraceroute loop
 * \param action A pointer to a function that process the current instance, e.g.
 *    that dispatches the events related to this instance.
 */

void pt_algorithm_instance_iter(
    struct pt_loop_s * loop,
    void (*action) (const void *, VISIT, int)
);

/**
 * \brief Send a ALGORITHM_TERMINATED event to the caller which may be
 *   either a calling algorithm or either the libparistraceroute loop 
 *   if this algorithm has been called by the user program.
 * \param loop The libparistraceroute loop
 */

void pt_algorithm_terminate(
    struct pt_loop_s     * loop,
    algorithm_instance_t * instance
);

#endif
