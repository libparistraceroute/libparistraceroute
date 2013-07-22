#include "config.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <arpa/inet.h>

#include "field.h"
#include "generator.h"

field_t * field_create(fieldtype_t type, const char * key, const void * value) {
    field_t * field;

    if (!(field = malloc(sizeof(field_t)))) goto ERR_MALLOC;
    field->key = key;
    field->type = type;

    switch (type) {
        case TYPE_STRING:
            if (!(field->value.string = strdup((const char *) value))) goto ERR_DUP_VALUE;
            break;
        case TYPE_GENERATOR:
            if (!(field->value.generator = generator_dup((const generator_t *) value))) goto ERR_DUP_VALUE;
            break;
        default:
            // Copy size bytes in value in the union.
            memcpy(&field->value, value, field_get_type_size(type));
            break;
    }

    return field;

ERR_DUP_VALUE:
    free(field);
ERR_MALLOC:
    return NULL;
}

void field_free(field_t * field)
{
    if (field) {
        switch (field->type) {
            case TYPE_STRING:
                free(field->value.string);
                break;
            case TYPE_GENERATOR:
                generator_free(field->value.generator);
                break;
            default:
                break;
        }

        free(field);
    }
}

field_t * field_create_address(const char * key, const address_t * address) {
    field_t * field = NULL;

    switch (address->family) {
        case AF_INET:
            field = field_create(TYPE_IPV4, key, &address->ip.ipv4);
            break;
        case AF_INET6:
            field =  field_create(TYPE_IPV6, key, &address->ip.ipv6);
            break;
        default:
            fprintf(stderr, "field_create_address: Invalid family address (family = %d)", address->family);
            break;
    }

    return field;
}

field_t * field_create_ipv4(const char * key, ipv4_t ipv4) {
    return field_create(TYPE_IPV4, key, &ipv4);
}

field_t * field_create_ipv6(const char * key, ipv6_t ipv6) {
    return field_create(TYPE_IPV6, key, &ipv6);
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

field_t * field_create_double(const char * key, double value) {
    return field_create(TYPE_DOUBLE, key, &value);
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

field_t * field_create_generator(const char * key, struct generator_s * value) {
    return field_create(TYPE_GENERATOR, key, value);
}

field_t * field_dup(const field_t * field) {
    const char * key_dup;

    if (!(key_dup = strdup(field->key))) goto ERR_STRDUP;

    // Note: if the field stores a pointer (TYPE_STRING, TYPE_GENERATOR),
    // we only copies this pointer. Thus the both fields points to the
    // same object.

    return field_create(field->type, key_dup, &field->value);

ERR_STRDUP:
    return NULL;
}

field_t * field_create_from_network(fieldtype_t type, const char * key, void * value)
{
    switch (type) {
        case TYPE_IPV4:
            return field_create_ipv4(key, *(ipv4_t *) value);
        case TYPE_IPV6:
            return field_create_ipv6(key, *(ipv6_t *) value);
        case TYPE_UINT4:
            return field_create_uint4(key, *(uint8_t *) value);
        case TYPE_UINT8:
            return field_create_uint8(key, *(uint8_t *) value);
        case TYPE_UINT16:
            return field_create_uint16(key, ntohs(*(uint16_t *) value));
        case TYPE_UINT32:
            return field_create_uint32(key, ntohl(*(uint32_t *) value));
        case TYPE_UINT64:
        case TYPE_UINT128:
        case TYPE_UINTMAX:
            perror("field_create_from_network: Not yet implemented");
            return NULL;
        case TYPE_STRING:
            return field_create_string(key, (const char *) value);
        default:
            break;
    }
    return NULL;
}

bool field_set_value(field_t * field, void * value)
{
    bool ret = false;

    if (field && value) {
        memcpy(&field->value, value, field_get_size(field));
        ret = true;
    }
    return ret;
}

// Accessors

size_t field_get_size(const field_t * field) {
    return field_get_type_size(field->type);
}

size_t field_get_type_size(fieldtype_t type) {
    switch (type) {
        case TYPE_IPV4:
            return sizeof(ipv4_t);
        case TYPE_IPV6:
            return sizeof(ipv6_t);
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
        case TYPE_DOUBLE:
            return sizeof(double);
        case TYPE_GENERATOR:
            return sizeof(struct generator_s);
        case TYPE_STRING:
            return sizeof(char *);
        default:
            fprintf(stderr, "field_get_type_size: type not supported %d\n", type);
            break;
    }
    return 0;
}

const char * field_get_key(field_t * field) {
    return field->key;
}

// Dump

bool field_match(const field_t * field1, const field_t * field2) {
    return field1 && field2 && field1->type == field2->type && strcmp(field1->key, field2->key) == 0;
}

const char * field_type_to_string(fieldtype_t type) {
    switch (type) {
        case TYPE_IPV4:      return "ipv4"; 
        case TYPE_IPV6:      return "ipv6"; 
        case TYPE_UINT4:     return "uint4";
        case TYPE_UINT8:     return "uint8";
        case TYPE_UINT16:    return "uint16";
        case TYPE_UINT32:    return "uint32";
        case TYPE_UINT64:    return "uint64";
        case TYPE_UINT128:   return "uint128";
        case TYPE_UINTMAX:   return "uintmax";
        case TYPE_DOUBLE:    return "double";
        case TYPE_STRING:    return "string";
        case TYPE_GENERATOR: return "generator";
        default:             break;
    }
    return "???";
}

void field_dump(const field_t * field)
{
    if (field) {
        switch (field->type) {
            case TYPE_IPV4:
                ipv4_dump(&field->value.ipv4);
                printf("\t(0x%08x)",  ntohl(*((const uint32_t *) &field->value.ipv4)));
                break;
            case TYPE_IPV6:
                ipv6_dump(&field->value.ipv6);
                printf("\t(0x%08x %08x %08x %08x)",
                    ntohl(((const uint32_t *) &field->value.ipv6)[0]),
                    ntohl(((const uint32_t *) &field->value.ipv6)[1]),
                    ntohl(((const uint32_t *) &field->value.ipv6)[2]),
                    ntohl(((const uint32_t *) &field->value.ipv6)[3])
                );
                break;
            case TYPE_UINT4:
                printf("%-10hhu\t(0x%1x)", field->value.int4, field->value.int4);
                break;
            case TYPE_UINT8:
                printf("%-10hhu\t(0x%02x)", field->value.int8, field->value.int8);
                break;
            case TYPE_UINT16:
                printf("%-10hu\t(0x%04x)", field->value.int16, field->value.int16);
                break;
            case TYPE_UINT32:
                printf("%-10u\t(0x%08x)", field->value.int32, field->value.int32);
                break;
            case TYPE_UINT64:
                printf("%lu", field->value.int64);
                break;
            case TYPE_DOUBLE:
                printf("%lf", field->value.dbl);
                break;
            case TYPE_UINTMAX:
                printf("%ju", field->value.intmax);
                break;
            case TYPE_STRING:
                printf("%s", field->value.string);
                break;
            case TYPE_GENERATOR :
                generator_dump(field->value.generator);
                break;
            case TYPE_UINT128:
            default:
                fprintf(stderr, "field_dump: type not supported (%d) (%s)", field->type, field_type_to_string(field->type));
                break;
        }
    }
}

