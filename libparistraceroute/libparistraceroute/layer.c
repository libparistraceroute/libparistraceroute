#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <errno.h>

#include "layer.h"
#include "common.h"

layer_t * layer_create(void)
{
    layer_t * layer = calloc(1, sizeof(layer_t));
    if (!layer) errno = ENOMEM;
    // TODO alloc mask
    return layer;
}

void layer_free(layer_t * layer)
{
    if (layer) {
        if (layer->mask)   free(layer->mask);
        if (layer->buffer) free(layer->buffer);
        free(layer);
    }
}

inline void layer_set_protocol(layer_t * layer, protocol_t * protocol)
{
    layer->protocol = protocol;
}

inline void layer_set_buffer_size(layer_t * layer, size_t buffer_size)
{
    layer->buffer_size = buffer_size;
}

inline size_t layer_get_buffer_size(const layer_t * layer)
{
    return layer->buffer_size;
}

inline void layer_set_header_size(layer_t * layer, size_t header_size)
{
    layer->header_size = header_size;
}

inline unsigned char * layer_get_buffer(layer_t * layer)
{
    return layer->buffer;
}

inline void layer_set_buffer(layer_t * layer, uint8_t * buffer)
{
    layer->buffer = buffer;
}

inline void layer_set_mask(layer_t * layer, uint8_t * mask)
{
    layer->mask = mask;
}

const field_t * layer_get_field(const layer_t * layer, const char * name)
{
    const protocol_field_t * field;
    if (layer && layer->protocol) {
        if ((field = protocol_get_field(layer->protocol, name))) {
            if (field->get) {
                return field->get(layer->buffer);
            } else {
                // TODO This function either can allocate or points to existing data !!!!
                return field_create_from_network(
                    field->type,
                    name,
                    layer->buffer + field->offset
                );
            }
        }
    }
    return NULL;
}

int layer_set_field(layer_t * layer, field_t * field)
{
    protocol_field_t * protocol_field;
    size_t             protocol_field_size;

    if (!field) return -1;
    if (!layer->protocol){
        // We're in the payload! 
        return -1;
    }

    protocol_field = protocol_get_field(layer->protocol, field->key);
    if (!protocol_field) {
        // Protocol field not found
        return -1;
    }

    // Check whether the probe buffer can store this field
    // NOTE: the allocation of the buffer might be tricky for headers with
    // variable len (such as IPv4 with options, etc.).
    protocol_field_size = field_get_type_size(protocol_field->type);
    if (protocol_field->offset + protocol_field_size > layer->header_size) {
        // The buffer allocated to this layer is too small
        return -2;
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

    return 0;
}

bool layer_set_payload(layer_t * layer, buffer_t * payload)
{
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
    return true;
}

void layer_dump(layer_t *layer, unsigned int indent)
{
    protocol_field_t *pfield;
    print_indent(indent);

    // There is no nested layer, so data carried by this layer is the payload
    if (!layer->protocol) {
        printf("PAYLOAD size = %lu\n", layer->buffer_size);
    } else {
        printf("LAYER: %s\n", layer->protocol->name);
        print_indent(indent);
        printf("----------\n");
        
        // Dump each field
        for(pfield = layer->protocol->fields; pfield->key; pfield++) {
            const field_t * field = layer_get_field(layer, pfield->key);
            print_indent(indent);
            printf("%s\t", pfield->key);
            field_dump(field);    
            printf("\n");
        }
    }
}
