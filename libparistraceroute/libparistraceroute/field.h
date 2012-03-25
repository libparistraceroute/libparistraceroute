#ifndef FIELD_H
#define FIELD_H

#include <stdint.h>

typedef enum {
    TYPE_INT16,
    TYPE_INT32,
    TYPE_STRING
} fieldtype_t;

typedef struct {
    char *key;
    union {
        void *value;
        uint16_t int16_value;
        uint32_t int32_value;
        char *string_value;
    };
    fieldtype_t type;
} field_t;

field_t *field_create_int16(char *key, uint16_t value);
field_t *field_create_int32(char *key, uint32_t value);
field_t *field_create_string(char *key, char *value);
void field_free(field_t *field);

#define I32(x, y) field_create_int(x, y)
#define STR(x, y) field_create_string(x, y)

#endif
