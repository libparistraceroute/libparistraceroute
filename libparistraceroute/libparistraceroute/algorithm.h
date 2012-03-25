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

/**
 * \enum status_t
 * \brief Algorithm status constants
 */
typedef enum {
    STARTED,    /*!< Algorithm is started */
    STOPPED,    /*!< Algorithm is stopped */
    ERROR       /*!< An error occurred */
} status_t;

struct algorithm_s;

/**
 * \enum caller_type_t
 * \brief Type of caller
 */
typedef enum {
    CALLER_ALGORITHM,   /*!< caller is an algorithm */
} caller_type_t;

/**
 * \struct caller_t
 * \brief Structure describing the caller for an entity.
 */
typedef struct {
    caller_type_t type; /*!< Type of the entity that called the algorithm instance */
    union {
        struct algorithm_instance_s *caller_algorithm;  /*!< Pointer to the caller algorithm */
    };
} caller_t;

/**
 * \struct algorithm_instance_t
 * \brief Structure describing a running instance of an algorithm.
 */
typedef struct algorithm_instance_s {
    unsigned int id;                /*!< Unique identifier */
    struct algorithm_s *algorithm;  /*!< Pointer to the type of algorithm */
    void *options;                  /*!< Pointer to an option structure specific to the algorithm */
    probe_t *probe_skel;            /*!< Skeleton for probes forged by this algorithm instance */
    void *data;                     /*!< Algorithm-defined data */
    dynarray_t *events;             /*!< An array of events received by the algorithm */
    caller_t *caller;               /*!< Reference to the entity that called the algorithm */
    struct pt_loop_s *loop;         /*!< Pointer to a library context */
} algorithm_instance_t;

/**
 * \struct algorithm_t
 * \brief Structure representing an algorithm
 */
typedef struct algorithm_s {
    char* name;                                                                 /*!< Algorithm name */
    void (*handler)(struct pt_loop_s *loop, algorithm_instance_t *instance);    /*!< Main handler function */
} algorithm_t;


/******************************************************************************
 * algorithm_t
 ******************************************************************************/

/**
 * \brief Search for available algorithms by name.
 * \param name Name of the algorithm
 * \return An algorithm_t structure
 */
algorithm_t* algorithm_search(char *name);

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

/******************************************************************************
 * algorithm_instance_t
 ******************************************************************************/

/**
 * \brief Create an instance of an algorithm
 * \param loop A pointer to a library main loop context
 * \param algorithm A pointer to a structure representing an algorithm
 * \param options A set of algorithm-specific options
 * \param probe_skel Skeleton for probes crafted by the algorithm
 * \return A pointer to an algorithm instance
 */
algorithm_instance_t* algorithm_instance_create(struct pt_loop_s *loop, algorithm_t *algorithm, void *options, probe_t *probe_skel);

/**
 * \brief Free an algorithm instance
 * \param instance The instance to be free'd
 */
void algorithm_instance_free(algorithm_instance_t *instance);

/**
 * \brief Compare two instances of an algorithm
 * \param instance1 Pointer to the first instance
 * \param instance2 Pointer to the second instance
 * \return Respectively -1, 0 or 1 if instance1 is lower, equal or bigger than
 *     instance2
 * 
 * The comparison is done on the instance id.
 *
 */
int algorithm_instance_compare(const void *instance1, const void *instance2);

void* algorithm_instance_get_options(algorithm_instance_t *instance);
void* algorithm_instance_get_probe_skel(algorithm_instance_t *instance);
void* algorithm_instance_get_data(algorithm_instance_t *instance);
void algorithm_instance_set_data(algorithm_instance_t *instance, void *data);
event_t** algorithm_instance_get_events(algorithm_instance_t *instance);
void algorithm_instance_clear_events(algorithm_instance_t *instance);
unsigned int algorithm_instance_get_num_events(algorithm_instance_t *instance);

void algorithm_instance_add_event(algorithm_instance_t *instance, event_t *event);

/******************************************************************************
 * pt_loop_t
 ******************************************************************************/

/**
 * \brief process algorithm events (private)
 */
// visitor for twalk
void pt_process_algorithms_instance(const void *node, VISIT visit, int level);
void pt_algorithm_instance_iter(struct pt_loop_s *loop, void (*action) (const void *, VISIT, int));
algorithm_instance_t * pt_algorithm_add(struct pt_loop_s *loop, char *name, void *options, probe_t *probe_skel);
void pt_algorithm_terminate(struct pt_loop_s *loop, algorithm_instance_t *instance);


#endif
