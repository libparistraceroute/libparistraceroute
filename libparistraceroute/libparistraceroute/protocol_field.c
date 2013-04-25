#include "protocol_field.h"
#include <arpa/inet.h>


int protocol_field_set(protocol_field_t * protocol_field, uint8_t * buffer, field_t * field)
{
    switch (protocol_field->type) {
        case TYPE_INT8:
            *(uint8_t *)(buffer + protocol_field->offset) = field->value.int8;
            break;
        case TYPE_INT16:
            *(uint16_t *)(buffer + protocol_field->offset) = htons(field->value.int16);
            break;
        case TYPE_INT32:
            *(uint32_t *)(buffer + protocol_field->offset) = htonl(field->value.int32);
            break;
        case TYPE_INT4:
            break;
        case TYPE_STRING:
            break;
        default:
            break;
    }
    return 0;
}

inline size_t protocol_field_get_offset(const protocol_field_t * protocol_field)
{
    return protocol_field->offset;
}

inline size_t protocol_field_get_size(const protocol_field_t * protocol_field)
{
   return field_get_type_size(protocol_field->type);
}
