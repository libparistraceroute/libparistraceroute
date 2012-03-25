#ifndef PROTOCOL_FIELD_H
#define PROTOCOL_FIELD_H

#include <stdint.h>

#include "field.h"

typedef struct {
    char *key;
    fieldtype_t type;
    size_t offset;
} protocol_field_t;

#endif

