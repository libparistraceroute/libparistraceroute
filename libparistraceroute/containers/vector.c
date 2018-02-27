#include "config.h"
#include "vector.h"

#include <stdint.h> // uin8_t
#include <stdlib.h> // malloc
#include <string.h> // memset

#define VECTOR_SIZE_INIT  5
#define VECTOR_SIZE_INC   5


static void * vector_get_ith_element_impl(const vector_t * vector, size_t i) {
    return (uint8_t *)(vector->cells) + i * (vector->cell_size);
}

static void vector_initialize(vector_t * vector) {
    memset(vector->cells, 0, VECTOR_SIZE_INIT * vector->cell_size);
    vector->num_cells = 0;
    vector->max_cells = VECTOR_SIZE_INIT;
}

vector_t * vector_create_impl(
    size_t size,
    void * (*callback_dup)(const void *),
    void   (*callback_free)(void *),
    void   (*callback_dump)(const void *)
) {
    vector_t * vector = malloc(sizeof(vector_t));
    if (vector) {
        vector->cell_size = size;
        vector->cells = calloc(VECTOR_SIZE_INIT, vector->cell_size);
        vector->cells_free = callback_free;
        vector->cells_dump = callback_dump;
        vector->cells_dup = callback_dup;
        vector_initialize(vector);
    }
    return vector;
}

void vector_free(vector_t * vector, void (*element_free)(void * element)) {
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
        // If the vector is full, allocate VECTOR_SIZE_INC
        // cells in the vector
        if (vector->num_cells == vector->max_cells) {
            vector->cells = realloc(
                    vector->cells,
                    (vector->num_cells + VECTOR_SIZE_INC) * vector->cell_size
                    );
            memset(
                    vector_get_ith_element_impl(vector, vector->num_cells),
                    0,
                    VECTOR_SIZE_INC * vector->cell_size
                  );
            vector->max_cells += VECTOR_SIZE_INC;
        }

        // Add the new element and update exposed size
        memcpy(vector_get_ith_element_impl(vector, vector->num_cells), element, vector->cell_size);
        vector->num_cells++;
        ret = true;
    }
    return ret;
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
    return ret;
}

void vector_dump(vector_t * vector) {
    size_t i;
    for(i = 0; i < vector_get_num_cells(vector); i++) {
        vector->cells_dump(vector_get_ith_element_impl(vector, i));
    }
}

vector_t * vector_deep_dup(const vector_t* vector){
    vector_t* copy = vector_create(vector->cell_size,vector->cells_dup, vector->cells_free, vector->cells_dump);
    size_t i;
    for(i = 0; i < vector->num_cells; ++i){
        vector_push_element(copy, vector->cells_dup(vector_get_ith_element(vector, i)));
    }
    return copy;
}

vector_t * vector_dup(const vector_t* vector){
    vector_t* copy = vector_create(vector->cell_size,vector->cells_dup, vector->cells_free, vector->cells_dump);
    size_t i;
    for(i = 0; i < vector->num_cells; ++i){
        vector_push_element(copy, vector_get_ith_element(vector, i));
    }
    return copy;
}
