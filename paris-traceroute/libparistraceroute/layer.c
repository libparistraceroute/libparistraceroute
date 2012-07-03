#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include "layer.h"
#include "common.h"

layer_t *layer_create(void)
{
    layer_t *layer;
    
    layer = malloc(sizeof(layer_t));
    layer->protocol = NULL;
    layer->buffer = NULL;
    layer->mask = NULL;
    layer->buffer_size = 0;
    layer->header_size = 0;

    return layer;
}

void layer_free(layer_t *layer)
{
    free(layer);
    layer = NULL;
}

/**
 * \brief Affects a specified protocol to the current layer
 * \param layer Pointer to the layer structure
 * \param name Name of the protocol to affect
 */
void layer_set_protocol(layer_t *layer, protocol_t *protocol)
{
    layer->protocol = protocol;
}

void layer_set_buffer_size(layer_t *layer, size_t buffer_size)
{
    layer->buffer_size = buffer_size;
}

size_t layer_get_buffer_size(layer_t *layer)
{
    return layer->buffer_size;
}

void layer_set_header_size(layer_t *layer, size_t header_size)
{
    layer->header_size = header_size;
}

unsigned char * layer_get_buffer(layer_t *layer)
{
    return layer->buffer;
}

void layer_set_buffer(layer_t *layer, unsigned char *buffer)
{
    layer->buffer = buffer;
}

void layer_set_mask(layer_t *layer, unsigned char *mask)
{
    layer->mask = mask;
}

field_t *layer_get_field(layer_t *layer, const char * name)
{
    protocol_field_t *pfield;
    if (!layer->protocol)
        return NULL;

    pfield = protocol_get_field(layer->protocol, name);
    if (!pfield)
        return NULL;

    if (pfield->get)
        return pfield->get(layer->buffer);
    else
        return field_create_from_network(pfield->type, name, layer->buffer + pfield->offset);
}

int layer_set_field(layer_t *layer, field_t *field)
{
    protocol_field_t *pfield;
    size_t pfield_size;

    if (!layer->protocol)
        return -1; // payload

    pfield = protocol_get_field(layer->protocol, field->key);
    if (!pfield)
        return -1; // field not found

    /* Check we have enough room in the probe buffer */
    pfield_size = field_get_type_size(pfield->type);
    if (pfield->offset + pfield_size > layer->header_size) {
        /* NOTE the allocation of the buffer might be tricky for headers with
         * variable len (such as IPv4 with options, etc.).
         */
        return -2; // the allocated buffer is not sufficient
    }

    /* 
     * Copy the field value into the buffer 
     * If we have a setter function, we use it, otherwise write the value
     * directly
     */
    if (pfield->set)
        pfield->set(layer->buffer, field);
    else
        protocol_field_set(pfield, layer->buffer, field);

    /* XXX update mask here */

    return 0;
}

int layer_set_payload(layer_t *layer, buffer_t * payload)
{
    if (layer->protocol)
        return -1; // can only set the payload layer

    if (buffer_get_size(payload) > layer->buffer_size)
        return -1; // not enough buffer space

    /* This could be some buffer helper function */
    memcpy(layer->buffer, buffer_get_data(payload), buffer_get_size(payload));
    return 0;
}

int layer_write_payload(layer_t * layer, buffer_t * payload, unsigned int offset)
{
    if (layer->protocol)
        return -1; // can only set the payload layer

    if (offset + buffer_get_size(payload) > layer->buffer_size)
        return -1; // not enough buffer space

    memcpy(layer->buffer + offset, buffer_get_data(payload), buffer_get_size(payload));
    return 0;
}

void layer_dump(layer_t *layer, unsigned int indent)
{
    protocol_field_t *pfield;
    print_indent(indent);

    if (!layer->protocol) {
        printf("PAYLOAD size = %d\n", layer->buffer_size);
        return;
    }

    printf("LAYER: %s\n", layer->protocol->name);
    print_indent(indent);
    printf("----------\n");
    
    /* We loop through all the fields of the protocol and dump them */
    for(pfield = layer->protocol->fields; pfield->key; pfield++) {
        field_t *field;
        field = layer_get_field(layer, pfield->key);
        print_indent(indent);
        printf("%s\t", pfield->key);
        field_dump(field);    
        printf("\n");
    }
    
}
