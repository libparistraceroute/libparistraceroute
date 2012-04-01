#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include "layer.h"

layer_t *layer_create(void)
{
    layer_t *layer;
    
    layer = malloc(sizeof(layer_t));
    layer->buffer = NULL;
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

void layer_set_header_size(layer_t *layer, size_t header_size)
{
    layer->header_size = header_size;
}

void layer_set_buffer(layer_t *layer, unsigned char *buffer)
{
    layer->buffer = buffer;
}

field_t *layer_get_field(layer_t *layer, char *name)
{
    protocol_field_t *pfield;
    field_t *field;

    pfield = protocol_get_field(layer->protocol, name);
    if (!pfield)
        return NULL;
    field = field_create(pfield->type, name, layer->buffer + pfield->offset);
    return field;
}

void print_indent(unsigned int indent)
{
    int i;
    for(i = 0; i < indent; i++)
        printf("    ");
}

void layer_dump(layer_t *layer, unsigned int indent)
{
    protocol_field_t *pfield;
    print_indent(indent);
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
