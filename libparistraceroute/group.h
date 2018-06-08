#ifndef LIBPT_GROUP_H
#define LIBPT_GROUP_H

#include <unistd.h>   // close
#include "dynarray.h" // dynarray_t
#include "probe.h"    // probe_t
/*
typedef enum {
    GROUP_ELT_TYPE_PROBE,
    GROUP_ELT_TYPE_GROUP
} group_elt_type_t;

typedef struct {
    union {
        group_t * group;
        probe_t * probe;
    }
    group_elt_type_t * type;
} group_elt_t;

// A group might receive an identifier, a skeleton (?) to quickly match probes
// inside, etc.
*/
typedef struct {
    dynarray_t * probes;                  /**< Probes related to this group of probes */
    double    (* delay_callback)(void *); /**< Return the interval of time before sending the next probe */
    int          timerfd;                 /**< This timerfd is activated when the next probe must be sent */
} group_t;

group_t * group_create(double (*callback)(void *));

void group_free(group_t * group);

int group_get_timerfd(const group_t * group);
 
probe_t * group_get_probe(const group_t * group, size_t i);

bool group_add_probe(group_t * group, probe_t * probe);

bool group_add_n_probes(group_t * group, probe_t * probes, size_t n);

bool group_set_timer(group_t * group);

#endif // LIBPT_GROUP_H
