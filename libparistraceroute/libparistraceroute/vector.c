#include <stdlib.h>
#include <string.h>

#include "vector.h"

#define VECTOR_SIZE_INIT  5
#define VECTOR_SIZE_INC   5


vector_t * vector_create(size_t size)
{
    vector_t * vector = malloc(size);
    if (vector) {
        vector->cell_size = size;
        vector->cells = calloc(VECTOR_SIZE_INIT, vector->cell_size);
        memset(vector->cells, 0, VECTOR_SIZE_INIT * vector->cell_size);
        vector->num_cells = 0;
        vector->max_cells = VECTOR_SIZE_INIT;
    }   
    return vector;
}


void vector_free(vector_t * vector, void (* element_free)(void * element)) { 
    unsigned int i;

    if (vector) {
        if (vector->cells) {
            if (element_free) {
                for(i = 0; i < vector->num_cells; i++) {
                    element_free((uint8_t *)(vector->cells) + i * (vector->cell_size));
                }
            }
            free(vector->cells);
        }
        free(vector);
    }
}

void vector_push_element(vector_t * vector, void * element)
{
    // If the vector is full, allocate VECTOR_SIZE_INC
    // cells in the vector
    if (vector->num_cells == vector->max_cells) {
        vector->cells = realloc(
            vector->cells,
            (vector->num_cells + VECTOR_SIZE_INC) * vector->cell_size
        );
        memset(
            (uint8_t *)(vector->cells) + (vector->num_cells) * (vector->cell_size), 0,
            VECTOR_SIZE_INC * vector->cell_size
        );
        vector->max_cells += VECTOR_SIZE_INC;
    }

    // Add the new element and update exposed size
    memcpy((uint8_t *)(vector->cells) + (vector->num_cells) * vector->cell_size, element, vector->cell_size);
    vector->num_cells++;
}

int vector_del_ith_element(vector_t * vector, unsigned int i)
{
    if (i >= vector->num_cells)
        return 0;
    /* Let's move all elements from the (i+1)-th to the left */
    memmove((uint8_t *)(vector->cells) + i * (vector->cell_size), (uint8_t *)(vector->cells) + (i + 1) * vector->cell_size, (vector->num_cells - i - 1) * vector->cell_size);
    vector->num_cells--;
    return 1;
}

void vector_clear(vector_t * vector, void (*element_free)(void * element))
{
    unsigned int i;

    if (vector) {
        for(i = 0; i < vector->num_cells; i++) {
            element_free((uint8_t *)(vector->cells) + i * (vector->cell_size));
        }
        vector->cells = realloc(vector->cells, VECTOR_SIZE_INIT * vector->cell_size); // XXX
        memset(vector->cells, 0, VECTOR_SIZE_INIT * vector->cell_size);
        vector->num_cells = 0;
        vector->max_cells = VECTOR_SIZE_INIT;
    }
}

unsigned vector_get_number_of_cells(const vector_t * vector)
{
    return vector ? vector->num_cells : 0;
}

void * vector_get_cells(const vector_t * vector)
{
    return vector ? vector->cells : NULL;
}

size_t vector_get_cell_size(const vector_t * vector) {
    return vector ? vector->cell_size : 0;
}

void * vector_get_ith_element(const vector_t * vector, unsigned int i)
{
    return (i >= vector->num_cells) ? NULL : ((uint8_t *)(vector->cells) + i * (vector->cell_size));
}

int vector_set_ith_element(vector_t * vector, unsigned int i, void * element)
{
    if (i > vector->num_cells) return -1; // out of range
    memcpy((uint8_t *)(vector->cells) + i * (vector->cell_size), element, vector->cell_size);
    return 0;
}
