#ifndef FIELD_H
#define FIELD_H

/**
 * \file field.h
 * \brief A field is a part of a header network protocol.
 */

#include <stddef.h>  // size_t
#include <stdint.h>  // uint*_t
#include <stdbool.h> // bool

#include "address.h" // address_t, ipv4_t, ipv6_t

struct generator_s;

typedef union{
	uint32_t	d32[4];
	uint64_t	d64[2];
} uint128_t;

/**
 * \enum fieldtype_t
 * \brief Enumeration of the possible data types for a field
 */

typedef enum {
#ifdef USE_IPV4
    TYPE_IPV4,              /**< ipv4_t structure */
#endif
#ifdef USE_IPV6
    TYPE_IPV6,              /**< ipv6_t structure */
#endif
#ifdef USE_BITS
    TYPE_BITS,              /**< n < 8 bits integer (right aligned) */
#endif
    TYPE_UINT8,             /**< 8 bits integer     */
    TYPE_UINT16,            /**< 16 bits integer    */
    TYPE_UINT32,            /**< 32 bits integer    */
    TYPE_UINT64,            /**< 64 bits integer    */
    TYPE_UINT128,           /**< 128 bits integer   */
    TYPE_UINTMAX,           /**< max integer        */
    TYPE_DOUBLE,            /**< double             */
    TYPE_STRING,            /**< string             */
    TYPE_GENERATOR          /**< generator          */
} fieldtype_t;

/**
 * \brief Convert a field type in the corresponding human
 *    readable string. 
 * \param type A field type.
 * \return The corresponding string.
 */

const char * field_type_to_string(fieldtype_t type);

/**
 * \union value_t
 * \brief Store a value carried by a field.
 */

typedef union {
    void                * value;     /**< Pointer to raw data                */
#ifdef USE_IPV4
    ipv4_t                ipv4;      /**< Value of data as a IPv4 address    */
#endif
#ifdef USE_IPV6
    ipv6_t                ipv6;      /**< Value of data as a IPv6 address    */
#endif
#ifdef USE_BITS
    // TODO test with uint128_t to handle bigger bit-level fields
    uint8_t               bits;      /**< Value of data as a n<8 bit integer */
#endif
    uint8_t               int8;      /**< Value of data as a   8 bit integer */
    uint16_t              int16;     /**< Value of data as a  16 bit integer */
    uint32_t              int32;     /**< Value of data as a  32 bit integer */
    uint64_t              int64;     /**< Value of data as a  64 bit integer */
    uint128_t             int128;    /**< Value of data as a 128 bit integer */
    uintmax_t             intmax;    /**< Value of data as a max integer     */
    double                dbl;       /**< Value of data as a double          */
    char                * string;    /**< Pointer to string data             */
    struct generator_s  * generator; /**< Pointer to generator_t data        */
} value_t;

/**
 * \struct field_t
 * \brief Structure describing a header field
 */

typedef struct {
    const char  * key;   /**< Pointer to a unique identifier key; the
                          * referenced memory is not freed when the
                          * field is freed */
    value_t       value; /**< Union of all field data; the referenced
                          * memory is freed if it's a string or
                          * generator, when the field is freed */
    fieldtype_t   type;  /**< Type of data stored in the field */
} field_t;

/**
 * \brief Create a field structure to hold a generic address. 
 * \param key The name which identify the field to create.
 * \param address Address to store in the field.
 * \return Structure containing the newly created field.
 */

field_t * field_create_address(const char * key, const address_t * address);

#ifdef USE_IPV4
/**
 * \brief Create a field structure to hold an IPv4 address. 
 * \param key The name which identify the field to create.
 * \param address Address to store in the field.
 * \return Structure containing the newly created field.
 */

field_t * field_create_ipv4(const char * key, ipv4_t ipv4);
#endif

#ifdef USE_IPV6
/**
 * \brief Create a field structure to hold an IPv6 address. 
 * \param key The name which identify the field to create.
 * \param address Address to store in the field.
 * \return Structure containing the newly created field.
 */

