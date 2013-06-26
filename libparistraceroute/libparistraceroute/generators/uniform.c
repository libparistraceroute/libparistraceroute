#include <stdlib.h>           // malloc()

#include "field.h"
#include "../generator.h"     // generator_t

static field_t uniform_fields[] = {
    {
        .key       = "mean",
        .type      = TYPE_DOUBLE,
        .value.dbl = 2,
    },
    END_GENERATOR_FIELDS
};

typedef struct {
    generator_t generator; // parent class
} uniform_generator_t;

static double uniform_generator_get_next_value(generator_t * uniform_generator) {
    double next_value;
    generator_extract_value(uniform_generator, "mean", &next_value);
    return next_value;
}


void uniform_generator_free(uniform_generator_t * uniform_generator) {

    if (uniform_generator) {
        //generator_free(uniform_generator->generator);
        free(uniform_generator);
    }
}

static generator_t uniform = {
    .name           = "uniform",
    .get_next_value = uniform_generator_get_next_value,
    .fields         = uniform_fields,
    .num_fields     = 1,
    .size           =  sizeof(uniform_generator_t),
    .value          = 0,
};

GENERATOR_REGISTER(uniform);
