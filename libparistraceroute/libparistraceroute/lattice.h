#ifndef STRUCTURE_LATTICE_H
#define STRUCTURE_LATTICE_H

#include "dynarray.h"

typedef enum {
    LATTICE_WALK_DFS,
    LATTICE_WALK_BFS
} lattice_traversal_t;

typedef enum {
    LATTICE_CONTINUE,
    LATTICE_INTERRUPT_NEXT,
    LATTICE_INTERRUPT_ALL,
    LATTICE_ERROR
} lattice_return_t;

typedef struct {
    /* Next hop elements */
    dynarray_t *next;
    /* Sibling elements */
    dynarray_t *siblings;
    /* Local data */
    void *data;
} lattice_elt_t;

typedef struct {
    lattice_elt_t *root;
    void *data;
    int (*cmp)(void*, void*);
} lattice_t;

lattice_elt_t * lattice_elt_create(void *id);
void lattice_elt_free(lattice_elt_t *le);

lattice_t * lattice_create(void);
void lattice_free(lattice_t * lattice, void (*lattice_element_free)(void *element));
int lattice_set_data(lattice_t * lattice, void * data);
void * lattice_get_data(lattice_t * lattice);

int lattice_set_cmp(lattice_t * lattice, int (*cmp)(void *, void *));

int lattice_search(lattice_t *lattice, int (*visitor)(void * local_data, void * data), void * data, lattice_traversal_t traversal);
int lattice_walk(lattice_t *lattice, int (*visitor)(void * local_data, void * data), void * data, lattice_traversal_t traversal);
void * lattice_find(lattice_t * lattice, void * data);
unsigned int lattice_get_num_next(lattice_t * lattice, void * data);
unsigned int lattice_get_num_siblings(lattice_t * lattice, void * data);

int lattice_add_element(lattice_t *lattice, void * prev, void * local_data);

void * lattice_get_data(lattice_t * lattice);
int lattice_set_data(lattice_t * lattice, void * data);

void lattice_dump(lattice_t * lattice, void (*element_dump)(void*));

#endif // STRUCTURE_LATTICE_H
