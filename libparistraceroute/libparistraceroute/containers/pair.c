#include "config.h"

#include <stdlib.h> // malloc, free
#include <stdio.h>  // printf 
#include <assert.h> // assert

#include "pair.h"
// pair.c

pair_t * pair_create(const object_t * first, const object_t * second) {
    pair_t * pair;

    if (!(pair = malloc(sizeof(pair_t))))     goto ERR_MALLOC;
    if (!(pair->first  = object_dup(first)))  goto ERR_FIRST_DUP;
    if (!(pair->second = object_dup(second))) goto ERR_SECOND_DUP;
    return pair;

ERR_SECOND_DUP:
    if (first->free && first->element) object_free(first->element);
ERR_FIRST_DUP:
    free(pair);
ERR_MALLOC:
    return NULL;
}

pair_t * make_pair_impl(const pair_t * dummy_pair, const void * first, const void * second) {
    pair_t   * pair;

    if (!(pair         = malloc(sizeof(pair_t))))                  goto ERR_MALLOC;
    if (!(pair->first  = make_object(dummy_pair->first,  first)))  goto ERR_FIRST_MAKE_OBJECT;
    if (!(pair->second = make_object(dummy_pair->second, second))) goto ERR_SECOND_MAKE_OBJECT;
    return pair;

ERR_SECOND_MAKE_OBJECT:
    object_free(pair->first);
ERR_FIRST_MAKE_OBJECT:
    free(pair);
ERR_MALLOC:
    return NULL;
}

int pair_compare(const pair_t * pair1, const pair_t * pair2) {
    int ret;

    assert(pair1 && pair1->first && pair1->second);
    assert(pair2 && pair2->first && pair2->second);
    assert(pair1->first->compare  == pair2->first->compare);
    assert(pair1->second->compare == pair2->second->compare);
    
    ret = pair1->first->compare(pair1->first->element, pair2->first->element);
    if (!ret) {
        ret = pair1->second->compare(pair1->second->element, pair2->second->element);
    }
    return ret;
}

pair_t * pair_dup(const pair_t * pair) {
    return pair_create(pair->first, pair->second);
}

void pair_free(pair_t * pair) {
    if (pair) {
        if (pair->first)  object_free(pair->first);
        if (pair->second) object_free(pair->second);
        free(pair);
    }
}

void pair_dump(const pair_t * pair) {
    printf("(");
    object_dump(pair->first);
    printf(", ");
    object_dump(pair->second);
    printf(")");
}
