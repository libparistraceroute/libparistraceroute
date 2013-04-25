#ifndef FIELD_H
#define FIELD_H

/**
 * \file field.h
 * \brief A field is a part of a header network protocol. 
 */

#include <stddef.h> // size_t
#include <stdint.h>


typedef union{
	uint32_t	d32[4];
	uint64_t	d64[2];
} uint128_t;

/**
 * \enum fieldtype_t
 * \brief Enumeration of the possible data types for a field
 */

typedef enum {
<<<<<<< HEAD
	/** 4 bit integer */
    TYPE_INT4,
	/** 8 bit integer */
    TYPE_INT8,
	/** 16 bit integer */
    TYPE_INT16,
	/** 32 bit integer */
    TYPE_INT32,
	/** 64 bit integer */
    TYPE_INT64,
	/** 128 bit */
    TYPE_INT128,
    /** max integer */
    TYPE_INTMAX,
	/** String */
    TYPE_STRING
=======
    TYPE_INT4,             /**< 4 bit integer  */
    TYPE_INT8,             /**< 8 bit integer  */
    TYPE_INT16,            /**< 16 bit integer */
    TYPE_INT32,            /**< 32 bit integer */
    TYPE_INT64,            /**< 64 bit integer */
    TYPE_INTMAX,           /**< max integer    */
    TYPE_STRING            /**< string         */
>>>>>>> origin/master
} fieldtype_t;

/**
 * \union value_t
 * \brief Store a value carried by a field.
 */

typedef union {
<<<<<<< HEAD
	/** Pointer to raw data */
    void          * value;
	/** Value of data as a 4 bit integer */
    unsigned int    int4:4; 
	/** Value of data as an 8 bit integer */
    uint8_t         int8;
	/** Value of data as a 16 bit integer */
    uint16_t        int16;
	/** Value of data as a 32 bit integer */
    uint32_t        int32;
    /** Value of data as a 64 bit integer */
    uint64_t        int64;
    /** Value of data of a 128 Bit bitfield as 4x uint32_t, 2x uint64_t */
    uint128_t		int128;
    /** Value of data as a max integer */
    uintmax_t       intmax;
	/** Pointer to string data */
    char          * string;
=======
    void         * value;  /**< Pointer to raw data               */
    unsigned int   int4:4; /**< Value of data as a 4 bit integer  */
    uint8_t        int8;   /**< Value of data as an 8 bit integer */
    uint16_t       int16;  /**< Value of data as a 16 bit integer */
    uint32_t       int32;  /**< Value of data as a 32 bit integer */
    uint64_t       int64;  /**< Value of data as a 64 bit integer */
    uintmax_t      intmax; /**< Value of data as a max integer    */
    char         * string; /**< Pointer to string data            */
>>>>>>> origin/master
} value_t;

/**
 * \struct field_t
 * \brief Structure describing a header field
 */

typedef struct {
<<<<<<< HEAD
	/** Pointer to a unique identifier key */
    char      * key;
	/** Union of all field data */
    value_t     value;
    fieldtype_t type;
=======
    char * key;           /**< Pointer to a unique identifier key */
    value_t value;        /**< Union of all field data            */
    fieldtype_t type;     /**< Type of data stored in the field   */
>>>>>>> origin/master
} field_t;

/**
 * \brief Create a field structure to hold an 8 bit integer value
 * \param key The name which identify the field to create
 * \param value Value to store in the field
 * \return Structure containing the newly created field
 */

field_t * field_create_int8(const char * key, uint8_t value);

/**
 * \brief Create a field structure to hold a 16 bit integer value
 * \param key The name which identify the field to create
 * \param value Value to store in the field
 * \return Structure containing the newly created field
 */

field_t * field_create_int16(const char * key, uint16_t value);

/**
 * \brief Create a field structure to hold a 32 bit integer value
 * \param key The name which identify the field to create
 * \param value Value to store in the field
 * \return Structure containing the newly created field
 */

field_t * field_create_int32(const char * key, uint32_t value);

/**
 * \brief Create a field structure to hold a 64 bit integer value
 * \param key The name which identify the field to create
 * \param value Value to store in the field
 * \return Structure containing the newly created field
 */

