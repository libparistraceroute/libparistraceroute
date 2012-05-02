#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <arpa/inet.h>
#include "field.h"

field_t *field_create_int8(const char *key, uint8_t value) {
    field_t * field = malloc(sizeof(field_t));

    if (field) {
        field->key = strdup(key);
        field->value.int8 = value;
        field->type = TYPE_INT8;
    }
    return field;
}

field_t *field_create_int16(const char *key, uint16_t value) {
    field_t * field = malloc(sizeof(field_t));

    if (field) {
        field->key = strdup(key);
        field->value.int16 = value;
        field->type = TYPE_INT16;
    }
    return field;
}

field_t *field_create_int32(const char *key, uint32_t value) {
    field_t * field = malloc(sizeof(field_t));

    if (field) {
        field->key = strdup(key);
        field->value.int32 = value;
        field->type = TYPE_INT32;
    }
    return field;
}

field_t *field_create_int64(const char *key, uint64_t value) {
    field_t * field = malloc(sizeof(field_t));

    if (field) {
        field->key = strdup(key);
        field->value.int64 = value;
        field->type = TYPE_INT64;
    }
    return field;
}

field_t *field_create_intmax(const char *key, uintmax_t value) {
    field_t * field = malloc(sizeof(field_t));

    if (field) {
        field->key = strdup(key);
        field->value.intmax = value;
        field->type = TYPE_INTMAX;
    }
    return field;
}

field_t *field_create_string(const char *key, const char *value)
{
    field_t * field = malloc(sizeof(field_t));

    if (field) {
        field->key = strdup(key);
        field->value.string = strdup(value);
        field->type = TYPE_STRING;
    }
    return field;
}

field_t *field_create(fieldtype_t type, const char *key, void *value)
{
    switch (type) {
        case TYPE_INT8:
            return field_create_int8(key, *(uint8_t*)value);
        case TYPE_INT16:
            return field_create_int16(key, *(uint16_t*)value);
        case TYPE_INT32:
            return field_create_int32(key, *(uint32_t*)value);
        case TYPE_INT64:
            return field_create_int64(key, *(uint64_t*)value);
        case TYPE_INTMAX:
            return field_create_intmax(key, *(uintmax_t*)value);
        case TYPE_STRING:
            return field_create_string(key, (char *)value);
        case TYPE_INT4:
        default:
            break;
    }
    return 0;
}

field_t *field_create_from_network(fieldtype_t type, const char *key, void *value)
{
    switch (type) {
        case TYPE_INT8:
            return field_create_int8(key, *(uint8_t*)value);
        case TYPE_INT16:
            return field_create_int16(key, ntohs(*(uint16_t*)value));
        case TYPE_INT32:
            return field_create_int32(key, ntohl(*(uint32_t*)value));
        case TYPE_INT64:
            return field_create_int64(key, ntohl(*(uint64_t*)value));
        case TYPE_INTMAX:
            return field_create_intmax(key, ntohl(*(uintmax_t*)value));
        case TYPE_STRING:
            return field_create_string(key, (char *)value);
        case TYPE_INT4:
        default:
            break;
    }
    return 0;
}

void field_free(field_t *field)
{
    if(field) {
        if(field->key) free(field->key);
        free(field);
    }
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
        case TYPE_INT64:
            return sizeof(uint64_t);
        case TYPE_INTMAX:
            return sizeof(uintmax_t);
        case TYPE_INT4:
        case TYPE_STRING:
        default:
            break;
    }
    return 0;
}

// Comparison

int field_compare(field_t *field1, field_t *field2)
{
    if (field1->type != field2->type)
        return -2; // Field are not of the same type !!

    switch (field1->type) {
        case TYPE_INT8:
            return field1->value.int8 - field2->value.int8;
        case TYPE_INT16:
            return field1->value.int16 - field2->value.int16;
        case TYPE_INT32:
            return field1->value.int32 - field2->value.int32;
        case TYPE_INT64:
            return field1->value.int64 - field2->value.int64;
        case TYPE_INTMAX:
            return field1->value.intmax - field2->value.intmax;
        case TYPE_INT4:
            return field1->value.int4 - field2->value.int4;
        case TYPE_STRING:
            return strcmp(field1->value.string, field2->value.string);
        default:
            break;
    }
    return -3; // Unknown comparison
    
}

// Accessors

size_t field_get_size(field_t *field)
{
    return field_get_type_size(field->type);
}

// Dump
void field_dump(field_t *field)
{
    switch (field->type) {
        case TYPE_INT8:
            printf("%hhu", field->value.int8);
            break;
        case TYPE_INT16:
            printf("%hu", field->value.int16);
            break;
        case TYPE_INT32:
            printf("%u", field->value.int32);
            break;
        case TYPE_INT64:
            printf("%lu", field->value.int64);
            break;
        case TYPE_INTMAX:
            printf("%ju", field->value.intmax);
            break;
        case TYPE_INT4:
            break;
        case TYPE_STRING:
            printf("%s", field->value.string);
            break;
        default:
            break;
    }
    
}
