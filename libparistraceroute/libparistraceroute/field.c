#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <arpa/inet.h>
#include "field.h"

field_t * field_create(fieldtype_t type, const char * key, const void * value) { 
    field_t * field;

    if (!(field = malloc(sizeof(field_t)))) goto ERR_MALLOC;
    if (!(field->key = strdup(key)))        goto ERR_STRDUP;
    field->type = type;

    if (type == TYPE_STRING) {
        if (!(field->value.string = strdup((const char *) value))) goto ERR_STRDUP2;
    } else {
        // Copy size bytes in value in the union.
        memcpy(&field->value, value, field_get_type_size(type));
    }
    return field;

ERR_STRDUP2:
    free(field->key);
ERR_STRDUP:
    free(field);
ERR_MALLOC:
    return NULL;
}

field_t * field_create_uint4(const char * key, uint8_t value) {
    return field_create(TYPE_UINT4, key, &value);
}

field_t * field_create_uint8(const char * key, uint8_t value) {
    return field_create(TYPE_UINT8, key, &value);
}

field_t * field_create_uint16(const char * key, uint16_t value) {
    return field_create(TYPE_UINT16, key, &value);
}

field_t * field_create_uint32(const char * key, uint32_t value) {
    return field_create(TYPE_UINT32, key, &value);
}

field_t * field_create_uint64(const char * key, uint64_t value) {
    return field_create(TYPE_UINT64, key, &value);
}

field_t * field_create_uint128(const char * key, uint128_t value) {
    return field_create(TYPE_UINT128, key, &value);
}

field_t * field_create_uintmax(const char * key, uintmax_t value) {
    return field_create(TYPE_UINTMAX, key, &value);
}

field_t * field_create_string(const char * key, const char * value) {
    return field_create(TYPE_STRING, key, value);
}

field_t * field_create_from_network(fieldtype_t type, const char * key, void * value)
{
    switch (type) {
        case TYPE_UINT4:
            return field_create_uint4(key, *(uint8_t *) value); 
        case TYPE_UINT8:
            return field_create_uint8(key, *(uint8_t *) value);
        case TYPE_UINT16:
            return field_create_uint16(key, ntohs(*(uint16_t *) value));
        case TYPE_UINT32:
            return field_create_uint32(key, ntohl(*(uint32_t *) value));
        case TYPE_UINT64:
            return field_create_uint64(key, ntohl(*(uint64_t *) value));
        case TYPE_UINT128:
            // return field_create_uint128(key, ntohl(*(uint128_t *) value));
            perror("field_create_from_network: Not yet implemented");
            return NULL;
        case TYPE_UINTMAX:
            return field_create_uintmax(key, ntohl(*(uintmax_t *) value));
        case TYPE_STRING:
            return field_create_string(key, (const char *) value);
        default:
            break;
    }
    return NULL;
}

void field_free(field_t * field)
{
    if (field) {
        if (field->key) free(field->key);
        if (field->type == TYPE_STRING) free(field->value.string); 
        free(field);
    }
}

// Accessors

size_t field_get_size(const field_t * field) {
    return field_get_type_size(field->type);
}

size_t field_get_type_size(fieldtype_t type) {
    switch (type) {
        case TYPE_UINT4:
            return sizeof(uint8_t);
        case TYPE_UINT8:
            return sizeof(uint8_t);
        case TYPE_UINT16:
            return sizeof(uint16_t);
        case TYPE_UINT32:
            return sizeof(uint32_t);
        case TYPE_UINT64:
            return sizeof(uint64_t);
        case TYPE_UINT128:
            return sizeof(uint128_t);
        case TYPE_UINTMAX:
            return sizeof(uintmax_t);
        case TYPE_STRING:
            return sizeof(char *);
        default:
            fprintf(stderr, "field_get_type_size: type not supported %d\n", type);
            break;
    }
    return 0;
}

// Dump

void field_dump(const field_t * field)
{
    switch (field->type) {
        case TYPE_UINT4:
            printf("%-10hhu (0x%1x)", field->value.int4, field->value.int4);
            break;
        case TYPE_UINT8:
            printf("%-10hhu (0x%02x)", field->value.int8, field->value.int8);
            break;
        case TYPE_UINT16:
            printf("%-10hu (0x%04x)", field->value.int16, field->value.int16);
            break;
        case TYPE_UINT32:
            printf("%-10u (0x%08x)", field->value.int32, field->value.int32);
            break;
        case TYPE_UINT64:
            printf("%lu", field->value.int64);
            break;
        case TYPE_UINT128:
            perror("Not yet implemented");
            //printf("%llu", field->value.int128); //TODO does printf work this way for 128 bit ints?
            break;
        case TYPE_UINTMAX:
            printf("%ju", field->value.intmax);
            break;
        case TYPE_STRING:
            printf("%s", field->value.string);
            break;
        default:
            break;
    }
}

