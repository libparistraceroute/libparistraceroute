#ifndef LIBPT_ALGORITHM_H
#define LIBPT_ALGORITHM_H

/**
 * \file algorithm.h
 * \brief Header file for algorithms and algorithm instances.
 */

#include <search.h>     // VISIT

#include "probe.h"      // probe_t
#include "event.h"      // event_t
#include "dynarray.h"   // dynarray_t
#include "pt_loop.h"    // pt_loop_t
#include "optparse.h"   // opt_spec

#ifdef __cplusplus
extern "C" {
#endif

/**
 * \enum status_t
 * \brief Algorithm status constants
 */

typedef enum {
    STARTED,    /**< Algorithm instance is started */
    STOPPED,    /**< Algorithm instance is stopped */
    ERROR       /**< An error has occurred         */
} status_t;

/**
 * \struct algorithm_t
 * \brief Structure representing an algorithm.
 * The handler is called everytime an event concerning this instance is raised.
 */

typedef struct algorithm_s {
    const char * name;                        /**< Algorithm name */
    int (*handler)(
        pt_loop_t *  loop,
        event_t   *  event,
        void      ** pdata,
        probe_t   *  skel,
        void      *  poptions
    );                                        /**< Main handler function */
    const struct opt_spec * options;          /**< Options supported by this algorithm */
} algorithm_t;

/**
 * \struct algorithm_instance_t
 * \brief Structure describing a running instance of an algorithm.
 */

typedef struct algorithm_instance_s {
    unsigned int                  id;         /**< Unique identifier */
    algorithm_t                 * algorithm;  /**< Pointer to the type of algorithm */
    void                        * options;    /**< Pointer to an option structure specific to the algorithm */
    probe_t                     * probe_skel; /**< Skeleton for probes forged by this algorithm instance */
    void                        * data;       /**< Internal algorithm data */
    void                        * outputs;    /**< Data exposed to the caller and filled by the instance */
    dynarray_t                  * events;     /**< An array of events received by the algorithm */
    struct algorithm_instance_s * caller;     /**< Reference to the entity that called the algorithm (NULL if called by user program) */
    struct pt_loop_s            * loop;       /**< Pointer to a library context */
} algorithm_instance_t;

//--------------------------------------------------------------------
// algorithm_t
//--------------------------------------------------------------------

/**
 * \brief Search for available algorithms by name.
 * \param name Name of the algorithm
 * \return A pointer to the corresponding algorithm_t structure if any, NULL otherwise
 */

algorithm_t * algorithm_search(const char * name);

/**
 * \brief Register an algorithm to be used by the library.
 * \param algorithm Pointer to a structure representing the algorithm
 */

void algorithm_register(algorithm_t * algorithm);

#define ALGORITHM_REGISTER(MOD)	\
static void __init_ ## MOD (void) __attribute__ ((constructor));	\
static void __init_ ## MOD (void) {	\
	algorithm_register(&MOD); \
}

//--------------------------------------------------------------------
// algorithm_instance_t
//--------------------------------------------------------------------

/**
 * \brief Create an instance of an algorithm
 * \param loop A pointer to a library main loop context
 * \param algorithm A pointer to a structure representing an algorithm
 * \param options A set of algorithm-specific options
 * \param probe_skel Skeleton for probes crafted by the algorithm
 * \return A pointer to an algorithm instance
 */

algorithm_instance_t * algorithm_instance_create(
    pt_loop_t   * loop,
    algorithm_t * algorithm,
    void        * options,
    probe_t     * probe_skel
);

/**
 * \brief Free an algorithm instance
 * \param instance The instance to be free'd
 */

void algorithm_instance_free(algorithm_instance_t * instance);

/**
 * \brief Compare two instances of an algorithm
 *  The comparison is done on the instance id.
 * \param instance1 Pointer to the first instance
 * \param instance2 Pointer to the second instance
 * \return Respectively -1, 0 or 1 if instance1 is lower, equal or bigger than
 *     instance2
 */

int algorithm_instance_compare(
    const algorithm_instance_t * instance1,
    const algorithm_instance_t * instance2
);

void    *  algorithm_instance_get_options   (algorithm_instance_t * instance);
probe_t *  algorithm_instance_get_probe_skel(algorithm_instance_t * instance);
void    *  algorithm_instance_get_data      (algorithm_instance_t * instance);
event_t ** algorithm_instance_get_events    (algorithm_instance_t * instance);
unsigned   algorithm_instance_get_num_events(algorithm_instance_t * instance);
void       algorithm_instance_set_data      (algorithm_instance_t * instance, void * data);
void       algorithm_instance_clear_events  (algorithm_instance_t * instance);

//--------------------------------------------------------------------
// pt_* functions involving an algorithm_instance_t
// Due to mutual header inclusions, they cannot be declared/implemented
// in pt_loop.c and pt_loop.h
//--------------------------------------------------------------------

/**
 * \brief Throw an event from the loop to a given algorithm_instance_t
 * \param loop The libparistraceroute loop.
 *    Pass NULL if this event is raised for an instance.
 * \param instance The instance that must receives the event.
 *    Pass NULL if this event has to be sent to the user program.
 * \param event The event that must be raised
 */

void pt_throw(
    struct pt_loop_s     * loop,
    algorithm_instance_t * instance,
    event_t              * event
);

/**
 * \brief Send a TERM event to the algorithm (to make it release its data from the
 *    memory and unregister this algorithm from the pt_loop_t.
 * \param loop The main loop
 * \param instance The instance we are freeing
 */

void pt_stop_instance(
    struct pt_loop_s     * loop,
    algorithm_instance_t * instance
);

/**
 * \brief Add a new algorithm instance in the libparistraceroute loop.
 * \param loop The libparistraceroute loop
 * \param name Name of the corresponding algorithm (for instance 'traceroute').
 * \param options Options passed to this instance.
 * \param probe_skel Probe skeleton that constrains the way the packets
 *   produced by this instance will be forged.
 * \return A pointer to the instance, NULL otherwise.
 */

algorithm_instance_t * pt_add_instance(
    struct pt_loop_s * loop,
    const char       * name,
    void             * options,
    probe_t          * probe_skel
);

/**
 * \brief Unregister an algorithm instance from the pt_loop.
 *    Data related to the instance is NOT freed.
 * \param loop The libparistraceroute loop.
 * \param instance The algorithm instance.
 */

void pt_del_instance(
    struct pt_loop_s * loop,
    algorithm_instance_t * instance
);

#ifdef __cplusplus
}
#endif

#endif // LIBPT_ALGORITHM_H
