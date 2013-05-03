#include <stdlib.h>
#include <stdbool.h>
#include "lattice.h"

// Lattice elements

lattice_elt_t * lattice_elt_create(void * data)
{
    lattice_elt_t *le;

    le = malloc(sizeof(lattice_elt_t));
    if (!le) goto error;

    le->next = dynarray_create();
    if (!le->next) goto err_next;
    
    le->siblings = dynarray_create();
    if (!le->siblings) goto err_siblings;

    dynarray_push_element(le->siblings, le);

    le->data = data;

    return le;

err_siblings:
    dynarray_free(le->next, NULL);     /* XXX */
err_next:
    free(le);
error:
    return NULL;
}

void lattice_elt_free(lattice_elt_t *le)
{
    dynarray_free(le->siblings, NULL); /* XXX */
    dynarray_free(le->next, NULL);     /* XXX */
    /* id */
    free(le);
}

size_t lattice_elt_get_num_next(const lattice_elt_t * elt)
{
    return dynarray_get_size(elt->next);
}

size_t lattice_elt_get_num_siblings(const lattice_elt_t * elt)
{
    return dynarray_get_size(elt->siblings);
}

void * lattice_elt_get_data(const lattice_elt_t * elt)
{
    return elt->data;
}

// Lattice

lattice_t * lattice_create(void)
{
    lattice_t *lattice;

    lattice = malloc(sizeof(lattice_t));
    if (!lattice) goto error;

    //lattice->root = NULL;
    lattice->roots = dynarray_create();
    lattice->data = NULL;
    lattice->cmp  = NULL; /* by default, we compare pointer data */

    return lattice;
error:
    return NULL;
}

void lattice_free(lattice_t * lattice, void (*lattice_element_free)(void *element))
{
    /* TODO Free elements and data if neeeded ? */
    //if (lattice_element_free)
    //    lattice_element_free();

    free(lattice);
}

// Accessors

int lattice_set_data(lattice_t * lattice, void * data)
{
    lattice->data = data;
    return 0;
}

void * lattice_get_data(lattice_t * lattice)
{
    return lattice->data;
}

int lattice_set_cmp(lattice_t * lattice, int (*cmp)(const void * data1, const void * data2))
{
    lattice->cmp = cmp;
    return 0;
}

int lattice_walk_dfs_rec(
        lattice_elt_t *elt, 
        int (*visitor)(lattice_elt_t *, void *), 
        void *data)
{
    lattice_elt_t * elt_iter;
    unsigned int    i, num_next;
    int             ret;
    bool            done = true;

    /* Process the current interface... */
    ret = visitor(elt, data);
    switch (ret) {
        case LATTICE_DONE:           break;
        case LATTICE_CONTINUE:       break;
        case LATTICE_INTERRUPT_NEXT: return LATTICE_CONTINUE;
        case LATTICE_INTERRUPT_ALL:  return LATTICE_INTERRUPT_ALL;
        default:                     return LATTICE_ERROR;
    }

    /* ... then recurse on next ones */
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

int lattice_walk_dfs(
        lattice_t * lattice,         
        int (*visitor)(lattice_elt_t *, void *),
        void *data)
{
    lattice_elt_t * root;
    unsigned int    i, num_roots;
    int             ret;
    bool            done = true;
    
    /* Process all roots */
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


int lattice_walk(lattice_t *lattice, int (*visitor)(lattice_elt_t *, void * data), void * data, lattice_traversal_t traversal)
{
    switch (traversal) {
        case LATTICE_WALK_DFS:
            return lattice_walk_dfs(lattice, visitor, data);
        case LATTICE_WALK_BFS:
            return -3; // not implemented
        default:
            return -2; // wrong parameter
    }
}

lattice_elt_t * lattice_find_elt(lattice_elt_t * elt, void * data, int (*cmp)(const void * elt1, const void * elt2))
{
    unsigned int i, num_next;

    if ((cmp && (cmp(elt->data, data) == 0)) || (elt->data == data))
        return elt;

    num_next = dynarray_get_size(elt->next);
    for (i = 0; i < num_next; i++) {
        lattice_elt_t *elt_iter, *elt_search;
        elt_iter = dynarray_get_ith_element(elt->next, i);
        elt_search = lattice_find_elt(elt_iter, data, cmp);
        if (elt_search)
            return elt_search;
    }
    return NULL;
}

void * lattice_find(lattice_t * lattice, void * data)
{
    lattice_elt_t * elt,
                  * root;
    size_t          i, num_roots;
    
    /* Process all roots */
    num_roots = dynarray_get_size(lattice->roots);
    for (i = 0; i < num_roots; i++) {
        root = dynarray_get_ith_element(lattice->roots, i);
        elt = lattice_find_elt(root, data, lattice->cmp);
        if (elt) return elt->data;
    }

    return NULL;
}

int lattice_add_element(lattice_t * lattice, lattice_elt_t * prev, void * data)
{
    lattice_elt_t * elt = lattice_elt_create(data);

    if (!prev) {
        dynarray_push_element(lattice->roots, elt);
        return 0;
    }

    return lattice_connect(lattice, prev, elt);
}

int lattice_connect(lattice_t *lattice, lattice_elt_t * prev, lattice_elt_t * elt)
{
    unsigned int i, j, num_next, num_siblings;
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
    
    /* We need to update all siblings: the siblings of my child are the children
     * of all my siblings */
    /* Go though all my siblings, including me */
    num_siblings = dynarray_get_size(prev->siblings);
    for (i = 0; i < num_siblings; i++) {
        lattice_elt_t * sibling = dynarray_get_ith_element(prev->siblings, i);

        num_next = dynarray_get_size(sibling->next);
        for (j = 0; j < num_next; j++) {

            /* Existing interfaces get a new sibling */
            lattice_elt_t * elt_iter = dynarray_get_ith_element(sibling->next, j);
            dynarray_push_element(elt_iter->siblings, elt);

            /* Existing interfaces are a new sibling for the new interface */
            dynarray_push_element(elt->siblings, elt_iter);
        }
    }

    dynarray_push_element(prev->next, elt);
    return 0;
}

int lattice_element_dump(lattice_elt_t * elt, void * data)
{
    void (*dump)(void*) = data;
    /*dump function*/
    dump(elt);
    return LATTICE_CONTINUE;
}

void lattice_dump(lattice_t * lattice, void (*element_dump)(void*))
{
    lattice_walk(lattice, lattice_element_dump, element_dump, LATTICE_WALK_DFS);
}
