#ifndef LIBPT_LATTICE_H
#define LIBPT_LATTICE_H

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

//---------------------------------------------------------------------------
// lattice_elt_t 
//---------------------------------------------------------------------------

typedef struct {
    dynarray_t * next;     /**< Successors of this node */
    dynarray_t * siblings; /**< Sibling elements (element having the same depth from the root) */
    void       * data;     /**< Data stored in this node */
} lattice_elt_t;

/**
 * \brief Allocate a lattice_elt_t instance (lattice node).
 * \param data This address is stored in the newly allocated node.
 * \return The newly allocated lattice_elt_t instance if successful,
 *    NULL otherwise.
 */

lattice_elt_t * lattice_elt_create(void * data);

/**
 * \brief Release a lattice_elt_t instance from the memory.
 * \param elt A lattice_elt_t instance.
 */

void lattice_elt_free(lattice_elt_t * elt);

void * lattice_elt_get_data(const lattice_elt_t * elt);

//---------------------------------------------------------------------------
// lattice_t
//---------------------------------------------------------------------------

typedef struct {
    //lattice_elt_t *root;
    dynarray_t * roots;
    int       (* cmp)(const void *, const void *);
} lattice_t;

/**
 * \brief Allocate a lattice_t instance. 
 * \param data This address is stored in the newly allocated node.
 * \return The newly allocated lattice_elt_t instance if successful,
 *    NULL otherwise.
 */

lattice_t * lattice_create();

/**
 * \brief Release a lattice_t instance from the memory.
 * \param lattice A lattice_t instance.
 * \param lattice_element_free This callback is called for each released
 *    lattice_elt_t instance belonging to this lattice.
 */

void lattice_free(lattice_t * lattice, void (*lattice_element_free)(void *element));

/**
 * \brief Retrieve the number of successors of a given lattice node.
 * \param elt A lattice node.
 * \return The number of successors.
 */

size_t lattice_elt_get_num_next(const lattice_elt_t * elt);

/**
 * \brief Retrieve the number of sibling nodes (having the same depth from a root)
 *    of a given lattice node.
 * \param elt A lattice node.
 * \return The number of sibling nodes.
 */

size_t lattice_elt_get_num_siblings(const lattice_elt_t * elt);

//void lattice_set_cmp(lattice_t * lattice, int (*cmp)(const void *, const void *));

lattice_return_t lattice_walk(lattice_t * lattice, lattice_return_t (*visitor)(lattice_elt_t *, void *), void * data, lattice_walk_t walk);

/**
 * \brief Connect two nodes in the lattice.
 * \param u The first node.
 * \param v The second node.
 * \return true iif successful.
 */

bool lattice_connect(lattice_t * lattice, lattice_elt_t * u, lattice_elt_t * v);

/**
 * \brief Add a new node in the lattice.
 * \param predecessor The predecessor of this node in the lattice.
 *    You may pass NULL if there is no predecessor. In this case, the new
 *    nodes is stored in lattice->roots.
 */

bool lattice_add_element(lattice_t * lattice, lattice_elt_t * predecessor, void * data);

/**
 * \brief Dump a lattice_t structure to the standard output.
 * \param lattice A lattice_t instance.
 * \param element_dump A function that print lattice_elt->data. You may
 *    pass NULL if unused.
 */
void lattice_dump(lattice_t * lattice, void (* element_dump)(const void *));

#endif // LIBPT_LATTICE_H
