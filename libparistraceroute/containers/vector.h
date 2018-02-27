#ifndef VECTOR_H
#define VECTOR_H

#include <stdbool.h>    // bool
#include <stddef.h>     // size_t
#include "common.h"     // ELEMENT_FREE, ELEMENT_DUMP

/**
 * \file vector.h
 * \brief Header file: vector structure
 *
 * vector_t manages a dynamic vector of dynamic size, by
 * allocating memory in a chunk-by-chunk fashion. An initial memory_size is
 * allocated, and this size is increased by a given amount when needed.
 */

/**
 * \struct vector_t
 * \brief Structure representing a dynamic vector.
 */

typedef struct {
    void    * cells;      /**< Pointer to the elemnts contained in the vector */
    size_t    cell_size;  /**< The size of each cell caontianed in the vectore */
    size_t    num_cells;  /**< number of options containeted in the current vector */
    size_t    max_cells;  /**< maximum number of options contained in the vector */
    void*  (* cells_dup)(const void *);
    void   (* cells_free)(void *);
    void   (* cells_dump)(const void *);
} vector_t;

/**
 * \brief Create a vector structure.
 * \param cell_size Size of a cell contained in the vector.
 * \param callback_free A function used to free the vector.
 * \param callback_dump A function used to dump the vector.
 * \return A vector_t structure representing an empty vector
 */

vector_t * vector_create_impl(size_t cell_size, void * (*callback_dup)(const void *), void (*callback_free)(void *), void (*callback_dump)(const void *));

#define vector_create(s, dup, free, dump) vector_create_impl(s, (ELEMENT_DUP) dup, (ELEMENT_FREE) free, (ELEMENT_DUMP) dump)

/**
  * \biref Dump a vector instance
  * \param vector pointer to a vector structure
  */

void vector_dump(vector_t * vector);

/**
 * \brief Free a vector instance
 * \param vector A vector_t instance
 * \param element_free Pointer to a function used to free up element resources
 *     (can be NULL)
 */

void vector_free(vector_t * vector, void (*element_free)(void * element));

/**
 * \brief Add a new element at the end of the vector.
 * \param vector A vector_t instance
 * \param element Pointer to the element to add
 */

bool vector_push_element(vector_t * vector, void * element);

/**
 * \brief Delete the i-th element stored in a vector_t.
 * The element from i + 1 to the end of the vector are moved one cell before.
 * \param vector A vector instance
 * \param i The index of the element to remove.
 *    Valid values are between 0 and vector_get_number_of_options() - 1
 *  If i is out of range, nothing happens.
 * \returns true iif successful
 */

bool vector_del_ith_element(vector_t * vector, size_t i);

/**
 * \brief Clear a vector.
 * \param vector A vector instance
 * \param element_free Pointer to a function used to free up element resources
 *     (may be NULL)
 */

// TODO this callback should be moved in vector_create
void vector_clear(vector_t * vector, void (*element_free)(void * element));

/**
 * \brief Get the current number of options contained in the vector.
 * \param vector A vector instance
 * \return number of options contained in the vector
 */

size_t vector_get_num_cells(const vector_t * vector);

/**
 * \brief Get all the options inside a vector.
 * \param vector A vector instance
 * \return A pointer to an array of cells.
 */

void * vector_get_cells(const vector_t * vector);

/**
 * \brief Get the size of each cell contained in the vector
 * \param vector A vector instance
 * \return The size of a cell in this vector
 */

size_t vector_get_cell_size(const vector_t * vector);

/**
 * \brief Retrieve the i-th option stored in a vector
 * \param i The index of the option to retrieve.
 *    Valid values are between 0 and vector_get_number_of_options() - 1
 * \return NULL if i-th refers to an element out of range
 *    the address of the i-th element otherwise.
 */

void * vector_get_ith_element(const vector_t * vector, size_t i);


/**
 * @brief Duplicate the vector and the elements (used as a dup function for the container asking for it like map)
 * @param vector the vector to be duplicated
 * @return the new vector with the different values
 */
vector_t * vector_deep_dup(const vector_t* vector);

/**
 * @brief Duplicate the vector and share elements with source (used as a dup function for the container asking for it like map)
 * @param vector the vector to be duplicated
 * @return the new vector with the different values
 */
vector_t * vector_dup(const vector_t* vector);

#endif