<<<<<<< HEAD
field_t * field_create_int128(const char * key, uint128_t value);


field_t * field_create_intmax (const char * key, uintmax_t value);
=======
field_t * field_create_int64(const char * key, uint64_t value);

/**
 * \brief Create a field structure to hold a uintmax_t value
 * \param key The name which identify the field to create
 * \param value Value to store in the field
 * \return Structure containing the newly created field
 */

field_t * field_create_intmax(const char * key, uintmax_t value);
>>>>>>> origin/master

/**
 * \brief Create a field structure to hold a string
 * \param key The name which identify the field to create
 * \param value Value to copy in the field
 * \return Structure containing the newly created field
 */

field_t * field_create_string(const char * key, const char * value);

/**
 * \brief Create a field structure to hold an address
 * \param key The name which identify the field to create
 * \param value Address to copy in the field
 * \return Structure containing the newly created field
 */

field_t * field_create(fieldtype_t type, const char * key, void * value);

/**
 * \brief Create a field according to a field type and a buffer passed as parameters/
 * \param type The field type
 * \param key The name which identify the field to create
 * \param value Address of the value to store in the field
 * \return Structure containing the newly created field
 */

field_t * field_create_from_network(fieldtype_t type, const char * key, void * value);

/**
 * \brief Delete a field structure
 * \param field Pointer to the field structure to delete
 */

void field_free(field_t *field);

/**
 * \brief Macro shorthand for field_create_int8
 * \param x Pointer to a char * key to identify the field
 * \param y Value to store in the field
 * \return Structure containing the newly created field
 */

#define I8(x, y)  field_create_int8(x, y)

/**
 * \brief Macro shorthand for field_create_int16
 * \param x Pointer to a char * key to identify the field
 * \param y Value to store in the field
 * \return Structure containing the newly created field
 */

#define I16(x, y) field_create_int16(x, y)

/**
 * \brief Macro shorthand for field_create_int32
 * \param x Pointer to a char * key to identify the field
 * \param y Value to store in the field
 * \return Structure containing the newly created field
 */

#define I32(x, y) field_create_int32(x, y)

/**
 * \brief Macro shorthand for field_create_int64
 * \param x Pointer to a char * key to identify the field
 * \param y Value to store in the field
 * \return Structure containing the newly created field
 */

#define I64(x, y) field_create_int64(x, y)
<<<<<<< HEAD
#define I128(x, y) field_create_int128(x, y)
=======

/**
 * \brief Macro shorthand for field_create_intmax
 * \param x Pointer to a char * key to identify the field
 * \param y Value to store in the field
 * \return Structure containing the newly created field
 */

>>>>>>> origin/master
#define IMAX(x, y) field_create_intmax(x, y)

/**
 * \brief Macro shorthand for field_create_string
 * \param x Pointer to a char * key to identify the field
 * \param y String to store in the field
 * \return Structure containing the newly created field
 */

#define STR(x, y) field_create_string(x, y)

/**
 * \brief Return the size (in bytes) related to a field type
 * \param type A field type
 * \return 0 if type is equal to TYPE_INT4 or TYPE_STRING,
 *  the size related to the field type otherwise.
 */

size_t field_get_type_size(fieldtype_t type);

/**
 * \brief Return the size (in bytes) related to a field
 * \param field A field instance
 * \return 0 if the field type is equal to TYPE_INT4 or TYPE_STRING
 *         The size of the value that can be stored in this field
 */

size_t field_get_size(const field_t * field);

/**
 * \brief Compare two fields, for instance in order to sort them
 * \param field1 The first field instance
 * \param field2 The second field instance
 * \return -2 if field1 and field2 cannot be compared
 *         -3 if the field type is not supported
 *         A negative value if field1 < field2
 *         A positive value if field1 > field2
 *         0 if field1 == field2
 */

// TODO the return values are not well-designed
int field_compare(const field_t * field1, const field_t * field2);

/**
 * \brief Print the content of a field
 * \param field A field instance
 */

void field_dump(const field_t * field);

#endif
