#ifndef LIBPT_CONTAINER_OBJECT_H
#define LIBPT_CONTAINER_OBJECT_H

#include "common.h"

/**
 * object_t allows to embed some useful callback intensively used
 * in set_t and map_t containers.
 */

typedef struct {
    // Methods
    void * (*dup)(const void * element);
    void   (*free)(void * element);
    void   (*dump)(const void * element);
    int    (*compare)(const void * element1, const void * element2);

    void   * element;
} object_t;

object_t * object_create_impl(
    const void * element,
    void * (*element_dup)(const void * element),
    void   (*element_free)(void * element),
    void   (*element_dump)(const void * element),
    int    (*element_compare)(const void * element1, const void * element2)
);

#define object_create(elt, dup, free, dump, compare)  object_create_impl(\
    (const void *) elt, \
    (ELEMENT_DUP) dup, \
    (ELEMENT_FREE) free, \
    (ELEMENT_DUMP) dump, \
    (ELEMENT_COMPARE) compare \
)

object_t * make_object(
    const object_t * dummy_element,
    const void     * element
);

object_t * object_dup(const object_t * object);

void object_free(object_t * object);

int object_compare(const object_t * object1, const object_t * object2);

void object_dump(const object_t * object);

#endif // LIBPT_CONTAINER_OBJECT_H
