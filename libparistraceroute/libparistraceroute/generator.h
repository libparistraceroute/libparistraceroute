#ifndef GENERATOR_H
#define GENERATOR_H

#include <stdbool.h>
#include <stdlib.h>

#include "field.h"

#define END_GENERATOR_FIELDS { .key = NULL }

typedef struct generator_s {
    const char * name;                                    /**< Name of the generator */
    double    (* get_next_value)(struct generator_s * g); /**< We generate the i-th value */
    field_t   ** fields;
    size_t       num_fields;
    size_t       size;
    double       value;
} generator_t;


/**
 * \brief
 */

generator_t * generator_create_by_name(const char * name);

/**
 * \brief
 */

void generator_free(generator_t * generator);

/**
 * \brief
 */

void generator_dump(const generator_t * generator);

/**
 * \brief
 */

generator_t * generator_dup(const generator_t * generator);

/**
 * \brief
 */

size_t generator_get_size(const generator_t * generator);

/**
 * \brief
 */

size_t generator_get_num_fields(const generator_t * generator);

/**
 * \brief
 */

bool generator_extract_value(const generator_t * generator, const char * key, void * value);

/**
 * \brief
 */

double generator_get_value(const generator_t * generator);

/**
 * \brief
 */

double generator_next_value(generator_t * generator);
/**
 * \brief
 */
// generator_set(u, DOUBLE("mean", 2.3))
// return false if mean is not supported in this generator
bool generator_set_field(generator_t * generator, field_t * field);

/**
 * \brief Search a registered generator in the library according to its name
 * \param name The name of the generator (for example "udp", "ipv4", etc.)
 * \return A pointer to the corresponding generator if any, NULL othewise
 */

const generator_t * generator_search(const char * name);

/**
 * \brief Register a generator
 * \param generator Pointer to a generator_t structure describing the generator to register
 * \return None
 */

void generator_register(generator_t * generator);

#define GENERATOR_REGISTER(MOD)    \
static void __init_ ## MOD (void) __attribute__ ((constructor));    \
static void __init_ ## MOD (void) {    \
    generator_register(&MOD); \
}

void protocol_clear() __attribute__((destructor(200)));

#endif

