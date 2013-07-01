#include "lattice.h"

#include <stdlib.h>  // malloc, free...
#include <stddef.h>  // size_t
#include <stdbool.h> // bool
#include <stdio.h>   // fprintf

// Lattice elements

lattice_elt_t * lattice_elt_create(void * data)
{
    lattice_elt_t * elt;

    if (!(elt = malloc(sizeof(lattice_elt_t))))    goto ERR_MALLOC;
    if (!(elt->next = dynarray_create()))          goto ERR_DYNARRAY_CREATE;
    if (!(elt->siblings = dynarray_create()))      goto ERR_DYNARRAY_CREATE2;
    if (!dynarray_push_element(elt->siblings, elt)) goto ERR_DYNARRAY_PUSH_ELEMENT;
    elt->data = data;

    return elt;

ERR_DYNARRAY_PUSH_ELEMENT:
    dynarray_free(elt->siblings, NULL);
ERR_DYNARRAY_CREATE2:
    dynarray_free(elt->next, NULL);
ERR_DYNARRAY_CREATE:
    free(elt);
ERR_MALLOC:
    return NULL;
}

void lattice_elt_free(lattice_elt_t * elt) {
    // TODO element_free
    dynarray_free(elt->siblings, NULL);
    dynarray_free(elt->next, NULL);
    free(elt);
}

size_t lattice_elt_get_num_next(const lattice_elt_t * elt) {
    return dynarray_get_size(elt->next);
}

size_t lattice_elt_get_num_siblings(const lattice_elt_t * elt) {
    return dynarray_get_size(elt->siblings);
}

void * lattice_elt_get_data(const lattice_elt_t * elt) {
    return elt->data;
}

// Lattice

lattice_t * lattice_create() {
    lattice_t * lattice;

    if (!(lattice = calloc(1, sizeof(lattice_t)))) goto ERR_MALLOC;
    if (!(lattice->roots = dynarray_create()))      goto ERR_DYNARRAY_CREATE;
      
    return lattice;
ERR_DYNARRAY_CREATE:
    free(lattice);
ERR_MALLOC:
    return NULL;
}

void lattice_free(lattice_t * lattice, void (*lattice_element_free)(void *element))
{
    /* TODO Free elements and data if needed ? */
    //if (lattice_element_free)
    //    lattice_element_free();

    free(lattice);
}

// Accessors

/*
void lattice_set_data(lattice_t * lattice, void * data) {
    lattice->data = data;
}

void * lattice_get_data(const lattice_t * lattice) {
    return lattice->data;
}
*/

void lattice_set_cmp(lattice_t * lattice, int (*cmp)(const void * x, const void * y)) {
    lattice->cmp = cmp;
}

static lattice_return_t lattice_walk_dfs_rec(
    lattice_elt_t *elt, 
    int (*visitor)(lattice_elt_t *, void *), 
    void *data
) {
    lattice_elt_t    * elt_iter;
    unsigned int       i, num_next;
    lattice_return_t   ret;
    bool               done = true;

    // Process the current node...
    ret = visitor(elt, data);
    switch (ret) {
        case LATTICE_DONE:           break;
        case LATTICE_CONTINUE:       break;
        case LATTICE_INTERRUPT_NEXT: return LATTICE_CONTINUE;
        case LATTICE_INTERRUPT_ALL:  return LATTICE_INTERRUPT_ALL;
        default:                     return LATTICE_ERROR;
    }

    // ... then recurse on next ones 
    num_next = dynarray_get_size(elt->next);
    for (i = 0; i < num_next; i++) {
        elt_iter = dynarray_get_ith_element(elt->next, i);
        ret = lattice_walk_dfs_rec(elt_iter, visitor, data);
        switch (ret) {
            case LATTICE_DONE:           break;
            case LATTICE_CONTINUE:       done = false; break;// continue the for
            case LATTICE_INTERRUPT_ALL:  return LATTICE_INTERRUPT_ALL;
            default:                     return LATTICE_ERROR;
        }
    }

    return done ? LATTICE_DONE : LATTICE_CONTINUE;
}

