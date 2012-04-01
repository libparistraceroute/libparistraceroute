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

    return 0;
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
