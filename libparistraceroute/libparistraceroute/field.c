#include <stdlib.h>
#include <string.h>
#include "field.h"

field_t *field_create_int32(char *key, uint32_t value) {
    field_t *field;

    field = (field_t*)malloc(sizeof(field_t));

    field->key = strdup(key);
    field->int32_value = value;
    field->type = TYPE_INT32;

    return field;
}

field_t *field_create_int16(char *key, uint16_t value) {
    field_t *field;

    field = (field_t*)malloc(sizeof(field_t));

    field->key = strdup(key);
    field->int16_value = value;
    field->type = TYPE_INT16;

    return field;
}

field_t *field_create_int8(char *key, uint8_t value) {
    field_t *field;

    field = (field_t*)malloc(sizeof(field_t));

    field->key = strdup(key);
    field->int16_value = value;
    field->type = TYPE_INT8;

    return field;
}

field_t *field_create_string(char *key, char *value) {
    field_t *field;

    field = (field_t*)malloc(sizeof(field_t));

    field->key = strdup(key);
    field->string_value = strdup(value);
    field->type = TYPE_STRING;

    return field;
}

void field_free(field_t *field)
{
    free(field->key);
    free(field);
}

size_t field_get_type_size(fieldtype_t type)
{
    switch (type) {
        case TYPE_INT8:
            return sizeof(uint8_t);
        case TYPE_INT16:
            return sizeof(uint16_t);
        case TYPE_INT32:
            return sizeof(uint32_t);
        case TYPE_INT4:
        case TYPE_STRING:
        default:
            perror("field_get_type_size: invalid parameter");
            break;
    }
    return 0;
}
