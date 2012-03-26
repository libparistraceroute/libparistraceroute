#include <stdlib.h>
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

int algorithm_compare(const void *algorithm1, const void *algorithm2)
{
    return strcmp (((const algorithm_t*)algorithm1)->name,
            ((const algorithm_t*)algorithm2)->name);
}

algorithm_t* algorithm_search(char *name)
{
    algorithm_t *algorithm, search;

    if (!name) return NULL;

    search.name = name;
    algorithm = *(algorithm_t**)tfind((const void*)&search, (void* const*)&algorithms_root,
            algorithm_compare);

    return algorithm;
}

void algorithm_register(algorithm_t *algorithm)
{
    /* insert the algorithm in the tree if the keys does not exist yet */
    (void) tsearch((const void *)algorithm, (void **)&algorithms_root,
            algorithm_compare);
}

/******************************************************************************
 * algorithm_instance_t
 ******************************************************************************/

algorithm_instance_t* algorithm_instance_create(pt_loop_t *loop, algorithm_t *algorithm, void *options, probe_t *probe_skel)
{
    algorithm_instance_t *instance;

    instance = (algorithm_instance_t*)malloc(sizeof(algorithm_instance_t));
    instance->id = loop->next_algorithm_id++;
    instance->algorithm = algorithm;
    instance->options = options;
    instance->probe_skel = probe_skel;
    instance->data = NULL;
    instance->events = dynarray_create();
    instance->caller = NULL;
    instance->loop = loop;

    return instance;
}

void algorithm_instance_free(algorithm_instance_t *instance)
{
    // options ?
    if (instance->probe_skel)
        probe_free(instance->probe_skel);
    free(instance);
    instance = NULL;
}

int algorithm_instance_compare(const void *instance1, const void *instance2)
{
    // both structures need to be the same in memory
    return ((algorithm_instance_t*)instance1)->id - ((algorithm_instance_t*)instance2)->id;
}

void* algorithm_instance_get_options(algorithm_instance_t *instance)
{
    return instance->options;
}

void* algorithm_instance_get_probe_skel(algorithm_instance_t *instance)
{
    return instance->probe_skel;
}

void* algorithm_instance_get_data(algorithm_instance_t *instance)
{
    return instance->data;
}

void algorithm_instance_set_data(algorithm_instance_t *instance, void *data)
{
    instance->data = data;
}

event_t** algorithm_instance_get_events(algorithm_instance_t *instance)
{
    return (event_t**)(dynarray_get_elements(instance->events));
}

void algorithm_instance_clear_events(algorithm_instance_t *instance)
{
    dynarray_clear(instance->events, (void (*)(void*))event_free);
}

unsigned int algorithm_instance_get_num_events(algorithm_instance_t *instance) {
    return instance->events->size;
}

void algorithm_instance_add_event(algorithm_instance_t *instance, event_t *event) {
    dynarray_push_element(instance->events, (void*)event);
}

/******************************************************************************
 * pt_loop_t
 ******************************************************************************/

// visitor for twalk
void pt_process_algorithms_instance(const void *node, VISIT visit, int level)
{
    algorithm_instance_t *instance;

    instance = *(algorithm_instance_t**)node;
    instance->algorithm->handler(instance->loop, instance);//, instance->options, instance->probe_skel,
//            &instance->data, 
//            algorithm_instance_get_events(instance),
//            algorithm_instance_get_num_events(instance));
    algorithm_instance_clear_events(instance);
}

algorithm_instance_t * pt_algorithm_instance_add(struct pt_loop_s *loop, algorithm_instance_t *instance)
{
    return tsearch((const void *)instance, (void**)&loop->algorithm_instances_root,
        algorithm_instance_compare);
}

void pt_algorithm_instance_iter(pt_loop_t *loop, void (*action) (const void *, VISIT, int))
{
    twalk((void*)loop->algorithm_instances_root, action);
}

algorithm_instance_t * pt_algorithm_add(struct pt_loop_s *loop, char *name, void *options, probe_t *probe_skel)
{
    algorithm_t *algorithm;
    algorithm_instance_t *instance;

    algorithm = algorithm_search(name);
    if (!algorithm) {
        return NULL;  // No such algorithm
    }
    
    /* Create a new instance of a running algorithm */
    instance = algorithm_instance_create(loop, algorithm, options, probe_skel);

    /* We need to queue a new event for the algorithm: it has been started */
    algorithm_instance_add_event(instance, event_create(ALGORITHM_INIT, NULL));
    // pt_notify_algorithm_fd(loop); /* TODO */

    /* Add this algorithms to the list of handled algorithms */
    pt_algorithm_instance_add(loop, instance);

    return instance;
}

void pt_algorithm_terminate(struct pt_loop_s *loop, algorithm_instance_t *instance)
{
    /* notify the caller */
    if (!instance->caller) {
        // We should callback
        return;
    }
    switch (instance->caller->type)
    {
        case CALLER_ALGORITHM:
            /* Queue an event for the caller : child has terminated */
            algorithm_instance_add_event(instance->caller->caller_algorithm,
                    event_create(ALGORITHM_TERMINATED, NULL));
            break;
        default:
            break;
    }
}

