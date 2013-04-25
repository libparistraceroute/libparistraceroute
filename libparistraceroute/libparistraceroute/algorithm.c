#include <stdlib.h>
#include <string.h>
#include <search.h>

#include "algorithm.h"
#include "dynarray.h"
#include "event.h"
#include "pt_loop.h"

/* static ? */
void *algorithms_root;

/**
 * \brief Create an instance of an algorithm
 * \param loop A pointer to a library main loop context
 * \param algorithm A pointer to a structure representing an algorithm
 * \param options A set of algorithm-specific options
 * \param probe_skel Skeleton for probes crafted by the algorithm
 * \return A pointer to an algorithm instance
 */

static algorithm_instance_t * algorithm_instance_create(
    pt_loop_t   * loop,
    algorithm_t * algorithm,
    void        * options,
    probe_t     * probe_skel
);

/**
 * \brief Free an algorithm instance
 * \param instance The instance to be free'd
 */

static void algorithm_instance_free(algorithm_instance_t * instance);

/**
 * \brief Compare two instances of an algorithm
 *  The comparison is done on the instance id.
 * \param instance1 Pointer to the first instance
 * \param instance2 Pointer to the second instance
 * \return Respectively -1, 0 or 1 if instance1 is lower, equal or bigger than
 *     instance2
 */

static int algorithm_instance_compare(
    const algorithm_instance_t * instance1,
    const algorithm_instance_t * instance2
);

/**
 * \brief Register an algorithm instance in the main loop
 * \param instance The instance that we register
 */

static algorithm_instance_t * pt_algorithm_instance_add(
    struct pt_loop_s     * loop,
    algorithm_instance_t * instance
);


//--------------------------------------------------------------------
// algorithm_t
//--------------------------------------------------------------------

int algorithm_compare(const void * algorithm1, const void * algorithm2)
{
    return strcmp(
        ((const algorithm_t *) algorithm1)->name,
        ((const algorithm_t *) algorithm2)->name
    );
}

algorithm_t* algorithm_search(const char * name)
{
    algorithm_t **algorithm, search;

    if (!name) return NULL;
    search.name = name;
    algorithm = tfind(&search, &algorithms_root, algorithm_compare);

    return algorithm ? *algorithm : NULL;
}

void algorithm_register(algorithm_t * algorithm)
{
    // Insert the algorithm in the tree if the keys does not yet exist
    tsearch(algorithm, &algorithms_root, algorithm_compare);
}

//--------------------------------------------------------------------
// algorithm_instance_t (internal usage)
//--------------------------------------------------------------------

static algorithm_instance_t * algorithm_instance_create(
    pt_loop_t   * loop,
    algorithm_t * algorithm,
    void        * options,
    probe_t     * probe_skel
) {
    algorithm_instance_t * instance = malloc(sizeof(algorithm_instance_t));
    if (instance) {
        instance->id         = loop->next_algorithm_id++;
        instance->algorithm  = algorithm;
        instance->options    = options;
        instance->probe_skel = probe_skel;
        instance->data       = NULL;
        instance->events     = dynarray_create();
        instance->caller     = NULL;
        instance->loop       = loop;
    }
    return instance;
}

static void algorithm_instance_free(algorithm_instance_t * instance)
{
    // caller may have to free 
    // - the options passed to this instance (only if instanciated by libparistraceroute)
    // - the probes related to this instance

    if (instance) {
        if (instance->probe_skel) {
            probe_free(instance->probe_skel);
        }
        dynarray_free(instance->events, (ELEMENT_FREE) event_free);
        free(instance);
    }
}

static inline int algorithm_instance_compare(
    const algorithm_instance_t * instance1,
    const algorithm_instance_t * instance2
) {
    // Both structures need to be of the same type in memory
    return instance1->id - instance2->id;
}


static inline algorithm_instance_t * pt_algorithm_instance_add(
    struct pt_loop_s     * loop,
    algorithm_instance_t * instance
) {
    return tsearch(
        instance,
        &loop->algorithm_instances_root,
        (int (*)(const void *, const void *)) algorithm_instance_compare
    );
}

//--------------------------------------------------------------------
// algorithm_instance_t
//--------------------------------------------------------------------

inline void * algorithm_instance_get_options(algorithm_instance_t *instance) {
    return instance ? instance->options : NULL;
}

