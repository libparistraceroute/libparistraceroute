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
	/** Pointer to an identifying key */
    char *key;
	/** Enum to set the type of data stored in the field */
    fieldtype_t type;
	/** Offset from start of header data */
    size_t offset;
} protocol_field_t;

#endif

