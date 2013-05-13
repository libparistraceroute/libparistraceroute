#include <stdlib.h>
#include <string.h>

#include "vector.h"

#define VECTOR_SIZE_INIT  5
#define VECTOR_SIZE_INC   5


vector_t * vector_create(void)
{
    vector_t * vector = malloc(sizeof(vector_t));
    if (vector) {
        vector->options = calloc(VECTOR_SIZE_INIT, sizeof(struct opt_spec));
        memset(vector->options, 0, VECTOR_SIZE_INIT * sizeof(struct opt_spec));
        vector->num_options = 0;
        vector->max_options = VECTOR_SIZE_INIT;
    }   
    return vector;
}


void vector_free(vector_t * vector, void (* element_free)(struct opt_spec element)) { 
    unsigned int i;

    if (vector) {
        if (vector->options) {
            if (element_free) {
                for(i = 0; i < vector->num_options; i++) {
                    element_free(vector->options[i]);
                }
            }
            free(vector->options);
        }
        free(vector);
    }
}

void vector_push_element(vector_t * vector, struct opt_spec * element)
{
    // If the vector is full, allocate VECTOR_SIZE_INC
    // cells in the vector
    if (vector->num_options == vector->max_options) {
        vector->options = realloc(
            vector->options,
            (vector->num_options + VECTOR_SIZE_INC) * sizeof(struct opt_spec)
        );
        memset(
            vector->options + vector->num_options, 0,
            VECTOR_SIZE_INC * sizeof(struct opt_spec)
        );
        vector->max_options += VECTOR_SIZE_INC;
    }

    // Add the new element and update exposed size
    vector->options[vector->num_options] = * element;
    vector->num_options++;
}

int vector_del_ith_element(vector_t * vector, unsigned int i)
{
    if (i >= vector->num_options)
        return 0;
    /* Let's move all elements from the (i+1)-th to the left */
    memmove(vector->options + i, vector->options + (i + 1), (vector->num_options - i - 1) * sizeof(struct opt_spec));
    vector->num_options--;
    return 1;
}

void vector_clear(vector_t * vector, void (*element_free)(struct opt_spec element))
{
    unsigned int i;

    if (vector) {
        for(i = 0; i < vector->num_options; i++) {
            element_free(vector->options[i]);
        }
        vector->options = realloc(vector->options, VECTOR_SIZE_INIT * sizeof(struct opt_spec)); // XXX
        memset(vector->options, 0, VECTOR_SIZE_INIT * sizeof(struct opt_spec));
        vector->num_options = 0;
        vector->max_options = VECTOR_SIZE_INIT;
    }
}

unsigned vector_get_number_of_options(const vector_t * vector)
{
    return vector ? vector->num_options : 0;
}

struct opt_spec * vector_get_options(const vector_t * vector)
{
    return vector ? vector->options : NULL;
}

struct opt_spec * vector_get_ith_element(const vector_t * vector, unsigned int i)
{
    return (i >= vector->num_options) ? NULL : &vector->options[i];
}

int vector_set_ith_element(vector_t * vector, unsigned int i, struct opt_spec element)
{
    if (i > vector->num_options) return -1; // out of range
    vector->options[i] = element;
    return 0;
}
