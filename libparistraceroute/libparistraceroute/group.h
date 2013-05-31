#ifndef GROUP_H
#define GROUP_H

#include "dynarray.h"
#include "probe.h"
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
    dynarray_t * elements;
    double    (* distribution_callback)(void *);
    int          timerfd;
} group_t;

group_t * group_create(double (*callback)(void *));

void group_free(group_t * group);

inline int group_get_timerfd(group_t * group);

probe_t * group_get_probe(group_t * group, size_t i);

bool group_add_probe(group_t * group, probe_t * probe);

bool group_add_n_probes(group_t * group, probe_t * probes, size_t n);

#endif // GROUP_H
