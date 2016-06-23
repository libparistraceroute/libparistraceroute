#include "config.h"

#include <stdlib.h> // malloc, free
#include <string.h> // memcpy 
#include <assert.h> // assert
#include "object.h"

object_t * object_create_impl(
    const void * element,
    void * (*element_dup)(const void * element),
    void   (*element_free)(void * element),
    void   (*element_dump)(const void * element),
    int    (*element_compare)(const void * element1, const void * element2)
) {
    object_t * object;

    if (!(object = malloc(sizeof(object_t)))) goto ERR_MALLOC;

    if (element) {
        if (!(object->element = element_dup(element))) goto ERR_ELEMENT_DUP;
    } else object->element = NULL;

    object->dup     = element_dup;
    object->free    = element_free;
    object->dump    = element_dump;
    object->compare = element_compare;
    return object;

ERR_ELEMENT_DUP:
    free(object);
ERR_MALLOC:
    return NULL;
}

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
) {
    return object_create(
        element,
        dummy_element->dup,
        dummy_element->free,
        dummy_element->dump,
        dummy_element->compare
    );
}

object_t * object_dup(const object_t * object) {
    object_t * object_duplicated;

    if (!(object_duplicated = malloc(sizeof(object_t)))) {
        goto ERR_MALLOC;
    }

    memcpy(object_duplicated, object, sizeof(object_t));
    
    if (object->element) {
        if (!(object_duplicated->element = object->dup(object->element))) {
            goto ERR_ELEMENT_DUP;
        }
    }

    return object_duplicated;

ERR_ELEMENT_DUP:
    free(object_duplicated);
ERR_MALLOC:
    return NULL;
}

void object_free(object_t * object) {
    if (object) {
        if (object->free && object->element) object->free(object->element);
        free(object);
    }
}

int object_compare(const object_t * object1, const object_t * object2) {
    assert(object1 && object2 && object1->compare == object2->compare);
    return object1->compare(object1->element, object2->element);
}

void object_dump(const object_t * object) {
    object->dump(object->element);
}