inline probe_t * algorithm_instance_get_probe_skel(algorithm_instance_t *instance) {
    return instance ? instance->probe_skel : NULL;
}

inline void * algorithm_instance_get_data(algorithm_instance_t * instance) {
    return instance ? instance->data : NULL;
}

inline void algorithm_instance_set_data(algorithm_instance_t * instance, void * data) {
    if (instance) instance->data = data;
}

inline event_t ** algorithm_instance_get_events(algorithm_instance_t * instance) {
    return instance ?
        (event_t**) dynarray_get_elements(instance->events) :
        NULL;
}

inline void algorithm_instance_clear_events(algorithm_instance_t * instance) {
    if (instance) {
        dynarray_clear(instance->events, (void (*)(void*)) event_free);
    }
}

inline unsigned int algorithm_instance_get_num_events(algorithm_instance_t * instance) {
    return instance && instance->events ? instance->events->size : 0;
}

//--------------------------------------------------------------------
// pt_loop: user interface 
//--------------------------------------------------------------------

void pt_process_algorithms_instance(const void * node, VISIT visit, int level)
{
    algorithm_instance_t * instance = *((algorithm_instance_t **) node);
    unsigned int i, num_events;
    uint64_t        ret;
    ssize_t         count;
    
    // Save temporarily this algorithm context
    instance->loop->cur_instance = instance;

    // Execute algorithm handler for events of each algorithm
    num_events = dynarray_get_size(instance->events);
    for (i = 0; i < num_events; i++) {
        event_t *event;

        count = read(instance->loop->eventfd_algorithm, &ret, sizeof(ret));
        if (count == -1)
            return;

        event = dynarray_get_ith_element(instance->events, i);
        instance->algorithm->handler(instance->loop, event, &instance->data, instance->probe_skel, instance->options);
    }

    // Restore the algorithm context
    instance->loop->cur_instance = NULL;

    // Flush events queue
    algorithm_instance_clear_events(instance);
}

// Notify the called algorithm that it can start

algorithm_instance_t * pt_algorithm_add(
    struct pt_loop_s * loop,
    const char       * name,
    void             * options,
    probe_t          * probe_skel
) {
    algorithm_t          * algorithm;
    algorithm_instance_t * instance;

    if (!(algorithm = algorithm_search(name))) {
        return NULL;  // No such algorithm
    }
    
    // If the probe skeleton does not exist, create it.
    if (!probe_skel) {
        probe_skel = probe_create();
    }

    // Create a new instance of a running algorithm
    instance = algorithm_instance_create(loop, algorithm, options, probe_skel);

    // We need to queue a new event for the algorithm: it has been started
    pt_algorithm_throw(NULL, instance, event_create(ALGORITHM_INIT, NULL, NULL));

    // Add this algorithms to the list of handled algorithms
    pt_algorithm_instance_add(loop, instance);

    return instance;
}

void pt_algorithm_throw(
    pt_loop_t            * loop, 
    algorithm_instance_t * instance,
    event_t              * event
) {
    if (event) {
        if (instance) {
            // Enqueue an algorithm event
            dynarray_push_element(instance->events, event);
            eventfd_write(instance->loop->eventfd_algorithm, 1);
        } else if (loop) {
            // Enqueue an user event
            dynarray_push_element(loop->events_user, event);
            eventfd_write(loop->eventfd_user, 1);
        }
    }
}

inline void pt_algorithm_free(algorithm_instance_t * instance) {
    // Notify the called algorithm that it can free its data
    pt_algorithm_throw(NULL, instance, event_create(ALGORITHM_TERMINATED, NULL, NULL));

    // Release this instance from the memory
    algorithm_instance_free(instance);
}

//--------------------------------------------------------------------
// Internal usage (see pt_loop.c) 
//--------------------------------------------------------------------

inline void pt_algorithm_instance_iter(
    pt_loop_t * loop,
    void     (* action) (const void *, VISIT, int))
{
    twalk(loop->algorithm_instances_root, action);
}


// Notify the caller that the current algorithm has ended
inline void pt_algorithm_terminate(
    struct pt_loop_s     * loop,
    algorithm_instance_t * instance
) {
    pt_algorithm_throw(
        instance->caller ? NULL : loop,
        instance->caller,
        event_create(ALGORITHM_TERMINATED, NULL, NULL)
    );
}


