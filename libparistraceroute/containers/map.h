#ifndef LIBPT_CONTAINER_MAP_H
#define LIBPT_CONTAINER_MAP_H

#include <stdbool.h>
#include "containers/set.h" // set_t

/**
 * map_t allows to store for each key a given data.
 * Two keys are said to be equal if they verify key1 <= key2 and key2 <= key1,
 * where <= corresponds to the key_compare callback (see map_create_impl).
 *
 * TODO: This implementation is not very efficient.
 * So far, a map_t instance manages a set of pair<object<key>, object<data> >.
 * It would be nice to manage a set of pair<key, value> to reduce the
 * memory consumption.
 */

typedef struct {
    set_t * set; /**< The key-value pairs stored in the map */
} map_t;

/**
 * \brief Create a map of key-value pairs. Some callbacks may be set to
 *    NULL (see set_create_impl behaviour). 
 * \param key_dup Callback used to duplicate key (may be set to NULL).
 * \param key_free Callback used to free key (may be set to NULL).
 * \param key_dump Callback used to dump key (may be set to NULL).
 * \param key_compare Callback used to compare keys (mandatory).
 * \param data_dup Callback used to duplicate value (may be set to NULL).
 * \param data_free Callback used to free value (may be set to NULL).
 * \param data_dump Callback used to dump value (may be set to NULL).
 */

map_t * map_create_impl(
    void * (*key_dup)(const void * key),
    void   (*key_free)(void * key),
    void   (*key_dump)(const void * key),
    int    (*key_compare)(const void * key1, const void * key2),
    void * (*data_dup)(const void * data),
    void   (*data_free)(void * data),
    void   (*data_dump)(const void * data)
);

#define map_create(                             \
    key_dup,  key_free,  key_dump,  key_compare,\
    data_dup, data_free, data_dump              \
) map_create_impl(                              \
    (ELEMENT_DUP)     key_dup,                  \
    (ELEMENT_FREE)    key_free,                 \
    (ELEMENT_DUMP)    key_dump,                 \
    (ELEMENT_COMPARE) key_compare,              \
    (ELEMENT_DUP)     data_dup,                 \
    (ELEMENT_FREE)    data_free,                \
    (ELEMENT_DUMP)    data_dump                 \
)

/**
 * \brief Create a map_t instance.
 * \param dummy_key This object instance carrying the callbacks used by
 *    the map_t instance to manage its keys.
 * \param dummy_data This object instance carrying the callbacks used by
 *    the map_t instance to manage the values attached to each keys. 
 * \return The newly allocated map_t instance if successful, NULL otherwise.
 */

map_t * make_map(const object_t * dummy_key, const object_t * dummy_data);

/**
 * \brief Release a map_t instance from the memory.
 * \param map A map_t instance.
 */

void map_free(map_t * map);

/**
 * \brief Print a map_t instance in the standard output.
 * \param set A map_t instance.
 * \warning This function uses a static variable and is not thread-safe.
 */

void map_dump(const map_t * map);

/**
 * \brief Insert a new key-value pair in the map. If the key already
 *    exists in the map, its corresponding data is updating usnig "data".
 * \param map A map_t instance.
 * \param key The key.
 * \param data The data attached to the key.
 * \return true iif successful.
 */

bool map_update_impl(map_t * map, const void * key, const void * data);

#define map_update(map, key, data) map_update_impl(map, (const void *) key, (const void *) data)

/**
 * \brief Search a key in the map in order to retrieve the
 *    corresponding value.
 * \param map A map_t instance.
 * \param key The key we're seeking.
 * \param pdata Pass NULL. *pdata will store the address data
 *    related to this key (if found).
 * \return true if the key has been found, false otherwise
 */

bool map_find_impl(const map_t * map, const void * key, const void ** pdata);

#define map_find(map, key, pdata) map_find_impl(map, (const void *) key, (const void **) pdata)

#endif // LIBPT_CONTAINER_MAP_H
