#ifndef FIELD_H
#define FIELD_H

/**
 * \file field.h
 * \brief Header for header fields
 */

#include <stddef.h> // size_t
#include <stdint.h>

/**
 * \enum fieldtype_t
 * \brief Enumeration of the possible data types for a header field
 */
typedef enum {
	/** 4 bit integer */
    TYPE_INT4,
	/** 8 bit integer */
    TYPE_INT8,
	/** 16 bit integer */
    TYPE_INT16,
	/** 32 bit integer */
    TYPE_INT32,
	/** String */
    TYPE_STRING
} fieldtype_t;

typedef union {
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
	/** Pointer to string data */
    char          * string;
} value_t;

/**
 * \struct field_t
 * \brief Structure describing a header field
 */
typedef struct {
	/** Pointer to a unique identifier key */
    char *key;
	/** Union of all field data */
    value_t value;
    fieldtype_t type;
} field_t;

/**
 * \brief Create a field structure to hold an 8 bit integer value
 * \param key Pointer to a unique header identifier
 * \param value Value to store in the field
 * \return Structure containing the newly created field
 */

field_t * field_create_int8  (const char * key, uint8_t  value);

/**
 * \brief Create a field structure to hold a 16 bit integer value
 * \param key Pointer to a unique header identifier
 * \param value Value to store in the field
 * \return Structure containing the newly created field
 */

field_t * field_create_int16 (const char * key, uint16_t value);

/**
 * \brief Create a field structure to hold a 32 bit integer value
 * \param key Pointer to a unique header identifier
 * \param value Value to store in the field
 * \return Structure containing the newly created field
 */

field_t * field_create_int32 (const char * key, uint32_t value);

/**
 * \brief Create a field structure to hold a string
 * \param key Pointer to a unique header identifier
 * \param value Value to copy in the field
 * \return Structure containing the newly created field
 */

field_t * field_create_string(const char * key, const char * value);

/**
 *
 */

field_t * field_create(fieldtype_t type, const char *key, void *value);

/**
 *
 */

field_t * field_create_from_network(fieldtype_t type, const char *key, void *value);

/**
 * \brief Delete a field structure
 * \param field Pointer to the field structure to delete
 */

void      field_free(field_t *field);

/**
 * \brief Macro shorthand for field_create_int8
 * \param x Pointer to a char* key to identify the field
 * \param y Value to store in the field
 * \return Structure containing the newly created field
 */

#define I8(x, y)  field_create_int8(x, y)

/**
 * \brief Macro shorthand for field_create_int16
 * \param x Pointer to a char* key to identify the field
 * \param y Value to store in the field
 * \return Structure containing the newly created field
 */

#define I16(x, y) field_create_int16(x, y)

/**
 * \brief Macro shorthand for field_create_int32
 * \param x Pointer to a char* key to identify the field
 * \param y Value to store in the field
 * \return Structure containing the newly created field
 */

#define I32(x, y) field_create_int32(x, y)

/**
 * \brief Macro shorthand for field_create_string
 * \param x Pointer to a char* key to identify the field
 * \param y String to store in the field
 * \return Structure containing the newly created field
 */

#define STR(x, y) field_create_string(x, y)

// Accessors

size_t field_get_type_size(fieldtype_t type);
size_t field_get_size(field_t *field);

// Comparison

int field_compare(field_t *field1, field_t *field2);

// Dump

void field_dump(field_t *field);

#endif
