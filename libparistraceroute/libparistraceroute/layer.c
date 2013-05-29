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
    layer->mask = NULL;
    return layer;

ERR_CALLOC:
    return NULL;
}

void layer_free(layer_t * layer) {
    if (layer) {
        free(layer);
    }
}

inline void layer_set_protocol(layer_t * layer, const protocol_t * protocol) {
    layer->protocol = protocol;
}

inline size_t layer_get_segment_size(const layer_t * layer) {
    return layer->segment_size;
}

inline void layer_set_segment_size(layer_t * layer, size_t segment_size) {
    layer->segment_size = segment_size;
}

inline size_t layer_get_header_size(const layer_t * layer) {
    return layer->header_size;
}

inline void layer_set_header_size(layer_t * layer, size_t header_size) {
    layer->header_size = header_size;
}

inline uint8_t * layer_get_segment(const layer_t * layer) {
    return layer->segment;
}

inline void layer_set_segment(layer_t * layer, uint8_t * segment) {
    layer->segment = segment;
}

inline uint8_t * layer_get_mask(const layer_t * layer) {
    return layer->mask;
}

inline void layer_set_mask(layer_t * layer, uint8_t * mask) {
    layer->mask = mask;
}

field_t * layer_create_field(const layer_t * layer, const char * name)
{
    const protocol_field_t * protocol_field;
    if (layer && layer->protocol) {
        if ((protocol_field = protocol_get_field(layer->protocol, name))) {
            if (protocol_field->get) {
                return protocol_field->get(layer->segment);
            } else {
                return field_create_from_network(
                    protocol_field->type,
                    name,
                    layer->segment + protocol_field->offset
                );
            }
        }
    }
    return NULL;
}

bool layer_set_field(layer_t * layer, const field_t * field)
{
    const protocol_field_t * protocol_field;
    size_t                   protocol_field_size;

    if (!field) {
        fprintf(stderr, "layer_set_field: invalid field\n");
        goto ERR_INVALID_FIELD;
    }

    if (!layer->protocol) {
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
        if (!(protocol_field->set(layer->segment, field))) {
            fprintf(stderr, "layer_set_field: can't set field '%s'\n", field->key);
            goto ERR_PROTOCOL_FIELD_SET;
        }
    } else {
        protocol_field_set(protocol_field, layer->segment, field);
    }

    // TODO update segment of mask here
    // TODO use protocol_field_get_offset
    // TODO use protocol_field_get_size

    return true;

ERR_PROTOCOL_FIELD_SET:
ERR_BUFFER_TOO_SMALL:
ERR_FIELD_NOT_FOUND:
ERR_IN_PAYLOAD:
ERR_INVALID_FIELD:
    return false;
}

bool layer_write_payload(layer_t * layer, buffer_t * payload) {
    return layer_write_payload_ext(layer, payload, 0);
}

bool layer_write_payload_ext(layer_t * layer, const buffer_t * payload, unsigned int offset)
{
    if (layer->protocol) {
        // The layer embeds a nested layer
        return false;
    }

    if (offset + buffer_get_size(payload) > layer->segment_size) {
        // The buffer allocated to this layer is too small
        return false;
    }

    memcpy(layer->segment + offset, buffer_get_data(payload), buffer_get_size(payload));
    return true;
}

layer_t * layer_create_from_segment(const protocol_t * protocol, uint8_t * segment, size_t segment_size) {
    layer_t * layer;

    // Create a new layer
    if (!(layer = layer_create())) {
        goto ERR_CREATE_LAYER;
    }

    // Initialize and install the new layer in the probe
    layer_set_segment(layer, segment);
    layer_set_segment_size(layer, segment_size);
    layer_set_protocol(layer, protocol);

    // TODO consider variable length headers
    layer_set_header_size(layer, protocol ? protocol->header_len : 0); // TODO manage header with variable length by querying a protocol's callback
    return layer;

ERR_CREATE_LAYER:
    return NULL;
}

bool layer_extract(const layer_t * layer, const char * field_name, void * value) {
    const protocol_field_t * protocol_field;
    bool  ret = false;

    if (layer && layer->protocol) {
        if ((protocol_field = protocol_get_field(layer->protocol, field_name))) {
            memcpy(
                value,
                layer->segment + protocol_field->offset,
                field_get_type_size(protocol_field->type)
            );
            ret = true;
        }
    }
    return ret;
}

void layer_dump(layer_t * layer, unsigned int indent)
{
    size_t             i, size;
    protocol_field_t * protocol_field;
    field_t          * field;
    const char       * sep = "----------\n";

    // There is no nested layer, so data carried by this layer is the payload
    if (!layer->protocol) {
        size = layer->segment_size; 
        print_indent(indent);
        printf("PAYLOAD:\n");
        print_indent(indent);
        printf(sep);
        print_indent(indent);
        printf("%-15s %lu\n", "size", size);
        print_indent(indent);
        printf("%-15s", "data");
        for (i = 0; i < size; i++) {
           printf(" %02x", layer->segment[i]);
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

