#include "config.h"

#include <stdlib.h>          // malloc
#include <assert.h>          // assert

#include "containers/map.h"  // map_t
#include "containers/pair.h" // pair_t

static int map_pair_compare(const pair_t * pair1, const pair_t * pair2) {
    assert(pair1 && pair1->first);
    assert(pair2 && pair2->first);
    assert(pair1->first->compare == pair2->first->compare);
    
    return pair1->first->compare(
        pair1->first->element,
        pair2->first->element
    );
}

map_t * map_create_impl(
    void * (*key_dup)(const void * key),
    void   (*key_free)(void * key),
    void   (*key_dump)(const void * key),
    int    (*key_compare)(const void * key1, const void * key2),
    void * (*data_dup)(const void * data),
    void   (*data_free)(void * data),
    void   (*data_dump)(const void * data)
) {
    map_t    * map;
    pair_t   * pair;
    object_t * dummy_key,
             * dummy_data,
             * dummy_pair;

    assert(key_compare);

    // TODO Avoid this useless duplicate/free by improving set_t and pair_t
    if (!(map = malloc(sizeof(map_t)))) {
        goto ERR_MALLOC;
    }

    if (!(dummy_key = object_create(NULL, key_dup,  key_free,  key_dump,  key_compare))) {
        goto ERR_OBJECT_CREATE_KEY;
    }

    if (!(dummy_data = object_create(NULL, data_dup, data_free, data_dump, NULL))) {
        goto ERR_OBJECT_CREATE_DATA;
    }

    if (!(pair = pair_create(dummy_key, dummy_data))) {
        goto ERR_PAIR_CREATE;
    }
    object_free(dummy_key);
    object_free(dummy_data);

    if (!(dummy_pair = object_create(NULL, pair_dup, pair_free, pair_dump, map_pair_compare))) {
        goto ERR_OBJECT_CREATE_PAIR;
    }
    dummy_pair->element = pair;

    if (!(map->set = make_set(dummy_pair))) {
        goto ERR_SET_CREATE;
    }
    object_free(dummy_pair);

    return map;

ERR_SET_CREATE:
    object_free(dummy_pair);
ERR_OBJECT_CREATE_PAIR:
    pair_free(pair);
ERR_PAIR_CREATE:
    object_free(dummy_data);
ERR_OBJECT_CREATE_DATA:
    object_free(dummy_key);
ERR_OBJECT_CREATE_KEY:
    free(map);
ERR_MALLOC:
    return NULL;

}

map_t * make_map(const object_t * dummy_key, const object_t * dummy_data) {
    map_t    * map;
    pair_t   * pair;
    object_t * dummy_pair;

    assert(dummy_key && dummy_key->compare);
    assert(dummy_data);

    if (!(map = malloc(sizeof(map_t))))               goto ERR_MALLOC;
    if (!(pair = pair_create(dummy_key, dummy_data))) goto ERR_PAIR_CREATE;
    if (!(dummy_pair = object_create(NULL, pair_dup, pair_free, pair_dump, map_pair_compare))) goto ERR_OBJECT_CREATE;
    dummy_pair->element = pair;

    // TODO Avoid this useless malloc/free by improving set_t
    if (!(map->set = make_set(dummy_pair))) goto ERR_SET_CREATE;
    object_free(dummy_pair);

    return map;

ERR_SET_CREATE:
    object_free(dummy_pair);
ERR_OBJECT_CREATE:
    pair_free(pair);
ERR_PAIR_CREATE:
    free(map);
ERR_MALLOC:
    return NULL;
}

bool map_update_impl(map_t * map, const void * key, const void * data) {
    pair_t * pair;
    pair_t * pair_in_set;
    void   * swap;

    if (!(pair = make_pair_impl((const pair_t *) map->set->dummy_element->element, key, data))) goto ERR_MAKE_PAIR;
    if (!(set_insert(map->set, pair))) {
        pair_in_set = (pair_t *) set_find(map->set, pair);
        assert(pair_in_set);
        swap = pair_in_set->second;
        pair_in_set->second = pair->second;
        pair->second = swap;
    }
    pair_free(pair);
    return true;

ERR_MAKE_PAIR:
    return false;
}

#include <stdio.h> // DEBUG
bool map_find_impl(const map_t * map, const void * key, const void ** pdata) {
    pair_t       * pair,
                 * search;
    const pair_t * dummy_pair;

    *pdata = NULL;
    dummy_pair = (const pair_t *) ((const object_t *) map->set->dummy_element)->element;

    if (!(search = make_pair_impl(dummy_pair, (const void *) key, NULL))) {
        goto ERR_MAKE_PAIR; 
    }

    if ((pair = set_find(map->set, search))) {
        *pdata = pair->second->element;
    }

    pair_free(search);
    return (pair != NULL);

ERR_MAKE_PAIR:
    return false;
}

void map_free(map_t * map) {
    if (map) {
        if (map->set) set_free(map->set);
        free(map);
    }
}

void map_dump(const map_t * map) {
    set_dump(map->set);
}

