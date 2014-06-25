#include "config.h"

#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include "vector.h"

#define VECTOR_SIZE_INIT  5

static void * vector_get_ith_element_impl(const vector_t * vector, size_t i) {
    return (uint8_t *)(vector->cells) + i * (vector->cell_size);
}

static void vector_initialize(vector_t * vector) {
    memset(vector->cells, 0, VECTOR_SIZE_INIT * vector->cell_size);
    vector->num_cells = 0;
    vector->max_cells = VECTOR_SIZE_INIT;
}

vector_t * vector_create_impl(size_t size, void (* callback_free)(void *), void (* callback_dump)(const void *)) {
    vector_t * vector = malloc(sizeof(vector_t));
    if (vector) {
        vector->cell_size = size;
        vector->cells = calloc(VECTOR_SIZE_INIT, vector->cell_size);
        vector->cells_free = callback_free;
        vector->cells_dump = callback_dump;
        vector_initialize(vector);
    }
    return vector;
}

void vector_free(vector_t * vector, void (* element_free)(void * element)) {
    size_t i;
    void * element;

    if (vector) {
        if (vector->cells) {
            if (element_free) {
                for(i = 0; i < vector->num_cells; i++) {
                    if ((element = vector_get_ith_element_impl(vector, i))) {
                        element_free(element);
                    }
                }
            }
            free(vector->cells);
        }
        free(vector);
    }
}

bool vector_push_element(vector_t * vector, void * element)
{
    bool ret = false;

    if (vector && element) {
        // If the vector is full, resize to double
        // previous max capacity
        if (vector->num_cells == vector->max_cells) {
            vector_resize(vector, 2 * vector->max_cells);
        }

        // Add the new element and update exposed size
        memcpy(vector_get_ith_element_impl(vector, vector->num_cells), element, vector->cell_size);
        vector->num_cells++;
        ret = true;
    }
    return ret;
}

void vector_resize(vector_t * vector, size_t max_cells)
{
    if (vector) {
        vector->cells = realloc(vector->cells, max_cells * vector->cell_size);
        // If resizing larger, set added memory to 0
        if (max_cells > vector->max_cells) {
            memset(
                    vector->cells + vector->max_cells,
                    0,
                    (max_cells - vector->max_cells) * vector->cell_size
                  );
        }
        vector->max_cells = max_cells;
    }
}

bool vector_del_ith_element(vector_t * vector, size_t i)
{
    if (i >= vector->num_cells) {
        return false;
    }
    memmove(
            vector_get_ith_element_impl(vector, i),
            vector_get_ith_element_impl(vector, i + 1),
            (vector->num_cells - i - 1) * vector->cell_size
           );
    vector->num_cells--;
    return true;
}

void vector_clear(vector_t * vector, void (*element_free)(void * element))
{
    size_t i;
    void * element;    

    if (vector) {
        if (element_free) {
            for(i = 0; i < vector->num_cells; i++) {
                if ((element = vector_get_ith_element_impl(vector, i))) {
                    element_free(element);
                }
            }
        }
        vector->cells = realloc(vector->cells, VECTOR_SIZE_INIT * vector->cell_size);
        vector_initialize(vector);
    }
}

size_t vector_get_num_cells(const vector_t * vector) {
    return vector ? vector->num_cells : 0;
}

size_t vector_get_cell_size(const vector_t * vector) {
    return vector ? vector->cell_size : 0;
}

void * vector_get_ith_element(const vector_t * vector, size_t i) {
    return (i >= vector->num_cells) ?
        NULL :
        vector_get_ith_element_impl(vector, i); 
}

bool vector_set_ith_element(vector_t * vector, size_t i, void * element)
{
    bool ret = false;
    if (i < vector->num_cells) {
        memcpy(vector_get_ith_element_impl(vector, i), element, vector->cell_size);
        ret = true;
    }
    else if (i >= 0) {
        ret = vector_push_element(vector, element);
    }
    return ret;
}

void vector_dump(vector_t * vector) {
    size_t i;
    for(i = 0; i < vector_get_num_cells(vector); i++) {
        vector->cells_dump(vector_get_ith_element_impl(vector, i));
    }
}
