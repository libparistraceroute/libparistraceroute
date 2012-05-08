#ifndef STRUCTURE_LATTICE_H
#define STRUCTURE_LATTICE_H

#include "dynarray.h"

typedef enum {
    LATTICE_WALK_DFS,
    LATTICE_WALK_BFS
} lattice_traversal_t;

typedef enum {
    LATTICE_DONE,
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
    //lattice_elt_t *root;
    dynarray_t * roots;
    void *data;
    int (*cmp)(void*, void*);
} lattice_t;

lattice_elt_t * lattice_elt_create(void *id);
void lattice_elt_free(lattice_elt_t *le);

void * lattice_elt_get_data(lattice_elt_t * elt);
unsigned int lattice_elt_get_num_next(lattice_elt_t * elt);
unsigned int lattice_elt_get_num_siblings(lattice_elt_t * elt);

lattice_t * lattice_create(void);
void lattice_free(lattice_t * lattice, void (*lattice_element_free)(void *element));
int lattice_set_data(lattice_t * lattice, void * data);
void * lattice_get_data(lattice_t * lattice);

int lattice_set_cmp(lattice_t * lattice, int (*cmp)(void *, void *));

int lattice_walk(lattice_t *lattice, int (*visitor)(lattice_elt_t *, void *), void * data, lattice_traversal_t traversal);
void * lattice_find(lattice_t * lattice, void * data);

int lattice_connect(lattice_t * lattice, lattice_elt_t * prev, lattice_elt_t * elt);
int lattice_add_element(lattice_t *lattice, lattice_elt_t * prev, void * data);

void lattice_dump(lattice_t * lattice, void (*element_dump)(void*));

#endif // STRUCTURE_LATTICE_H
