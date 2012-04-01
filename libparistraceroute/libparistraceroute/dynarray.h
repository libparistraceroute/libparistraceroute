#ifndef DYNARRAY_H
#define DYNARRAY_H

/**
 * \file dynarray.h
 * \brief Header file: dynamic array structure
 *
 * dynarray_t manages a dynamic array of potentially infinite size, by
 * allocating memory in a chunk-by-chunk fashion. An initial memory_size is
 * allocated, and this size is increased by a given amount when needed.
 *
 */

/**
 * \struct dynarray_t
 * \brief Structure representing a dynamic array.
 */
typedef struct {
    void **elements;        /*!< Pointer to the array of elements */
    unsigned int size;      /*!< Current size of the array */
    unsigned int max_size;  /*!< Current maximum size of the array */
} dynarray_t;

/**
 * \brief Create a dynamic array structure.
 * \return A dynarray_t structure representing an empty dynamic array
 */
dynarray_t* dynarray_create(void);

/**
 * \brief Free a dynamic array structure.
 * \param dynarray Pointer to a dynamic array structure
 * \param element_free Pointer to a function used to free up element resources
 *     (can be NULL)
 */
void dynarray_free(dynarray_t *dynarray, void (*element_free)(void *element));

/**
 * \brief Add a new element at the end of the dynamic array.
 * \param dynarray Pointer to a dynamic array structure
 * \param element Pointer to the element to add
 */
void dynarray_push_element(dynarray_t *dynarray, void *element);

/**
 * \brief Clear a dynamic array.
 * \param dynarray Pointer to a dynamic array structure
 * \param element_free Pointer to a function used to free up element resources
 *     (can be NULL)
 */
void dynarray_clear(dynarray_t *dynarray, void (*element_free)(void *element));

/**
 * \brief Get the current size of a dynamic array.
 * \param dynarray Pointer to a dynamic array structure
 * \return Current size of the dynamic array
 */
unsigned int dynarray_get_size(dynarray_t *dynarray);

/**
 * \brief Get all the elements inside a dynamic array.
 * \param dynarray Pointer to a dynamic array structure
 * \return An array of pointer to the dynamic array elements
 */
void **dynarray_get_elements(dynarray_t *dynarray);

void *dynarray_get_ith_element(dynarray_t *dynarray, unsigned int i);

#endif
