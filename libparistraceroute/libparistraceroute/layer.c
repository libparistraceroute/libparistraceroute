#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <errno.h>

#include "layer.h"
#include "common.h"

layer_t * layer_create() {
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

    if (field->type == TYPE_GENERATOR) {
        fprintf(stderr, "layer_set_field: invalid field\n");
        goto ERR_INVALID_FIELD;
    }

    if (!layer->protocol) {
        goto ERR_IN_PAYLOAD;
    }

    if (!(protocol_field = protocol_get_field(layer->protocol, field->key))) {
        goto ERR_FIELD_NOT_FOUND;
    }

    if (protocol_field->type != field->type) {
        fprintf(stderr, "'%s' field has not the right type (%s instead of %s)\n",
            field->key,
            field_type_to_string(field->type),
            field_type_to_string(protocol_field->type)
        );
        goto ERR_INVALID_FIELD_TYPE;
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
    // If we have a setter function, use it ; otherwise write it by using the generic function
    if ((protocol_field->set && !protocol_field->set(layer->segment, field)) 
    || (!protocol_field->set && !protocol_field_set(protocol_field, layer->segment, field))
    ){
        fprintf(stderr, "layer_set_field: can't set field '%s' (layer %s)\n", field->key, layer->protocol->name);
        goto ERR_PROTOCOL_FIELD_SET;
    }

    // TODO update segment of mask here

    return true;

ERR_PROTOCOL_FIELD_SET:
ERR_BUFFER_TOO_SMALL:
ERR_INVALID_FIELD_TYPE:
ERR_FIELD_NOT_FOUND:
ERR_IN_PAYLOAD:
ERR_INVALID_FIELD:
    return false;
}

bool layer_write_payload(layer_t * layer, const void * bytes, size_t num_bytes) { 
    return layer_write_payload_ext(layer, bytes, num_bytes, 0);
}

bool layer_write_payload_ext(layer_t * layer, const void * bytes, size_t num_bytes, size_t offset)
{
    if (layer->protocol) {
        // The layer embeds a nested layer
        fprintf(stderr, "Calling layer_write_payload_ext not for a payload\n");
        return false;
    }

    if (offset + num_bytes > layer->segment_size) {
        // The buffer allocated to this layer is too small
        fprintf(stderr, "Payload too small\n");
        return false;
    }

    memcpy(layer->segment + offset, bytes, num_bytes);
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
    layer_set_header_size(layer, protocol ? protocol->get_header_size(segment) : 0);
    return layer;

ERR_CREATE_LAYER:
    return NULL;
}

bool layer_extract(const layer_t * layer, const char * field_name, void * value) {
    const protocol_field_t * protocol_field;
    field_t                * field;

    if (!(layer && layer->protocol)) goto ERR_PARAM;
    if (!(protocol_field = protocol_get_field(layer->protocol, field_name))) goto ERR_PROTOCOL_GET_FIELD;

    // TODO this is crappy
    if (!(field = field_create_from_network(
            protocol_field->type,
            protocol_field->key,
            layer->segment + protocol_field->offset
        )
    )) goto ERR_FIELD_CREATE;

    memcpy(value, &field->value, field_get_size(field));
    field_free(field);
    return true;

ERR_PROTOCOL_GET_FIELD:
ERR_FIELD_CREATE:
ERR_PARAM:
    return false;
}

void layer_dump(const layer_t * layer, unsigned int indent)
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


void layer_debug(const layer_t * layer1, const layer_t * layer2, unsigned int indent) {
    protocol_field_t * protocol_field;
    field_t          * field1,
                     * field2;
    const char       * sep = "----------\n";

    if (!layer1->protocol) {
        layer_dump(layer1, indent);
    } else {
        print_indent(indent);
        printf("LAYER: %s\n", layer1->protocol->name);
        print_indent(indent);
        printf(sep);

        // Dump each (relevant) field
        for(protocol_field = layer1->protocol->fields; protocol_field->key; protocol_field++) {
            // Print field2 (if relevant)
            if (strcmp(protocol_field->key, "length")   == 0
            ||  strcmp(protocol_field->key, "checksum") == 0
            ||  strcmp(protocol_field->key, "protocol") == 0) {
                // Print caption
                print_indent(indent);
                printf("%-15s ", protocol_field->key);

                // Print field1
                field1 = layer_create_field(layer1, protocol_field->key);
                field_dump(field1);
                field_free(field1);

                // Print field2
                field2 = layer_create_field(layer2, protocol_field->key);
                printf("\t");
                field_dump(field2);
                field_free(field2);

                printf("\n");
            }
        }
    }
}
