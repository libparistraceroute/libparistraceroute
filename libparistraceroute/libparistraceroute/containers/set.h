#ifndef SET_H
#define SET_H

#include <stdbool.h>
#include "containers/object.h"

typedef struct {
    void     * root;          /**< tree of element   */
    object_t * dummy_element; /**< object_t<element> */
} set_t;

/**
 * \brief Create a set of element.
 * \param element_dup Callback used to duplicate element (may be set to NULL).
 *    If NULL, the set only stores references to the inserted objects without
 *    duplicating them.
 * \param element_free Callback used to free element (may be set to NULL).
 *    If NULL, the set does not release references that it contains.
 * \param element_dump Callback used to dump element (may be set to NULL).
 *    If NULL, the set cannot be printed using set_dump. 
 * \param element_compare Callback used to compare elements (mandatory).
 */

set_t * set_create_impl(
    void * (*element_dup)(const void * element),
    void   (*element_free)(void * element),
    void   (*element_dump)(const void * element),
    int    (*element_compare)(const void * element1, const void * element2)
);

#define set_create(dup, free, dump, compare) set_create_impl(\
    (ELEMENT_DUP) dup, \
    (ELEMENT_FREE) free, \
    (ELEMENT_DUMP) dump, \
    (ELEMENT_COMPARE) compare \
)

/**
 * \brief Create a set of element.
 * \param object
 */

set_t * make_set(const object_t * object);

/**
 * \brief Duplicate a set_t instance.
 * \param set A set_t instance.
 * \return The newly allocated set_t instance, NULL otherwise.
 */

set_t * set_dup(const set_t * set);

/**
 * \brief Release a set_t instance from the memory.
 * \param set A set_t instance.
 */

void set_free(set_t * set);

/**
 * \brief Print a set_t instance in the standard output.
 * \param set A set_t instance.
 * \warning This function uses a static variable and is not thread-safe.
 */

void set_dump(const set_t * set);

/**
 * \brief Search an element stored in a set_t instance.
 * \param set A set_t instance.
 * \param element The element we're looking for.
 */

void * set_find(const set_t * set, const void * element);

/**
 * \brief Insert a copy of an element in a set_t instance
 *    if this element does not already belongs to the set.
 * \param set A set_t instance.
 * \param element The element that must be added in the set.
 * \return true if successfully inserted. If the element is
 *   already in the set, it returns false.
 */

bool set_insert(set_t * set, const void * element);

/**
 * \brief Erase an element from the set.
 * \param element The element we want to remove.
 * \return true if an element has been successfully removed.
 */

bool set_erase(set_t * set, const void * element);

#endif

