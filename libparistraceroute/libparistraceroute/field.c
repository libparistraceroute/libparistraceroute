#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <arpa/inet.h>
#include "field.h"

field_t *field_create_int32(char *key, uint32_t value) {
    field_t * field = malloc(sizeof(field_t));

    if (field) {
        field->key = strdup(key);
        field->value.int32 = value;
        field->type = TYPE_INT32;
    }
    return field;
}

field_t *field_create_int16(char *key, uint16_t value) {
    field_t * field = malloc(sizeof(field_t));

    if (field) {
        field->key = strdup(key);
        field->value.int16 = value;
        field->type = TYPE_INT16;
    }
    return field;
}

field_t *field_create_int8(char *key, uint8_t value) {
    field_t * field = malloc(sizeof(field_t));

    if (field) {
        field->key = strdup(key);
        field->value.int8 = value;
        field->type = TYPE_INT8;
    }
    return field;
}

field_t *field_create_string(char *key, char *value)
{
    field_t * field = malloc(sizeof(field_t));

    if (field) {
        field->key = strdup(key);
        field->value.string = strdup(value);
        field->type = TYPE_STRING;
    }
    return field;
}

field_t *field_create(fieldtype_t type, char *key, void *value)
{
    switch (type) {
        case TYPE_INT8:
            return field_create_int8(key, *(uint8_t*)value);
        case TYPE_INT16:
            return field_create_int16(key, *(uint16_t*)value);
        case TYPE_INT32:
            return field_create_int32(key, *(uint32_t*)value);
        case TYPE_STRING:
            return field_create_string(key, (char *)value);
        case TYPE_INT4:
        default:
            break;
    }
    return 0;
}

field_t *field_create_from_network(fieldtype_t type, char *key, void *value)
{
    switch (type) {
        case TYPE_INT8:
            return field_create_int8(key, *(uint8_t*)value);
        case TYPE_INT16:
            return field_create_int16(key, ntohs(*(uint16_t*)value));
        case TYPE_INT32:
            return field_create_int32(key, ntohl(*(uint32_t*)value));
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
        case TYPE_INT4:
        case TYPE_STRING:
        default:
            break;
    }
    return 0;
}

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
        case TYPE_INT4:
            break;
        case TYPE_STRING:
            printf("%s", field->value.string);
            break;
        default:
            break;
    }
    
}
