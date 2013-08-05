#include "config.h"

#define _GNU_SOURCE
#include <search.h> // tsearch, twalk, tdestroy, tdelete
#include <stdlib.h> // malloc, free
#include <stdio.h>  // printf
#include <assert.h> // assert

#include "set.h"    // set_t

static void nothing_to_free() {}

set_t * set_create_impl(
    void * (*element_dup)(const void * element),
    void   (*element_free)(void * element),
    void   (*element_dump)(const void * element),
    int    (*element_compare)(const void * element1, const void * element2)
) {
    set_t * set;

    assert(element_compare);
    assert(element_dup);

    if (!(set = malloc(sizeof(set_t)))) goto ERR_MALLOC;
    if (!(set->dummy_element = object_create(NULL, element_dup, element_free, element_dump, element_compare))) goto ERR_OBJECT_CREATE;
    set->root = NULL;
    return set;

ERR_OBJECT_CREATE:
    free(set);
ERR_MALLOC:
    return NULL;
}

set_t * make_set(const object_t * dummy_element) {
    set_t * set;

    assert(dummy_element);
    assert(dummy_element->compare);
    assert(dummy_element->dup);

    if (!(set = malloc(sizeof(set_t))))                    goto ERR_MALLOC;
    if (!(set->dummy_element = object_dup(dummy_element))) goto ERR_OBJECT_DUP;
    set->root = NULL;
    return set;

ERR_OBJECT_DUP:
    free(set);
ERR_MALLOC:
    return NULL;
}

set_t * set_dup(const set_t * set) {
    fprintf(stderr, "set_dup: not yet implemented");
    return NULL;
}

void set_free(set_t * set) {
    if (set) {
#ifdef _GNU_SOURCE
        tdestroy(set->root, set->dummy_element->free ? set->dummy_element->free : nothing_to_free); 
#else
#    warning set_free cannot call tdestroy()
#endif
        object_free(set->dummy_element);
        free(set);
    }
}

void * set_find(const set_t * set, const void * element) {
    void ** search = tfind(element, &set->root, set->dummy_element->compare);
    return search ? *search : NULL;
}

bool set_insert(set_t * set, const void * element) {
    void * element_dup;
    bool   inserted;

    if (set->dummy_element->dup) {
        if (!(element_dup = set->dummy_element->dup(element))) goto ERR_ELEMENT_DUP;
    } else {
        element_dup = (void *) element;
    }

    inserted = (* (void **) tsearch(element_dup, &set->root, set->dummy_element->compare) == element_dup);

    if (!inserted && set->dummy_element->dup) {
        // This element is already in the tree, remove the duplicate
        set->dummy_element->free(element_dup);
    }

    return inserted;

ERR_ELEMENT_DUP:
    return NULL;
}

bool set_erase(set_t * set, const void * element) {
    void ** search;
    void *  element_to_delete;

    if ((search = tfind(element, &set->root, set->dummy_element->compare))) {
        element_to_delete = *(void **) search;
        search = tdelete(element, &set->root, set->dummy_element->compare);
        set->dummy_element->free(element_to_delete);
    }

    return search != NULL;
}

static const object_t * s_dummy_element;

static void callback_set_dump(const void * node, VISIT visit, int level) {
    const void * element;

    switch (visit) {
        case leaf:      // 1st visit (leaf)
        case postorder: // 3rd visit
            printf(" ");
            element = *((void * const *) node);
            if (s_dummy_element->dump) {
                s_dummy_element->dump(element);
            } else printf("?");
            break;
        case preorder:  // 1st visit (not leaf)
        case endorder:  // 2nd visit
            break;
    }
}

void set_dump(const set_t * set) {
    printf("{");
    s_dummy_element = set->dummy_element;
    twalk(set->root, callback_set_dump);
    printf(" }");
}