field_t * field_create_ipv6(const char * key, ipv6_t ipv6);
#endif

#ifdef USE_BITS
/**
 * \brief Create a field structure to hold an n<8 bit integer value.
 * \param key The name which identify the field to create.
 * \param value Value to store in the field.
 * \param size_in_bits
 * \return Structure containing the newly created field.
 */

field_t * field_create_bits(const char * key, const void * value, size_t offset_in_bits, size_t size_in_bits);
#endif

/**
 * \brief Create a field structure to hold an 8 bit integer value
 * \param key The name which identify the field to create
 * \param value Value to store in the field
 * \return Structure containing the newly created field
 */

field_t * field_create_uint8(const char * key, uint8_t value);

/**
 * \brief Create a field structure to hold a 16 bit integer value
 * \param key The name which identify the field to create
 * \param value Value to store in the field
 * \return Structure containing the newly created field
 */

field_t * field_create_uint16(const char * key, uint16_t value);

/**
 * \brief Create a field structure to hold a 32 bit integer value
 * \param key The name which identify the field to create
 * \param value Value to store in the field
 * \return Structure containing the newly created field
 */

field_t * field_create_uint32(const char * key, uint32_t value);

/**
 * \brief Create a field structure to hold a 64 bit integer value
 * \param key The name which identify the field to create
 * \param value Value to store in the field
 * \return Structure containing the newly created field
 */

field_t * field_create_uint64(const char * key, uint64_t value);

/**
 * \brief Create a field structure to hold a 128 bit integer value
 * \param key The name which identify the field to create
 * \param value Value to store in the field
 * \return Structure containing the newly created field
 */

field_t * field_create_uint128(const char * key, uint128_t value);

/**
 * \brief Create a field structure to hold a uintmax_t value
 * \param key The name which identify the field to create
 * \param value Value to store in the field
 * \return Structure containing the newly created field
 */

field_t * field_create_uintmax(const char * key, uintmax_t value);

/**
 * \brief Create a field structure to hold a double value
 * \param key The name which identify the field to create
 * \param value Value to store in the field
 * \return Structure containing the newly created field
 */

field_t * field_create_double(const char * key, double value);

/**
 * \brief Create a field structure to hold a string
 * \param key The name which identify the field to create
 * \param value Value to copy in the field
 * \return Structure containing the newly created field
 */

field_t * field_create_string(const char * key, const char * value);

/**
 * \brief Create a field structure to hold a generator
 * \param key The name which identify the field to create
 * \param value Value to copy in the field
 * \return Structure containing the newly created field
 */

field_t * field_create_generator(const char * key, struct generator_s * value);

/**
 * \brief Create a field structure to hold an address
 * \param key The name which identify the field to create
 * \param value Address to copy in the field. You may pass NULL to
 *    let the value field uninitialized.
 * \return Structure containing the newly created field
 */

field_t * field_create(fieldtype_t type, const char * key, const void * value);

/**
 * \brief Delete a field structure
 * \param field Pointer to the field structure to delete
 */

void field_free(field_t * field);

/**
 * \brief Duplicate a field instance
 * \param field The field instance that we're duplicating .
 * \return The duplicated field instance if successful, NULL otherwise.
 */

field_t * field_dup(const field_t * field);

#ifdef USE_IPV4
/**
 * \brief Macro shorthand for field_create_ipv4
 * \param x Pointer to a char * key to identify the field
 * \param y Value to store in the field
 * \return Structure containing the newly created field
 */

#    define IPV4(x, y)  field_create_ipv4(x, (ipv4_t) y)
#endif

#ifdef USE_IPV6
/**
 * \brief Macro shorthand for field_create_ipv6
 * \param x Pointer to a char * key to identify the field
 * \param y Value to store in the field
 * \return Structure containing the newly created field
 */

#    define IPV6(x, y)  field_create_ipv6(x, (ipv6_t) y)
#endif

