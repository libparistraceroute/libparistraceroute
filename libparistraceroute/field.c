#include "use.h"
#include "config.h"

#include <assert.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <arpa/inet.h>
#include <sys/socket.h>     // AF_INET*

#include "field.h"
#include "generator.h"

#include "bits.h"           // bits_fprintf

field_t * field_create(fieldtype_t type, const char * key, const void * value) {
    field_t * field;

    if (!(field = malloc(sizeof(field_t)))) goto ERR_MALLOC;
    field->key  = key;
    field->type = type;

    if (value) {
        switch (type) {
            case TYPE_STRING:
                if (!(field->value.string = strdup((const char *) value))) goto ERR_DUP_VALUE;
                break;
            case TYPE_GENERATOR:
                if (!(field->value.generator = generator_dup((const generator_t *) value))) goto ERR_DUP_VALUE;
                break;
#ifdef USE_BITS
            case TYPE_BITS:
                fprintf(stderr, "field_create: invalid field type (TYPE_BITS): use field_create_bits instead.\n");
                assert(false);
                break;
#endif
            default:
                // Copy size bytes in value in the union.
                memcpy(&field->value, value, field_get_type_size(type));
                break;
        }
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
#ifdef USE_BITS
            case TYPE_BITS:
                free(field->value.bits.bits);
                break;
#endif
            default:
                break;
        }

        free(field);
    }
}

field_t * field_create_address(const char * key, const address_t * address) {
    field_t * field = NULL;

    switch (address->family) {
#ifdef USE_IPV4
        case AF_INET:
            field = field_create(TYPE_IPV4, key, &address->ip.ipv4);
            break;
#endif
#ifdef USE_IPV6
        case AF_INET6:
            field = field_create(TYPE_IPV6, key, &address->ip.ipv6);
            break;
#endif
        default:
            fprintf(stderr, "field_create_address: Invalid family address (family = %d)\n", address->family);
            break;
    }

    return field;
}

#ifdef USE_IPV4
field_t * field_create_ipv4(const char * key, ipv4_t ipv4) {
    return field_create(TYPE_IPV4, key, &ipv4);
}
#endif

#ifdef USE_IPV6
field_t * field_create_ipv6(const char * key, ipv6_t ipv6) {
    return field_create(TYPE_IPV6, key, &ipv6);
}
#endif

#ifdef USE_BITS
field_t * field_create_bits(const char * key, const void * value, size_t offset_in_bits_in, size_t size_in_bits) {
    size_t    num_bytes;
    field_t * field;

    if (!(field = malloc(sizeof(field_t)))) goto ERR_MALLOC;
    field->key  = key;
    field->type = TYPE_BITS;

    num_bytes = (size_in_bits / 8) + (size_in_bits % 8 ? 1 : 0);
    if (!(field->value.bits.bits = malloc(num_bytes))) goto ERR_MALLOC2;
    field->value.bits.size_in_bits = size_in_bits;
    field->value.bits.offset_in_bits = (8 * num_bytes - size_in_bits);
    memset(field->value.bits.bits, 0, num_bytes);

    if (!bits_write(field->value.bits.bits, field->value.bits.offset_in_bits, value, offset_in_bits_in, size_in_bits)) {
        goto ERR_BITS_WRITE_BITS;
    }

    return field;

ERR_MALLOC2:
ERR_BITS_WRITE_BITS:
    free(field);
ERR_MALLOC:
    return NULL;
}
#endif

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

bool field_set_value(field_t * field, void * value)
{
    bool ret = false;

    if (field && value) {
#ifdef USE_BITS
        switch (field->type) {
            case TYPE_BITS:
                ret = bits_write(
                    field->value.bits.bits,
                    field->value.bits.offset_in_bits,
                    value,
                    0,
                    field->value.bits.size_in_bits
                );
                break;
            default:
#endif
                memcpy(&field->value, value, field_get_size(field));
                ret = true;
#ifdef USE_BITS
                break;
        }
#endif
    }
    return ret;
}

// Accessors

size_t field_get_size(const field_t * field) {
#ifdef USE_BITS
    if (field->type == TYPE_BITS) {
        fprintf(stderr, "field_get_size: invalid type TYPE_BITS: use field_get_size_in_bits instead\n");
        assert(false);
    }
#endif
    return field_get_type_size(field->type);
}

size_t field_get_size_in_bits(const field_t * field) {
    return field->type == TYPE_BITS ?
        field->value.bits.size_in_bits :
        8 * field_get_size(field);
}

size_t field_get_type_size(fieldtype_t type) {
    switch (type) {
#ifdef USE_IPV4
        case TYPE_IPV4:
            return sizeof(ipv4_t);
#endif
#ifdef USE_IPV6
        case TYPE_IPV6:
            return sizeof(ipv6_t);
#endif
#ifdef USE_BITS
        case TYPE_BITS:
            fprintf(stderr, "field_get_type_size: invalid type TYPE_BITS: use field_get_size_in_bits instead\n");
            assert(false);
            break;
#endif
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
#ifdef USE_IPV4
        case TYPE_IPV4:      return "ipv4";
#endif
#ifdef USE_IPV6
        case TYPE_IPV6:      return "ipv6";
#endif
        case TYPE_BITS:      return "bits";
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

void value_dump(const value_t * value, fieldtype_t type) {
    switch (type) {
#ifdef USE_IPV4
        case TYPE_IPV4:
            ipv4_dump(&value->ipv4);
            break;
#endif
#ifdef USE_IPV6
        case TYPE_IPV6:
            ipv6_dump(&value->ipv6);
            break;
#endif
#ifdef USE_BITS
        case TYPE_BITS:
            fprintf(stderr, "value_dump: type not supported (%d) (%s), use value_dump_hex instead\n", type, field_type_to_string(type));
            break;
#endif
        case TYPE_UINT8:
            printf("%hhu", value->int8);
            break;
        case TYPE_UINT16:
            printf("%hu", value->int16);
            break;
        case TYPE_UINT32:
            printf("%u", value->int32);
            break;
        case TYPE_UINT64:
            printf("%llu", (unsigned long long) value->int64);
            break;
        case TYPE_DOUBLE:
            printf("%lf", value->dbl);
            break;
        case TYPE_UINTMAX:
            printf("%ju", value->intmax);
            break;
        case TYPE_STRING:
            printf("%s", value->string);
            break;
        case TYPE_GENERATOR:
            generator_dump(value->generator);
            break;
        case TYPE_UINT128:
        default:
            fprintf(stderr, "value_dump: type not supported (%d) (%s)\n", type, field_type_to_string(type));
            break;
    }
}

void value_dump_hex(const value_t * value, size_t num_bytes, size_t offset_in_bits, size_t num_bits) {
    const uint8_t * bytes = (const uint8_t *) value;
    bits_fprintf(stdout, bytes, num_bytes << 3, offset_in_bits);
}

void field_dump(const field_t * field) {
    printf("Field %s", field->key);
}

