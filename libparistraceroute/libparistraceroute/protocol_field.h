#ifndef PROTOCOL_FIELD_H
#define PROTOCOL_FIELD_H

/**
 * \file protocol_field.h
 * \brief Header for the data fields of a protocol
 */

#include <stdint.h>

#include "field.h"

/**
 * \struct protocol_field_t
 * \brief Structure describing a data field for a protocol
 */

typedef struct {
    char        *   key;                                     /** Pointer to an identifying key */
    fieldtype_t     type;                                    /** Enum to set the type of data stored in the field */
    size_t          offset;                                  /** Offset from start of header data */
    field_t     * (*get)(uint8_t * buffer);                  /** Getter function */
    int           (*set)(uint8_t * buffer, field_t * field); /** Setter function */
} protocol_field_t;

size_t protocol_field_get_size(const protocol_field_t * protocol_field);

int protocol_field_set(protocol_field_t * protocol_field, uint8_t * buffer, field_t * field);

size_t protocol_field_get_offset(const protocol_field_t * protocol_field);

#endif

