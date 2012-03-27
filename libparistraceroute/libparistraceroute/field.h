#ifndef FIELD_H
#define FIELD_H

#include <stdint.h>

typedef enum {
    TYPE_INT4,
    TYPE_INT8,
    TYPE_INT16,
    TYPE_INT32,
    TYPE_STRING
} fieldtype_t;

typedef struct {
    char *key;
    union {
        void       * value;
        unsigned int int4_value:4; 
        uint8_t      int8_value;
        uint16_t     int16_value;
        uint32_t     int32_value;
        char       * string_value;
    };
    fieldtype_t type;
} field_t;

field_t * field_create_int8  (char * key, uint8_t  value);
field_t * field_create_int16 (char * key, uint16_t value);
field_t * field_create_int32 (char * key, uint32_t value);
field_t * field_create_string(char * key, char   * value);

void      field_free(field_t *field);

#define I4(x, y) field_create_int4  (x, y)
#define I8(x, y) field_create_int8  (x, y)
#define I16(x, y) field_create_int16 (x, y)
#define I32(x, y) field_create_int32 (x, y)
#define STR(x, y) field_create_string(x, y)

#endif
