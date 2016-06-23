#include "use.h"
#include "config.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <arpa/inet.h>

#include "field.h"
#include "generator.h"

#ifdef USE_BITS
#    include "bits.h"
#endif

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

#ifdef USE_BITS
// TODO factorize with field_create
field_t * field_create_bits(const char * key, const void * value, size_t offset_in_bits_in, size_t size_in_bits) {
    field_t * field;
    size_t    offset_in_bits_out;

    if (!(field = malloc(sizeof(field_t)))) goto ERR_MALLOC;
    field->key  = key;
    field->type = TYPE_BITS;
    memset(&field->value.bits, 0, sizeof(field->value.bits));

    offset_in_bits_out = 8 * sizeof(field->value.bits) - size_in_bits;
    if (!bits_write(&field->value.bits, offset_in_bits_out, value, offset_in_bits_in, size_in_bits)) {
        goto ERR_BITS_WRITE_BITS;
    }

    return field;

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
        case TYPE_BITS:
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
        case TYPE_BITS:
        case TYPE_UINT8:
            printf("%-10hhu", value->bits);
            break;
        case TYPE_UINT16:
            printf("%-10hu", value->int16);
            break;
        case TYPE_UINT32:
            printf("%-10u", value->int32);
            break;
        case TYPE_UINT64:
            printf("%llu", (unsigned long long)value->int64);
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
        case TYPE_GENERATOR :
            generator_dump(value->generator);
            break;
        case TYPE_UINT128:
        default:
            fprintf(stderr, "value_dump: type not supported (%d) (%s)", type, field_type_to_string(type));
            break;
    }
}

void value_dump_hex(const value_t * value, size_t num_bytes, size_t offset_in_bits, size_t num_bits) {
    size_t    i;
    const uint8_t * bytes = (const uint8_t *) value;

    if (num_bytes > 1 || num_bits >= 8 || num_bits == 0) {
        // >= 8 bits
        printf("0x");
        for (i = 0; i < num_bytes; ++i, ++bytes) {
            printf("%02x", *bytes);
            if (num_bytes % 8 == 0 && num_bytes > 1) printf(" ");
        }
#ifdef USE_BITS
    } else {
        // < 8 bits
        if (offset_in_bits % 4 == 0 && num_bits == 4) {
            // 4 bits right-aligned or left align
            printf("0x%01x", offset_in_bits ? (*bytes & 0xf0) >> 4 : (*bytes & 0x0f));
        } else {
            // Otherwise...
            for (i = 0; i < offset_in_bits; ++i) {
                printf(".");
            }
            for (; i < offset_in_bits + num_bits; ++i) {
                printf("%d", ((*bytes) & (1 << i)) ? 1 : 0);
            }
            for (; i < 8; ++i) {
                printf(".");
            }
        }
#endif
    }
}

void field_dump(const field_t * field) {
    printf("Field %s", field->key);
}

