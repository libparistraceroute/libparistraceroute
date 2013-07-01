#ifndef STRUCTURE_LATTICE_H
#define STRUCTURE_LATTICE_H

#include "dynarray.h"

typedef enum {
    LATTICE_DONE,
    LATTICE_CONTINUE,
    LATTICE_INTERRUPT_NEXT,
    LATTICE_INTERRUPT_ALL,
    LATTICE_ERROR
} lattice_return_t;

typedef enum {
    LATTICE_WALK_DFS,
    LATTICE_WALK_BFS
} lattice_walk_t;

// lattice_elt_t 

typedef struct {
    dynarray_t * next;     /**< Next hop elements */
    dynarray_t * siblings; /**< Sibling elements */
    void       * data;     /**< Local data */
} lattice_elt_t;

lattice_elt_t * lattice_elt_create(void * id);
void lattice_elt_free(lattice_elt_t * le);

// lattice_t

typedef struct {
    //lattice_elt_t *root;
    dynarray_t * roots;
//    void       * data;
    int       (* cmp)(const void *, const void *);
} lattice_t;

lattice_t * lattice_create();
void lattice_free(lattice_t * lattice, void (*lattice_element_free)(void *element));

void * lattice_elt_get_data(const lattice_elt_t * elt);
size_t lattice_elt_get_num_next(const lattice_elt_t * elt);
size_t lattice_elt_get_num_siblings(const lattice_elt_t * elt);

void lattice_set_data(lattice_t * lattice, void * data);
void lattice_set_cmp(lattice_t * lattice, int (*cmp)(const void *, const void *));

// TODO should return bool
lattice_return_t lattice_walk(lattice_t *lattice, int (*visitor)(lattice_elt_t *, void *), void * data, lattice_walk_t walk);

// TODO should return bool
int lattice_connect(lattice_t * lattice, lattice_elt_t * prev, lattice_elt_t * elt);

// TODO should return bool
int lattice_add_element(lattice_t *lattice, lattice_elt_t * prev, void * data);

void lattice_dump(lattice_t * lattice, void (*element_dump)(void*));

#endif // STRUCTURE_LATTICE_H
