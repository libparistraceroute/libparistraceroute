#ifndef VECTOR_H
#define VECTOR_H

#include "optparse.h"

/**
 * \file vector.h
 * \brief Header file: vector structure
 *
 * vector_t manages a dynamic vector of potentially infinite size, by
 * allocating memory in a chunk-by-chunk fashion. An initial memory_size is
 * allocated, and this size is increased by a given amount when needed.
 *
 */

/**
 * \struct vector_t
 * \brief Structure representing a dynamic vector.
 */

typedef struct {
    struct opt_spec         * options;      /**< the structure of opt_spec */
    unsigned int              num_options;  /**< number of options containeted in the current vector */
    unsigned int              max_options;  /**< maximum number of options contained in the vector */
} vector_t;


/**
 * \brief Create a vector structure.
 * \return A vector_t structure representing an empty vector
 */

vector_t * vector_create();

/**
 * \brief Free a vector structure.
 * \param dynarray Pointer to a vector structure
 * \param element_free Pointer to a function used to free up element resources
 *     (can be NULL)
 */

void vector_free(vector_t * vector , void (* element_free)(struct opt_spec element));

/**
 * \brief Add a new element at the end of the vector.
 * \param dynarray Pointer to a dynamic array structure
 * \param element Pointer to the element to add
 */

void vector_push_element(vector_t * vector, struct opt_spec * element);

/**
 * \brief Delete the i-th element stored in a vector_t.
 * The element from i + 1 to the end of the vector are moved one cell before.
 * \param vector A vecor_t instance
 * \param i The index of the element to remove.
 *    Valid values are between 0 and vector_get_number_of_options() - 1
 *  If i is out of range, nothing happens. 
 */

int vector_del_ith_element(vector_t * vector, unsigned int i);

/**
 * \brief Clear a vector.
 * \param vector Pointer to a vector structure
 * \param element_free Pointer to a function used to free up element resources
 *     (can be NULL)
 */

void vector_clear(vector_t * vector, void (*element_free)(struct opt_spec element));

/**
 * \brief Get the current number of options contained in the vector.
 * \param vector Pointer to a vector structure
 * \return number of options contained in the vector
*/

unsigned vector_get_number_of_options(const vector_t * vector);

/**
 * \brief Get all the options inside a vector.
 * \param vector Pointer to a vector structure
 * \return A pointer to an array of opt_spec structure.
 */

struct opt_spec * vector_get_options(const vector_t * vector);

/**
 * \brief Retrieve the i-th option stored in a vector
 * \param i The index of the option to retrieve.
 *    Valid values are between 0 and vector_get_number_of_options() - 1
 * \return NULL if i-th refers to an element out of range 
 *    the address of the i-th element otherwise.
 */

struct opt_spec * vector_get_ith_element(const vector_t * vector, unsigned int i);



#endif
