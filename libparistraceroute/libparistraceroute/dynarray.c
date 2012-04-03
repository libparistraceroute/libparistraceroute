#include <stdlib.h>
#include <string.h>
#include "dynarray.h"

#define DYNARRAY_SIZE_INIT 5
#define DYNARRAY_SIZE_INC 5

dynarray_t* dynarray_create(void)
{
    dynarray_t *dynarray;

    dynarray = malloc(sizeof(dynarray_t));
    dynarray->elements = calloc(DYNARRAY_SIZE_INIT, sizeof(void*));
    memset(dynarray->elements, 0, DYNARRAY_SIZE_INIT);
    dynarray->size = 0;
    dynarray->max_size = DYNARRAY_SIZE_INIT;
    return dynarray;
}

void dynarray_free(dynarray_t *dynarray, void (*element_free)(void *element))
{
    unsigned int i;
    if (element_free) {
        for(i = 0; i < dynarray->size; i++)
            element_free(dynarray->elements[i]);
    }
    free(dynarray->elements);
    free(dynarray);
    dynarray = NULL;
}

void dynarray_push_element(dynarray_t *dynarray, void *element)
{
    if (dynarray->size == dynarray->max_size) {
        dynarray->elements = realloc(dynarray->elements, dynarray->size + DYNARRAY_SIZE_INC);
        memset(dynarray->elements + dynarray->size, 0, DYNARRAY_SIZE_INC);
        dynarray->max_size += DYNARRAY_SIZE_INC;
    }
    dynarray->elements[dynarray->size] = element;
    dynarray->size++;
}

void dynarray_clear(dynarray_t *dynarray, void (*element_free)(void *element))
{
    unsigned int i;
    for(i = 0; i < dynarray->size; i++) {
        element_free(dynarray->elements[i]);
    }
    dynarray->elements = realloc(dynarray->elements, DYNARRAY_SIZE_INIT); // XXX
    memset(dynarray->elements, 0, DYNARRAY_SIZE_INIT);
    dynarray->size = 0;
    dynarray->max_size = DYNARRAY_SIZE_INIT;
}

size_t dynarray_get_size(dynarray_t *dynarray)
{
    return dynarray->size;
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
