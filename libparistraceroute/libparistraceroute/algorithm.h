#ifndef ALGORITHM_H
#define ALGORITHM_H

#include <search.h>
#include "probe.h"
#include "event.h"
#include "dynarray.h"
/**
 *  algorithm status
 */
typedef enum {
    STARTED,
    STOPPED,
    ERROR
} status_t;

struct algorithm_s;

typedef enum {
    CALLER_ALGORITHM,
} caller_type_t;

typedef struct {
    caller_type_t type;
    union {
        struct algorithm_instance_s *caller_algorithm;
    };
} caller_t;

typedef struct algorithm_instance_s {
    unsigned int id;
    struct algorithm_s *algorithm;
    void *options;
    probe_t *probe_skel;
    void *data;
    dynarray_t *events;
    caller_t *caller;
    struct pt_loop_s *loop;
} algorithm_instance_t;

typedef struct algorithm_s {
    char* name;
    void (*handler)(struct pt_loop_s *loop, algorithm_instance_t *instance);//, void *options, probe_t *probe_skel, void **data, event_t **events, unsigned int num_events);
} algorithm_t;


/******************************************************************************
 * algorithm_t
 ******************************************************************************/

algorithm_t* algorithm_search(char *name);
void algorithm_register(algorithm_t *algorithm);

#define ALGORITHM_REGISTER(MOD)	\
static void __init_ ## MOD (void) __attribute__ ((constructor));	\
static void __init_ ## MOD (void) {	\
	algorithm_register(&MOD); \
}

/******************************************************************************
 * algorithm_instance_t
 ******************************************************************************/

algorithm_instance_t* algorithm_instance_create(struct pt_loop_s *loop, algorithm_t *algorithm, void *options, probe_t *probe_skel);
void algorithm_instance_free(algorithm_instance_t *instance);

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
