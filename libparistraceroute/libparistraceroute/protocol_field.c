#include "protocol_field.h"
#include <arpa/inet.h>

size_t protocol_field_get_size(protocol_field_t *pfield)
{
   return field_get_type_size(pfield->type);
}

int protocol_field_set(protocol_field_t *pfield, unsigned char *buffer, field_t *field)
{
    switch (pfield->type) {
        case TYPE_INT8:
            *(uint8_t*)(buffer + pfield->offset) = field->int8_value;
            break;
        case TYPE_INT16:
            *(uint16_t*)(buffer + pfield->offset) = htons(field->int16_value);
            break;
        case TYPE_INT32:
            *(uint32_t*)(buffer + pfield->offset) = htonl(field->int32_value);
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
