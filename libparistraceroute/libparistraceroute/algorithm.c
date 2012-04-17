#include <stdlib.h>
#include <stdio.h> // XXX
#include <string.h>
#include <search.h>

#include "algorithm.h"
#include "dynarray.h"
#include "event.h"
#include "pt_loop.h"

/* static ? */
void *algorithms_root;

/******************************************************************************
 * algorithm_t
 ******************************************************************************/

int algorithm_compare(const void * algorithm1, const void * algorithm2)
{
    return strcmp(
        ((const algorithm_t *) algorithm1)->name,
        ((const algorithm_t *) algorithm2)->name
    );
}

algorithm_t* algorithm_search(char * name)
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

/******************************************************************************
 * algorithm_instance_t
 ******************************************************************************/

algorithm_instance_t * algorithm_instance_create(
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

void algorithm_instance_free(algorithm_instance_t * instance)
{
    // free options ?
    if (instance) {
        /*
        if (instance->probe_skel) {
            printf("> algorithm_instance_free: call probe_free\n");
            probe_free(instance->probe_skel);
        }
        */
        dynarray_free(instance->events, (ELEMENT_FREE) event_free);
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

inline void * algorithm_instance_get_options(algorithm_instance_t *instance) {
    return instance ? instance->options : NULL;
}

inline probe_t * algorithm_instance_get_probe_skel(algorithm_instance_t *instance) {
    return instance ? instance->probe_skel : NULL;
}

inline void * algorithm_instance_get_data(algorithm_instance_t * instance)
{
    return instance ? instance->data : NULL;
}

inline void algorithm_instance_set_data(algorithm_instance_t * instance, void * data)
{
    if (instance) instance->data = data;
}

inline event_t ** algorithm_instance_get_events(algorithm_instance_t * instance)
{
    return instance ?
        (event_t**) dynarray_get_elements(instance->events) :
        NULL;
}

inline void algorithm_instance_clear_events(algorithm_instance_t * instance)
{
    if (instance) {
        dynarray_clear(instance->events, (void (*)(void*))event_free);
    }
}

inline unsigned int algorithm_instance_get_num_events(algorithm_instance_t * instance)
{
    return instance && instance->events ? instance->events->size : 0;
}

/******************************************************************************
 * pt_loop_t
 ******************************************************************************/

// Visitor for twalk
void pt_process_algorithms_instance(const void * node, VISIT visit, int level)
{
    algorithm_instance_t * instance = *((algorithm_instance_t **) node);
    
    // Save temporarily this algorithm context
    instance->loop->cur_instance = instance;

    // Execute algorithm handler
    instance->algorithm->handler(instance->loop, instance);

    // Restore the algorithm context
    instance->loop->cur_instance = NULL;

    // Flush events queue
    algorithm_instance_clear_events(instance);
}

inline algorithm_instance_t * pt_algorithm_instance_add(
    struct pt_loop_s     * loop,
    algorithm_instance_t * instance
) {
    return tsearch(
        instance,
        &loop->algorithm_instances_root,
        (int (*)(const void *, const void *)) algorithm_instance_compare
    );
}

inline void pt_algorithm_instance_iter(
    pt_loop_t * loop,
    void     (* action) (const void *, VISIT, int))
{
    twalk(loop->algorithm_instances_root, action);
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

// Notify the called algorithm that it can start
algorithm_instance_t * pt_algorithm_add(
    struct pt_loop_s * loop,
    char             * name,
    void             * options,
    probe_t          * probe_skel
) {
    algorithm_t          * algorithm;
    algorithm_instance_t * instance;

    if(! (algorithm = algorithm_search(name)) ) {
        return NULL;  // No such algorithm
    }
    
    // Create a new instance of a running algorithm
    instance = algorithm_instance_create(loop, algorithm, options, probe_skel);

    // We need to queue a new event for the algorithm: it has been started
    pt_algorithm_throw(NULL, instance, event_create(ALGORITHM_INIT, NULL));

    // Add this algorithms to the list of handled algorithms
    pt_algorithm_instance_add(loop, instance);

    return instance;
}

// Notify the caller that the current algorithm has ended
inline void pt_algorithm_terminate(
    struct pt_loop_s     * loop,
    algorithm_instance_t * instance
) {
    printf("pt_algorithm_terminate: Creating ALGORITHM_TERMINATED\n");
    pt_algorithm_throw(
        instance->caller ? NULL : loop,
        instance->caller,
        event_create(ALGORITHM_TERMINATED, NULL)
    );
}

// Notify the called algorithm that it can free its data
inline void pt_algorithm_free(algorithm_instance_t * instance) {
    pt_algorithm_throw(NULL, instance, event_create(ALGORITHM_FREE, NULL));
}

