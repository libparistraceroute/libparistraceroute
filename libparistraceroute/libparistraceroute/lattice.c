#include <stdlib.h>
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

// Lattice

lattice_t * lattice_create(void)
{
    lattice_t *lattice;

    lattice = malloc(sizeof(lattice_t));
    if (!lattice) goto error;

    lattice->root = NULL;
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

int lattice_set_cmp(lattice_t * lattice, int (*cmp)(void * data1, void * data2))
{
    lattice->cmp = cmp;
    return 0;
}

int lattice_walk_dfs(
        lattice_elt_t *elt, 
        int (*visitor)(void * local_data, void * data), 
        void *data)
{
    lattice_elt_t * elt_iter;
    unsigned int    i, num_next;
    int             ret;

    /* Process the current interface... */
    ret = visitor(elt->data, data);
    switch (ret) {
        case LATTICE_CONTINUE:       break;
        case LATTICE_INTERRUPT_NEXT: return LATTICE_CONTINUE;
        case LATTICE_INTERRUPT_ALL:  return LATTICE_INTERRUPT_ALL;
        default:                     return LATTICE_ERROR;
    }

    /* ... then recurse on next ones */
    num_next = dynarray_get_size(elt->next);
    for (i = 0; i < num_next; i++) {
        elt_iter = dynarray_get_ith_element(elt->next, i);
        ret = lattice_walk_dfs(elt_iter, visitor, data);
        switch (ret) {
            case LATTICE_CONTINUE:       break;// continue the for
            case LATTICE_INTERRUPT_ALL:  return LATTICE_INTERRUPT_ALL;
            default:                     return LATTICE_ERROR;
        }
    }

    return LATTICE_CONTINUE;

}

int lattice_walk(lattice_t *lattice, int (*visitor)(void * local_data, void * data), void * data, lattice_traversal_t traversal)
{
    switch (traversal) {
        case LATTICE_WALK_DFS:
            return lattice_walk_dfs(lattice->root, visitor, data);
        case LATTICE_WALK_BFS:
            return -3; // not implemented
        default:
            return -2; // wrong parameter
    }
}

lattice_elt_t * lattice_find_elt(lattice_elt_t * elt, void * data, int (*cmp)(void *elt1, void *elt2))
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
    lattice_elt_t * elt;
    
    elt = lattice_find_elt(lattice->root, data, lattice->cmp);
    return elt ? elt->data : NULL;
}


int lattice_add_element(lattice_t *lattice, void * prev, void * local_data)
{
    unsigned int i, j, num_next, num_siblings;
    lattice_elt_t * elt, * elt_prev;

    if (!prev) {
        if (lattice->root)
            return -1;
        lattice->root = lattice_elt_create(local_data);
        return 0;
    }
    
    elt_prev = lattice_find_elt(lattice->root, prev, lattice->cmp);
    if (!elt_prev)
        return -1; // prev not found

    // Return if the element already existing in next hops
    num_next = dynarray_get_size(elt_prev->next);
    for (i = 0; i < num_next; i++) {
        lattice_elt_t * elt_iter = dynarray_get_ith_element(elt_prev->next, i);
        if ((lattice->cmp && (lattice->cmp(elt_iter->data, local_data) == 0)) ||
            (elt_iter->data == local_data))
            return 0;
    }
    
    // Create the element, update siblings then add it to next hops
    elt = lattice_elt_create(local_data);

    /* We need to update all siblings: the siblings of my child are the children
     * of all my siblings */
    /* Go though all my siblings, including me */
    num_siblings = dynarray_get_size(elt_prev->siblings);
    for (i = 0; i < num_siblings; i++) {
        lattice_elt_t * sibling = dynarray_get_ith_element(elt_prev->siblings, i);

        num_next = dynarray_get_size(sibling->next);
        for (j = 0; j < num_next; j++) {

            /* Existing interfaces get a new sibling */
            lattice_elt_t * elt_iter = dynarray_get_ith_element(sibling->next, j);
            dynarray_push_element(elt_iter->siblings, elt);

            /* Existing interfaces are a new sibling for the new interface */
            dynarray_push_element(elt->siblings, elt_iter);
        }
    }

    dynarray_push_element(elt_prev->next, elt);
    return 0;
}

unsigned int lattice_get_num_next(lattice_t * lattice, void * data)
{
    lattice_elt_t * elt;

    elt = lattice_find_elt(lattice->root, data, lattice->cmp);
    if (!elt) return -1;

    return dynarray_get_size(elt->next);
}

unsigned int lattice_get_num_siblings(lattice_t * lattice, void * data)
{
    lattice_elt_t * elt;

    elt = lattice_find_elt(lattice->root, data, lattice->cmp);
    if (!elt) return -1;

    return dynarray_get_size(elt->siblings);
}

int lattice_element_dump(void * local_data, void * data)
{
    void (*dump)(void*) = data;
    /*dump function*/
    dump(local_data);
    return LATTICE_CONTINUE;
}

void lattice_dump(lattice_t * lattice, void (*element_dump)(void*))
{
    lattice_walk(lattice, lattice_element_dump, element_dump, LATTICE_WALK_DFS);
}
