#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <errno.h>

#include "layer.h"
#include "common.h"

layer_t * layer_create(void) {
    layer_t * layer = calloc(1, sizeof(layer_t));
    if (!layer) goto ERR_CALLOC; 
//    layer->mask = NULL;
    return layer;

ERR_CALLOC:
    return NULL;
}

void layer_free(layer_t * layer) {
    if (layer) {
//        printf("layer->mask = %x\n", layer->mask);
//        if (layer->mask)   free(layer->mask);
        free(layer);
    }
}

inline void layer_set_protocol(layer_t * layer, const protocol_t * protocol) {
    layer->protocol = protocol;
}

inline void layer_set_buffer_size(layer_t * layer, size_t buffer_size) {
    layer->buffer_size = buffer_size;
    // TODO resize mask
}

inline size_t layer_get_buffer_size(const layer_t * layer) {
    return layer->buffer_size;
}

inline void layer_set_header_size(layer_t * layer, size_t header_size) {
    layer->header_size = header_size;
}

inline unsigned char * layer_get_buffer(layer_t * layer) {
    return layer->buffer;
}

inline void layer_set_buffer(layer_t * layer, uint8_t * buffer) {
    layer->buffer = buffer;
}

/*
inline void layer_set_mask(layer_t * layer, uint8_t * mask) {
    layer->mask = mask;
}
*/

field_t * layer_create_field(const layer_t * layer, const char * name)
{
    const protocol_field_t * protocol_field;
    if (layer && layer->protocol) {
        if ((protocol_field = protocol_get_field(layer->protocol, name))) {
            if (protocol_field->get) {
                return protocol_field->get(layer->buffer);
            } else {
                return field_create_from_network(
                    protocol_field->type,
                    name,
                    layer->buffer + protocol_field->offset
                );
            }
        }
    }
    return NULL;
}

bool layer_set_field(layer_t * layer, field_t * field)
{
    protocol_field_t * protocol_field;
    size_t             protocol_field_size;

    if (!field) {
        fprintf(stderr, "layer_set_field: invalid field\n");
        goto ERR_INVALID_FIELD;
    }
    if (!layer->protocol) {
        fprintf(stderr, "layer_set_field: trying to set '%s' field, but we're altering the payload\n", field->key); 
        goto ERR_IN_PAYLOAD;
    }

    if (!(protocol_field = protocol_get_field(layer->protocol, field->key))) {
        goto ERR_FIELD_NOT_FOUND;
    }

    // Check whether the probe buffer can store this field
    // NOTE: the allocation of the buffer might be tricky for headers with
    // variable len (such as IPv4 with options, etc.).
    protocol_field_size = field_get_type_size(protocol_field->type);
    if (protocol_field->offset + protocol_field_size > layer->header_size) {
        fprintf(stderr, "layer_set_field: buffer too small\n"); 
        goto ERR_BUFFER_TOO_SMALL;
    }

    // Copy the field value into the buffer 
    // If we have a setter function, we use it, otherwise write the value directly
    if (protocol_field->set) {
        protocol_field->set(layer->buffer, field);
    } else {
        protocol_field_set(protocol_field, layer->buffer, field);
    }

    // TODO update mask here
    // TODO use protocol_field_get_offset
    // TODO use protocol_field_get_size

    return true;

ERR_BUFFER_TOO_SMALL:
ERR_FIELD_NOT_FOUND:
ERR_IN_PAYLOAD:
ERR_INVALID_FIELD:
    return false;
}

bool layer_set_payload(layer_t * layer, buffer_t * payload) {
    return layer_write_payload(layer, payload, 0);
}

bool layer_write_payload(layer_t * layer, const buffer_t * payload, unsigned int offset)
{
    if (layer->protocol) {
        // The layer embeds a nested layer
        return false;
    }

    if (offset + buffer_get_size(payload) > layer->buffer_size) {
        // The buffer allocated to this layer is too small
        return false;
    }

    memcpy(layer->buffer + offset, buffer_get_data(payload), buffer_get_size(payload));
    // TODO update mask
    return true;
}

void layer_dump(layer_t * layer, unsigned int indent)
{
    size_t             i, size;
    protocol_field_t * protocol_field;
    field_t          * field;
    const char       * sep = "----------\n";

    // There is no nested layer, so data carried by this layer is the payload
    if (!layer->protocol) {
        size = layer->buffer_size; 
        print_indent(indent);
        printf("PAYLOAD:\n");
        print_indent(indent);
        printf(sep);
        print_indent(indent);
        printf("%-15s %lu\n", "size", size);
        print_indent(indent);
        printf("%-15s", "data");
        for (i = 0; i < size; i++) {
           printf(" %02x", layer->buffer[i]);
        }
        printf("\n");
    } else {
        print_indent(indent);
        printf("LAYER: %s\n", layer->protocol->name);
        print_indent(indent);
        printf(sep);
        
        // Dump each field
        for(protocol_field = layer->protocol->fields; protocol_field->key; protocol_field++) {
            field = layer_create_field(layer, protocol_field->key);
            print_indent(indent);
            printf("%-15s ", protocol_field->key);
            field_dump(field);    
            printf("\n");
            field_free(field);
        }
    }
}

