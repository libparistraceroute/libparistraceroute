#ifndef LIBPT_GENERATOR_H
#define LIBPT_GENERATOR_H

#include "field.h"

#define END_GENERATOR_FIELDS { .key = NULL }

/**
 * A generator_t in an object which produce a sequence of values.
 */

typedef struct generator_s {
    const char * name;                                    /**< Name of the generator */
    double    (* get_next_value)(struct generator_s * g); /**< We generate the i-th value */
    // TODO we should use field_t * to be coherent with protocol.h (or adapt protocol module)
    field_t ** fields;                                    /**< Fields embedded in the generator */
    size_t     num_fields;                                /**< Number of fields embedded in the generator */
    size_t     size;                                      /**< The size in bytes of a generator_t instance */
    double     value;                                     /**< The current value returned by the generator */
} generator_t;

/**
 * \brief Create a generator_t instance according to its string identifier.
 * \param A string identifying the kind of generator (for example "uniform").
 * \return A generator_t instance if successful, NULL otherwise.
 */

generator_t * generator_create_by_name(const char * name);

/**
 * \brief Release a generator_t instance from the memory.
 * \param generator A generator_t instance.
 */

void generator_free(generator_t * generator);

/**
 * \brief Dump to the standard output a generator_t instance.
 * \param generator A generator_t instance.
 */

void generator_dump(const generator_t * generator);

/**
 * \brief Duplicate a generator_t instance.
 * \param generator A generator_t instance if successful, NULL otherwise.
 */

generator_t * generator_dup(const generator_t * generator);

/**
 * \brief Retrieve the size in bytes of a generator_t instance
 * \param generator A generator_t instance.
 * \return The size in bytes.
 */

size_t generator_get_size(const generator_t * generator);

/**
 * \brief Retrieve the number of fields embedded in a generator_t instance.
 * \param generator A generator_t instance.
 * \return The number of fields.
 */

size_t generator_get_num_fields(const generator_t * generator);

/**
 * \brief
 */

bool generator_extract_value(const generator_t * generator, const char * key, void * value);

/**
 * \brief Fetch the current value produced by a generator_t instance.
 * \param generator A generator_t instance.
 * \return The current value.
 */

double generator_get_value(const generator_t * generator);

/**
 * \brief Fetch the next value produced by a generator_t instance.
 * \param generator A generator_t instance.
 * \return The next value.
 */

double generator_next_value(generator_t * generator);

/**
 * \brief Initializes a generator field.
 *   Example: generator_set(u, DOUBLE("mean", 2.3))
 * \param generator A generator_t instance.
 * \param field A field_t instance having a key recognized by thus generator.
 * \return true iif successful.
 */

bool generator_set_field(generator_t * generator, field_t * field);

/**
 * \brief Search a registered generator in the library according to its name
 * \param name The name of the generator (for example "uniform", ...)
 * \return A pointer to the corresponding generator if any, NULL othewise
 */

const generator_t * generator_search(const char * name);

/**
 * \brief Register a generator in the library.
 * \param generator A generator_t instance describing the generator to register.
 */

void generator_register(generator_t * generator);

#define GENERATOR_REGISTER(MOD)    \
static void __init_ ## MOD (void) __attribute__ ((constructor));    \
static void __init_ ## MOD (void) {    \
    generator_register(&MOD); \
}

#endif // LIBPT_GENERATOR_H
