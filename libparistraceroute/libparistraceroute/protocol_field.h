#ifndef PROTOCOL_FIELD_H
#define PROTOCOL_FIELD_H

/**
 * \file protocol_field.h
 * \brief Header for the data fields of a protocol
 */

#include <stdint.h>
#include <stdbool.h>

#include "use.h"    // USE_BITS
#include "field.h"

/**
 * \struct protocol_field_t
 * \brief Structure describing a data field for a protocol
 */

typedef struct {
    const char  * key;          /**< Pointer to an identifying key */
    fieldtype_t   type;         /**< Enum to set the type of data stored in the field */
    size_t        offset;       /**< Offset from start of header data */
#ifdef USE_BITS
    size_t        offset_bits;  /**< Additional offset in bits for non-aligned field (set to 0 otherwise) */
    size_t        size_in_bits; /**< Size in bits (only useful for non-aligned fields and fields not having a size equal to 8 * n bits */
#endif

    // The following callbacks allows to perform specific treatment when we translate
    // field content in packet content and vice versa. Most of time there are set
    // to NULL and we call default functions which manage endianness and so on. 

    field_t     * (*get)(const uint8_t * header);                  /**< Allocate a field_t instance corresponding to this field */
    bool          (*set)(uint8_t * header, const field_t * field); /**< Update a header according to a field. Return true iif successful */
} protocol_field_t;

/**
 * \brief Retrieve the size (in bytes) to a protocol field
 * \param protocol_field A pointer to the protocol_field_t instance
 * \return The corresponding size (in bytes)
 */

size_t protocol_field_get_size(const protocol_field_t * protocol_field);

/**
 * \brief Write in a buffer the value stored in a field according to
 *   the size and the offset stored in a protocol_field_t instance.
 * \param protocol_field A pointer to the corresponding protocol field.
 * \warning Fields of type TYPE_INT4 and TYPE_STRING are not supported.
 * \param buffer The buffer in which the value is written.
 * \param field The field storing the value to write in the buffer.
 * \return true iif successful
 */

bool protocol_field_set(const protocol_field_t * protocol_field, uint8_t * buffer, const field_t * field);

/**
 * \brief Retrieve the offset stored in a protocol_field_t instance.
 * \param protocol_field A pointer to a protocol_field_t instance.
 * \return The corresponding offset.
 */

size_t protocol_field_get_offset(const protocol_field_t * protocol_field);

/**
 * \brief Print information related to a protocol_field_t instance
 * \param protocol_field The protocol_field_t instance to print
 */

void protocol_field_dump(const protocol_field_t * protocol_field);

#endif

