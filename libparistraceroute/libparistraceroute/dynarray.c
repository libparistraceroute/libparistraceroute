#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

#include "dynarray.h"

#define DYNARRAY_SIZE_INIT  5
#define DYNARRAY_SIZE_INC   5

dynarray_t * dynarray_create(void)
{
    dynarray_t * dynarray = malloc(sizeof(dynarray_t));
    if (dynarray) {
        dynarray->elements = calloc(DYNARRAY_SIZE_INIT, sizeof(void *));
        memset(dynarray->elements, 0, DYNARRAY_SIZE_INIT * sizeof(void *));
        dynarray->size = 0;
        dynarray->max_size = DYNARRAY_SIZE_INIT;
    }
    return dynarray;
}

dynarray_t * dynarray_dup(const dynarray_t * dynarray, void * (*element_dup)(void *))
{
    dynarray_t   * da;
    unsigned int   i, size;

    if ((da = dynarray_create())) {
        // TODO need dynarray_resize
        size = dynarray_get_size(dynarray);
        for (i = 0; i < size; i++) {
            void * element = dynarray_get_ith_element(dynarray, i);
            if (element_dup) {
                dynarray_push_element(da, element_dup(element));
            } else {
                dynarray_push_element(da, element);
            }
        }
    }

    return da;
}

void dynarray_free(dynarray_t * dynarray, void (*element_free)(void *element)) { 
    size_t i, size;

    if (dynarray) {
        if (dynarray->elements) {
            if (element_free) {
                size = dynarray_get_size(dynarray);
                for(i = 0; i < size; i++) {
                    if (dynarray->elements[i]) {
                        element_free(dynarray->elements[i]);
                    }
                }
            }
            free(dynarray->elements);
        }
        free(dynarray);
    }
}

bool dynarray_push_element(dynarray_t * dynarray, void * element)
{
    // If the dynarray is full, allocate DYNARRAY_SIZE_INC
    // cells in the dynarray
    if (dynarray->size == dynarray->max_size) {
        dynarray->elements = realloc(
            dynarray->elements,
            (dynarray->size + DYNARRAY_SIZE_INC) * sizeof(void *)
        );
        if (!dynarray->elements) return false;
        memset(
            dynarray->elements + dynarray->size, 0,
            DYNARRAY_SIZE_INC * sizeof(void *)
        );
        dynarray->max_size += DYNARRAY_SIZE_INC;
    }

    // Add the new element and update exposed size
    dynarray->elements[dynarray->size] = element;
    dynarray->size++;
    return true;
}

/* void dynarray_add_element(dynarray_t * dynarray, void * element, size_t sizeof_element) {
    // TODO factorize with dynarray_push_element
    if (dynarray->size == dynarray->max_size) {
        dynarray->elements = realloc(dynarray->elements, (dynarray->size + DYNARRAY_SIZE_INC) * sizeof(void *));
        memset(dynarray->elements + dynarray->size, 0, DYNARRAY_SIZE_INC * sizeof(void *));
        dynarray->max_size += DYNARRAY_SIZE_INC;
    }
    copy = malloc(sizeof(sizeof_element));
    memcpy(copy, element, sizeof(sizeof_element));
    dynarray->elements[dynarray->size] = copy;
    dynarray->size++;
} */

bool dynarray_del_ith_element(dynarray_t * dynarray, size_t i, void (*element_free) (void * element))
{
    /*
    bool ret = false;
    if (i < dynarray->size)
    // Let's move all elements from the (i+1)-th to the left
    memmove(dynarray->elements + i, dynarray->elements + (i + 1), (dynarray->size - i - 1) * sizeof(void *));
    dynarray->size--;
    ret = true;
    return ret;*/
    return dynarray_del_n_elements(dynarray, i, 1, element_free);
}

bool dynarray_del_n_elements(dynarray_t * dynarray, size_t i, size_t n, void (*element_free)(void * element))
{
    bool   ret = false;
    size_t j , num_elements = dynarray_get_size(dynarray);

    if (dynarray && i + n <= num_elements) {
        if (element_free) {
            for (j = i; j < i + n; i++) {
                element_free(dynarray_get_ith_element(dynarray, j));
            }
        }
        memmove(dynarray->elements + i , dynarray->elements + i + n, (num_elements - (i + n)) * sizeof(void *));
        dynarray->size -= n ;
        ret = true;
    }
    return ret;
}

void dynarray_clear(dynarray_t * dynarray, void (*element_free)(void * element))
{
    size_t i, size;

    if (dynarray) {
        size = dynarray_get_size(dynarray);
        if (element_free) {
            for(i = 0; i < size; i++) {
                element_free(dynarray->elements[i]);
            }
        }
        dynarray->elements = realloc(dynarray->elements, DYNARRAY_SIZE_INIT * sizeof(void *)); // XXX
        memset(dynarray->elements, 0, DYNARRAY_SIZE_INIT * sizeof(void *));
        dynarray->size = 0;
        dynarray->max_size = DYNARRAY_SIZE_INIT;
    }
}


size_t dynarray_get_size(const dynarray_t * dynarray)
{
    return dynarray ? dynarray->size : 0;
}

void ** dynarray_get_elements(dynarray_t * dynarray)
{
    return dynarray->elements;
}

void * dynarray_get_ith_element(const dynarray_t * dynarray, unsigned int i)
{
    return (i >= dynarray->size) ? NULL : dynarray->elements[i];
}

bool dynarray_set_ith_element(dynarray_t * dynarray, unsigned int i, void * element)
{
    if (i > dynarray->size) return false; // out of range
    dynarray->elements[i] = element;
    return true;
}