static lattice_return_t lattice_walk_dfs(
    lattice_t * lattice,         
    int      (* visitor)(lattice_elt_t *, void *),
    void      * data
) {
    lattice_elt_t *  root;
    size_t           i, num_roots;
    lattice_return_t ret;
    bool             done = true;
    
    // Process all roots
    num_roots = dynarray_get_size(lattice->roots);
    for (i = 0; i < num_roots; i++) {
        root = dynarray_get_ith_element(lattice->roots, i);
        ret = lattice_walk_dfs_rec(root, visitor, data);
        switch (ret) {
            case LATTICE_DONE:           break;
            case LATTICE_CONTINUE:       done = false; break;// continue the for
            case LATTICE_INTERRUPT_ALL:  return LATTICE_INTERRUPT_ALL;
            default:                     return LATTICE_ERROR;
        }
    }

    return done ? LATTICE_DONE : LATTICE_CONTINUE;
}

lattice_return_t lattice_walk(
    lattice_t      * lattice,
    int           (* visitor)(lattice_elt_t *, void * data),
    void           * data,
    lattice_walk_t   walk
) {
    switch (walk) {
        case LATTICE_WALK_DFS:
            return lattice_walk_dfs(lattice, visitor, data);
        case LATTICE_WALK_BFS:
            fprintf(stderr, "lattice_walk: walk = LATTICE_WALK_BFS not yet implemented\n");
            break;
        default:
            break;
    }
    return LATTICE_ERROR;
}

static lattice_elt_t * lattice_find_elt(lattice_elt_t * elt, void * data, int (*cmp)(const void * elt1, const void * elt2))
{
    lattice_elt_t * elt_iter,
                  * elt_search;
    size_t          i, num_next;

    if ((cmp && (cmp(elt->data, data) == 0)) || (elt->data == data)) {
        return elt;
    }

    num_next = dynarray_get_size(elt->next);
    for (i = 0; i < num_next; i++) {
        elt_iter = dynarray_get_ith_element(elt->next, i);
        if ((elt_search = lattice_find_elt(elt_iter, data, cmp))) {
            return elt_search;
        }
    }
    return NULL;
}

static void * lattice_find(lattice_t * lattice, void * data)
{
    lattice_elt_t * elt_search,
                  * elt_root;
    size_t          i, num_roots;
    
    num_roots = dynarray_get_size(lattice->roots);
    for (i = 0; i < num_roots; i++) {
        elt_root = dynarray_get_ith_element(lattice->roots, i);
        if ((elt_search = lattice_find_elt(elt_root, data, lattice->cmp))) {
            return elt_search->data;
        }
    }

    return NULL;
}

int lattice_add_element(lattice_t * lattice, lattice_elt_t * prev, void * data)
{
    lattice_elt_t * elt;
   
    if (!(elt = lattice_elt_create(data))) return -1; 

    if (!prev) {
        dynarray_push_element(lattice->roots, elt);
        return 0;
    }

    return lattice_connect(lattice, prev, elt);
}

int lattice_connect(lattice_t *lattice, lattice_elt_t * prev, lattice_elt_t * elt)
{
    size_t       i, j, num_next, num_siblings;
    const void * data = lattice_elt_get_data(elt);
    
    // Return if the element already existing in next hops
    num_next = dynarray_get_size(prev->next);
    for (i = 0; i < num_next; i++) {
        lattice_elt_t * elt_iter = dynarray_get_ith_element(prev->next, i);
        const void * local_data = lattice_elt_get_data(elt_iter);
        if ((lattice->cmp && (lattice->cmp(local_data, data) == 0))
        ||  (local_data == data)) {
            return 0;
        }
    }
    
    // We need to update all siblings: the siblings of my child are the children
    // of all my siblings
    // Go though all my siblings, including me
    num_siblings = dynarray_get_size(prev->siblings);
    for (i = 0; i < num_siblings; i++) {
        lattice_elt_t * sibling = dynarray_get_ith_element(prev->siblings, i);

        num_next = dynarray_get_size(sibling->next);
        for (j = 0; j < num_next; j++) {

            // Existing interfaces get a new sibling
            lattice_elt_t * elt_iter = dynarray_get_ith_element(sibling->next, j);
            dynarray_push_element(elt_iter->siblings, elt);

            // Existing interfaces are a new sibling for the new interface
            dynarray_push_element(elt->siblings, elt_iter);
        }
    }

    dynarray_push_element(prev->next, elt);
    return 0;
}

int lattice_element_dump(lattice_elt_t * elt, void * data) {
    void (*dump)(void *) = data;
    dump(elt);
    return LATTICE_CONTINUE;
}

void lattice_dump(lattice_t * lattice, void (*element_dump)(void *)) {
    lattice_walk(lattice, lattice_element_dump, element_dump, LATTICE_WALK_DFS);
}
