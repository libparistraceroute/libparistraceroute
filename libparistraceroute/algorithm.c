#include "config.h"

#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <search.h>
#include <errno.h>

#include "algorithm.h"
#include "dynarray.h"
#include "event.h"
#include "pt_loop.h"

static void * algorithms_root = NULL;

//--------------------------------------------------------------------
// algorithm_t (internal usage)
//--------------------------------------------------------------------

static int algorithm_compare(
    const algorithm_t * algorithm1,
    const algorithm_t * algorithm2
) {
    return strcmp(algorithm1->name, algorithm2->name);
}

algorithm_t * algorithm_search(const char * name)
{
    algorithm_t ** algorithm, search;

    if (!name) return NULL;
    search.name = name;
    algorithm = tfind(&search, &algorithms_root, (ELEMENT_COMPARE) algorithm_compare);

    return algorithm ? *algorithm : NULL;
}

void algorithm_register(algorithm_t * algorithm)
{
    // Insert the algorithm in the tree if the keys does not yet exist
    tsearch(algorithm, &algorithms_root, (ELEMENT_COMPARE) algorithm_compare);
}

//--------------------------------------------------------------------
// algorithm_instance_t
//--------------------------------------------------------------------

algorithm_instance_t * algorithm_instance_create(
    pt_loop_t   * loop,
    algorithm_t * algorithm,
    void        * options,
    probe_t     * probe_skel
) {
    algorithm_instance_t * instance;

    if (!loop) {
        errno = EINVAL;
        return NULL;
    }

    if (!(instance = malloc(sizeof(algorithm_instance_t)))) {
        return NULL;
    }

    instance->id         = loop->next_algorithm_id++;
    instance->algorithm  = algorithm;
    instance->options    = options;
    instance->probe_skel = probe_skel;
    instance->data       = NULL;
    instance->events     = dynarray_create();
    instance->caller     = NULL;
    instance->loop       = loop;
    return instance;
}

/**
 * \brief Release an instance of algorithm from the memory
 * \param instance A pointer to an algorithm_instance_t structure
 */

void algorithm_instance_free(algorithm_instance_t * instance) {
    if (instance) {
        algorithm_instance_clear_events(instance);
        free(instance);
    }
}

inline int algorithm_instance_compare(
    const algorithm_instance_t * instance1,
    const algorithm_instance_t * instance2
) {
    // Both structures need to be of the same type in memory
    return instance1->id - instance2->id;
}

//--------------------------------------------------------------------
// algorithm_instance_t
//--------------------------------------------------------------------

inline void * algorithm_instance_get_options(algorithm_instance_t * instance) {
    return instance ? instance->options : NULL;
}

inline probe_t * algorithm_instance_get_probe_skel(algorithm_instance_t * instance) {
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
        dynarray_clear(instance->events, (ELEMENT_FREE) event_free);
    }
}

inline unsigned int algorithm_instance_get_num_events(algorithm_instance_t * instance) {
    return instance && instance->events ? instance->events->size : 0;
}

void pt_throw(
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
        } else {
            fprintf(stderr, "pt_algorithm_throw: event ignored\n");
        }
    }
}

void pt_stop_instance(
    struct pt_loop_s     * loop,
    algorithm_instance_t * instance
) {
    // Notify the caller that this instance will be freed
    pt_throw(NULL, instance, event_create(ALGORITHM_TERM, NULL, NULL, NULL));

    // This instance is unregistered by the user handler
}

/**
 * \brief Register an algorithm instance in the main loop
 * \param instance The instance that we register
 */

static inline algorithm_instance_t * pt_algorithm_instance_add(
    struct pt_loop_s     * loop,
    algorithm_instance_t * instance
) {
    return tsearch(
        instance,
        &loop->algorithm_instances_root,
        (ELEMENT_COMPARE) algorithm_instance_compare
    );
}

algorithm_instance_t * pt_add_instance(
    struct pt_loop_s * loop,
    const char       * name,
    void             * options,
    probe_t          * probe_skel
) {
    bool                   probe_allocated = false;
    algorithm_t          * algorithm;
    algorithm_instance_t * instance = NULL;

    if (!(algorithm = algorithm_search(name))) {
        goto ERR_ALGORITHM_NOT_FOUND;
    }

    // If the probe skeleton does not exist, create it.
    if (!probe_skel) {
        probe_skel = probe_create();
        probe_allocated = (probe_skel != NULL);
        if (!probe_allocated) goto ERR_PROBE_SKEL;
    }

    // Create a new instance of a running algorithm
    if (!(instance = algorithm_instance_create(loop, algorithm, options, probe_skel))) {
        goto ERR_INSTANCE;
    }

    // We need to queue a new event for the algorithm: it has been started
    pt_throw(NULL, instance, event_create(ALGORITHM_INIT, NULL, NULL, NULL));

    // Add this algorithms to the list of handled algorithms
    pt_algorithm_instance_add(loop, instance);
    return instance;

ERR_INSTANCE:
ERR_PROBE_SKEL:
    if (probe_allocated) probe_free(probe_skel);
ERR_ALGORITHM_NOT_FOUND:
    return NULL;
}

/**
 * \brief Unregister an algorithm instance in the main loop.
 *    Its data must be previously freed by using algorithm_instance_free
 * \param instance The instance that we register
 */

static inline algorithm_instance_t * pt_algorithm_instance_del(
    struct pt_loop_s     * loop,
    algorithm_instance_t * instance
) {
    return tdelete(
        instance,
        &loop->algorithm_instances_root,
        (ELEMENT_COMPARE) algorithm_instance_compare
    );
}

void pt_del_instance(
    struct pt_loop_s * loop,
    algorithm_instance_t * instance
) {
    pt_algorithm_instance_del(loop, instance);
    algorithm_instance_free(instance);
}


