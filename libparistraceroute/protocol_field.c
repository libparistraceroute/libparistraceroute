#include "use.h"              // USE_*
#include "config.h"

#include <string.h>           // memcpy
#include <stdio.h>            // fprintf

#include "protocol_field.h"
#ifdef USE_BITS
#    include "bits.h"         // bits_write
#endif

bool protocol_field_set(const protocol_field_t * protocol_field, uint8_t * segment, const field_t * field)
{
    bool      ret = true;
    uint8_t * segment_field = segment + protocol_field->offset;

    switch (protocol_field->type) {
#ifdef USE_IPV4
        case TYPE_IPV4:
            memcpy(segment_field, &field->value.ipv4, sizeof(ipv4_t));
            break;
#endif
#ifdef USE_IPV6
        case TYPE_IPV6:
            memcpy(segment_field, &field->value.ipv6, sizeof(ipv6_t));
            break;
#endif
        case TYPE_UINT8:
            *(uint8_t *) segment_field = field->value.int8;
            break;
        case TYPE_UINT16:
            *(uint16_t *) segment_field = htons(field->value.int16);
            break;
        case TYPE_UINT32:
            *(uint32_t *) segment_field = htonl(field->value.int32);
            break;
#ifdef USE_BITS
        case TYPE_BITS:
            /*
            ret = bits_write(
                segment_field,
                protocol_field->offset_in_bits,
                &field->value.bits,
                8 * sizeof(field->value.bits) - protocol_field->size_in_bits,
                protocol_field->size_in_bits
            );
            */
            ret = bits_write(
                segment_field,
                protocol_field->offset_in_bits,
                field->value.bits.bits,
                field->value.bits.offset_in_bits,
                protocol_field->size_in_bits
            );

            break;
#endif
        case TYPE_STRING:
        default:
            fprintf(stderr, "protocol_field_set: Type not supported\n");
            ret = false;
            break;
    }
    return ret;
}

inline size_t protocol_field_get_offset(const protocol_field_t * protocol_field) {
    return protocol_field->offset;
}

inline size_t protocol_field_get_size(const protocol_field_t * protocol_field) {
    return field_get_type_size(protocol_field->type);
}

inline size_t protocol_field_get_size_in_bits(const protocol_field_t * protocol_field) {
    size_t ret;
#ifdef USE_BITS
    switch (protocol_field->type) {
        case TYPE_BITS:
            ret = protocol_field->size_in_bits;
            break;
        default:
#endif
            ret = 8 * field_get_type_size(protocol_field->type);
#ifdef USE_BITS
            break;
    }
#endif
    return ret;
}

void protocol_field_dump(const protocol_field_t * protocol_field) {
    printf("> %s\t%s\n",
        field_type_to_string(protocol_field->type),
        protocol_field->key
    );
}

