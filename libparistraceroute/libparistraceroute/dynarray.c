#include <stdlib.h>
#include <string.h>
#include <stdio.h> //DEBUG
#include "dynarray.h"

#define DYNARRAY_SIZE_INIT 5
#define DYNARRAY_SIZE_INC 5

dynarray_t* dynarray_create(void)
{
    dynarray_t * dynarray = malloc(sizeof(dynarray_t));
    if (dynarray) {
        dynarray->elements = calloc(DYNARRAY_SIZE_INIT, sizeof(void*));
        memset(dynarray->elements, 0, DYNARRAY_SIZE_INIT * sizeof(void*));
        dynarray->size = 0;
        dynarray->max_size = DYNARRAY_SIZE_INIT;
    }
    return dynarray;
}

dynarray_t* dynarray_dup (dynarray_t* dynarray, void* (*element_dup)(void*))
{
    dynarray_t   *da;
    unsigned int  i, size;

    da = dynarray_create();
    if (!da)
        return NULL;

    size = dynarray_get_size(dynarray);
    for (i = 0; i < size; i++) {
        void *element = dynarray_get_ith_element(dynarray, i);
        if (element_dup)
            dynarray_push_element(dynarray, element_dup(element));
        else
            dynarray_push_element(dynarray, element);
    }

    return da;
}

void dynarray_free(dynarray_t * dynarray, void (*element_free)(void *element)) { 
    unsigned int i;

    //printf(">> dynarray_free(@%x): begin\n", dynarray);
    if (dynarray) {
        if (dynarray->elements) {
            if (element_free) {
                //printf(">> dynarray_free(@%x): size = %d\n", dynarray, dynarray->size);
                for(i = 0; i < dynarray->size; i++) {
                    //printf(">>> dynarray_free(%x): freeing [%d / %d] @%d\n", dynarray, i, dynarray->size, dynarray->elements[i]);
                    element_free(dynarray->elements[i]);
                    //printf(">>> dynarray_free(%x): element [%d / %d] freed\n", dynarray, i, dynarray->size);
                }
                //printf(">> dynarray_free(%x): end for\n", dynarray);
            }
            free(dynarray->elements);
        }
        free(dynarray);
    }
    //printf(">> dynarray_free(%x): end\n", dynarray);
}

void dynarray_push_element(dynarray_t *dynarray, void *element)
{
    if (dynarray->size == dynarray->max_size) {
        dynarray->elements = realloc(dynarray->elements, (dynarray->size + DYNARRAY_SIZE_INC) * sizeof(void*));
        memset(dynarray->elements + dynarray->size, 0, DYNARRAY_SIZE_INC * sizeof(void*));
        dynarray->max_size += DYNARRAY_SIZE_INC;
    }
    dynarray->elements[dynarray->size] = element;
    dynarray->size++;
}

int dynarray_del_ith_element(dynarray_t *dynarray, unsigned int i)
{
    if (i >= dynarray->size)
        return 0;
    /* Let's move all elements from the (i+1)-th to the left */
    memmove(dynarray->elements + i, dynarray->elements + (i+1), (dynarray->size-i-1) * sizeof(void*));
    dynarray->size--;
    return 1;
}

void dynarray_clear(dynarray_t *dynarray, void (*element_free)(void *element))
{
    unsigned int i;

    if (!dynarray)
        return;

    for(i = 0; i < dynarray->size; i++) {
        element_free(dynarray->elements[i]);
    }
    dynarray->elements = realloc(dynarray->elements, DYNARRAY_SIZE_INIT * sizeof(void*)); // XXX
    memset(dynarray->elements, 0, DYNARRAY_SIZE_INIT * sizeof(void*));
    dynarray->size = 0;
    dynarray->max_size = DYNARRAY_SIZE_INIT;
}


size_t dynarray_get_size(dynarray_t *dynarray)
{
    return dynarray ? dynarray->size : 0;
}

void **dynarray_get_elements(dynarray_t *dynarray)
{
    return dynarray->elements;
}

void *dynarray_get_ith_element(dynarray_t *dynarray, unsigned int i)
{
    if (i >= dynarray->size)
        return NULL; // out of range
    return dynarray->elements[i];
}

int dynarray_set_ith_element(dynarray_t *dynarray, unsigned int i, void *element)
{
    if (i > dynarray->size)
        return -1; // out of range
    dynarray->elements[i-1] = element;

    return 0;
}