/**
 * \brief Macro shorthand for field_create_address
 * \param x Pointer to a char * key to identify the field
 * \param y Value to store in the field
 * \return Structure containing the newly created field
 */

#define ADDRESS(x, y)  field_create_address(x, y)

#ifdef USE_BITS
#    define BITS(key, size_in_bits, value)  field_create_bits(key, (const void *) (value), sizeof(value) - size_in_bits, size_in_bits)
#endif

/**
 * \brief Macro shorthand for field_create_uint8
 * \param x Pointer to a char * key to identify the field
 * \param y Value to store in the field
 * \return Structure containing the newly created field
 */

#define I8(x, y)  field_create_uint8(x, (uint8_t) y)

/**
 * \brief Macro shorthand for field_create_uint16
 * \param x Pointer to a char * key to identify the field
 * \param y Value to store in the field
 * \return Structure containing the newly created field
 */

#define I16(x, y) field_create_uint16(x, (uint16_t) y)

/**
 * \brief Macro shorthand for field_create_uint32
 * \param x Pointer to a char * key to identify the field
 * \param y Value to store in the field
 * \return Structure containing the newly created field
 */

#define I32(x, y) field_create_uint32(x, (uint32_t) y)

/**
 * \brief Macro shorthand for field_create_uint64
 * \param x Pointer to a char * key to identify the field
 * \param y Value to store in the field
 * \return Structure containing the newly created field
 */

#define I64(x, y) field_create_uint64(x, (uint64_t) y)

/**
 * \brief Macro shorthand for field_create_uint128
 * \param x Pointer to a char * key to identify the field
 * \param y Value to store in the field
 * \return Structure containing the newly created field
 */

#define I128(x, y) field_create_uint64(x, (uint128_t) y)

/**
 * \brief Macro shorthand for field_create_double
 * \param x Pointer to a char * key to identify the field
 * \param y Value to store in the field
 * \return Structure containing the newly created field
 */

#define DOUBLE(x, y) field_create_double(x, (double) y)

/**
 * \brief Macro shorthand for field_create_intmax
 * \param x Pointer to a char * key to identify the field
 * \param y Value to store in the field
 * \return Structure containing the newly created field
 */

#define IMAX(x, y) field_create_uintmax(x, (uintmax_t) y)

/**
 * \brief Macro shorthand for field_create_string
 * \param x Pointer to a char * key to identify the field
 * \param y String to store in the field
 * \return Structure containing the newly created field
 */

#define STR(x, y) field_create_string(x, y)

/**
 * \brief Macro shorthand for field_create_generator
 * \param x Pointer to a char * key to identify the field
 * \param y Pointer to the generator to store in the field
 * \return Structure containing the newly created field
 */

#define GENERATOR(x, y) field_create_generator(x, y)

/**
 * \brief Return the size (in bytes) related to a field type
 * \param type A field type
 * \return
 *    1               if type == TYPE_UINT4
 *    sizeof(void * ) if type == TYPE_STRING or TYPE_GENERATOR
 *    the size of the field value in bytes otherwise 
 */

size_t field_get_type_size(fieldtype_t type);

/**
 * \brief Return the size (in bytes) related to a field
 * \param field A field instance
 * \return See field_get_type_size 
 */

size_t field_get_size(const field_t * field);

/**
 * \brief Compare two fields value, for instance in order to sort them
 * \param field1 The first field instance
 * \param field2 The second field instance
 * \return -2 if field1 and field2 cannot be compared
 *         -3 if the field type is not supported
 *          1 if field1 < field2
 *         -1 if field1 > field2
 *          0 if field1 == field2
 */
//int field_compare(const field_t * field1, const field_t * field2);

bool field_match(const field_t * field1, const field_t * field2);


const char * field_get_key(field_t * field);

bool field_set_value(field_t * field, void * value);

/**
 * \brief Print the content of a field
 * \param field A field instance
 */

void field_dump(const field_t * field);

#endif
