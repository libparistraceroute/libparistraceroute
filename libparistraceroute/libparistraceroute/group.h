#ifndef GROUP_H
#define GROUP_H

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
typedef struct {
    dynarray_t * elements;
} group_t;

#endif // GROUP_H
