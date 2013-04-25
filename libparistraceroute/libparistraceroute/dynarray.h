#ifndef DYNARRAY_H
#define DYNARRAY_H

/**
 * \file dynarray.h
 * \brief Header file: dynamic array structure
 *
 * dynarray_t manages a dynamic array of potentially infinite size, by
 * allocating memory in a chunk-by-chunk fashion. An initial memory_size is
 * allocated, and this size is increased by a given amount when needed.
 */

/**
 * \struct dynarray_t
 * \brief Structure representing a dynamic array.
 */

typedef struct {
<<<<<<< HEAD
    void         ** elements;  /**< Pointer to the array of elements */
    unsigned int    size;      /**< Current size of the array */
    unsigned int    max_size;  /**< Current maximum size of the array */
=======
    void   ** elements;  /**< Pointer to the array of elements */
    size_t    size;      /**< Size of the array (in bytes) (should be always <= to max_size) */
    size_t    max_size;  /**< Size of the allocated buffer (in bytes) */ 
>>>>>>> origin/master
} dynarray_t;

/**
 * \brief Create a dynamic array structure.
 * \return A dynarray_t structure representing an empty dynamic array
 */

<<<<<<< HEAD
dynarray_t * dynarray_create();

/**
 *
 */

dynarray_t * dynarray_dup(dynarray_t* dynarray, void * (*element_dup)(void *));
=======
dynarray_t * dynarray_create(void);

/**
 * \brief Duplicate a dynarray
 * \param dynarray The dynarray to duplicate
 * \param element_dup A function to call when an element stored in the dynarray is duplicated
 *     (can be NULL)
 * \return The address of the newly created array if successfull, NULL otherwise
 */

dynarray_t * dynarray_dup(const dynarray_t * dynarray, void * (*element_dup)(void *));
>>>>>>> origin/master

/**
 * \brief Free a dynamic array structure.
 * \param dynarray Pointer to a dynamic array structure
 * \param element_free Pointer to a function used to free up element resources
 *     (can be NULL)
 */

<<<<<<< HEAD
void dynarray_free(dynarray_t *dynarray , void (*element_free)(void *element));
=======
void dynarray_free(dynarray_t * dynarray , void (*element_free) (void * element));
>>>>>>> origin/master

/**
 * \brief Add a new element at the end of the dynamic array.
 * \param dynarray Pointer to a dynamic array structure
 * \param element Pointer to the element to add
 */
<<<<<<< HEAD

void dynarray_push_element(dynarray_t *dynarray, void *element);

/**
 * \brief Remove the i-th element from a dynarray
 * \param dynarray The dynarray to update
 * \param i The index of the element to remove
 * \return ...
 */

int dynarray_del_ith_element(dynarray_t *dynarray, unsigned int i);
=======

void dynarray_push_element(dynarray_t * dynarray, void * element);

/**
 * \brief Delete the i-th element stored in a dynarray_t.
 * The element from i + 1 to the end of the array are moved one cell before.
 * \param dynarray A dynarray_t instance
 * \param i The index of the element to remove.
 *    Valid values are between 0 and dynarray_get_size() - 1
 *  If i is out of range, nothing happens. 
 */

int dynarray_del_ith_element(dynarray_t * dynarray, unsigned int i);
>>>>>>> origin/master

/**
 * \brief Clear a dynamic array.
 * \param dynarray Pointer to a dynamic array structure
 * \param element_free Pointer to a function used to free up element resources
 *     (can be NULL)
 */

<<<<<<< HEAD
void dynarray_clear(dynarray_t *dynarray, void (*element_free)(void *element));
=======
void dynarray_clear(dynarray_t * dynarray, void (*element_free)(void * element));
>>>>>>> origin/master

/**
 * \brief Get the current size of a dynamic array.
 * \param dynarray Pointer to a dynamic array structure
 * \return Current size of the dynamic array
 */

<<<<<<< HEAD
size_t dynarray_get_size(dynarray_t *dynarray);
=======
size_t dynarray_get_size(const dynarray_t * dynarray);
>>>>>>> origin/master

/**
 * \brief Get all the elements inside a dynamic array.
 * \param dynarray Pointer to a dynamic array structure
 * \return An array of pointer to the dynamic array elements
 */

<<<<<<< HEAD
void ** dynarray_get_elements(dynarray_t *dynarray);

/**
 * \brief Retrieve the i-th element from a dynarray
 * \param dynarray The dynarray to query 
 * \param i The index of the element to retrieve 
 * \return ...
 */

void * dynarray_get_ith_element(dynarray_t *dynarray, unsigned int i);
=======
void ** dynarray_get_elements(dynarray_t * dynarray);

/**
 * \brief Retrieve the i-th element stored in a dynarray
 * \param i The index of the element to remove.
 *    Valid values are between 0 and dynarray_get_size() - 1
 * \return NULL if i-th refers to an element out of range 
 *    the address of the i-th element otherwise.
 */

void * dynarray_get_ith_element(const dynarray_t * dynarray, unsigned int i);
>>>>>>> origin/master

#endif
