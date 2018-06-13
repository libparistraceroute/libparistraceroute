#ifndef LIBPT_CONTAINER_PAIR_H
#define LIBPT_CONTAINER_PAIR_H

#include "containers/object.h"

typedef struct pair_s {
    object_t * first;  /**< The first object stored in the pair. */
    object_t * second; /**< The second object stored in the pair. */
} pair_t;

/**
 * \brief Create a pair of object
 * \param first The first object (which is duplicated in the pair).
 * \param second The second object (which is duplicated in the pair).
 * \return The newly allocated pair_t instance if successful,
 *    NULL otherwise.
 */

pair_t * pair_create(const object_t * first, const object_t * second);

/**
 * \brief Create a pair for a given first and second element (which will be
 *    nested in a object_t instance).
 * \param dummy_pair The pair used to initialize callback of the pair
 *    we are creating.
 * \param first The first element.
 * \param second The second element.
 * \return The newly allocated pair_t instance if successful,
 *    NULL otherwise.
 */

pair_t * make_pair_impl(const pair_t * dummy_pair, const void * first, const void * second);

#define make_pair(d, f, s) make_pair_impl(d, (const void *) f, (const void *) s)

/**
 * \brief Release a pair_t instance from the memory.
 * \param pair A pair_t instance.
 */

void pair_free(pair_t * pair);

/**
 * \brief Duplicate a pair_t instance.
 * \param pair A pair_t instance.
 * \return The newly allocated pair_t instance if successful,
 *    NULL otherwise.
 */

pair_t * pair_dup(const pair_t * pair);

/**
 * \brief Compare two pair instance. First we compare the
 *    first element and if this is not sufficient, we
 *    compare the second element.
 * \param pair1 A pair_t instance.
 * \param pair2 A pair_t instance.
 * \return An integer
 *    <  0 if pair1 <  pair2
 *    == 0 if pair1 == pair2
 *    >  0 if pair1 >  pair2
 */

int pair_compare(const pair_t * pair1, const pair_t * pair2);

/**
 * \brief Print a pair_t instance in the standard output.
 * \param pair A pair_t instance.
 */

void pair_dump(const pair_t * pair);

#endif // LIBPT_CONTAINER_PAIR_H
