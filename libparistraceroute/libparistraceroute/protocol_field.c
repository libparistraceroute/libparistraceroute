#include "protocol_field.h"
#include <arpa/inet.h>
#include <stdio.h>

bool protocol_field_set(const protocol_field_t * protocol_field, uint8_t * buffer, const field_t * field)
{
    bool ret = true;

    switch (protocol_field->type) {
        case TYPE_UINT8:
            *(uint8_t *)(buffer + protocol_field->offset) = field->value.int8;
            break;
        case TYPE_UINT16:
            *(uint16_t *)(buffer + protocol_field->offset) = htons(field->value.int16);
            break;
        case TYPE_UINT32:
            *(uint32_t *)(buffer + protocol_field->offset) = htonl(field->value.int32);
            break;
        case TYPE_UINT4:
        case TYPE_STRING:
        default:
            fprintf(stderr, "protocol_field_set: Type not supported");
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
