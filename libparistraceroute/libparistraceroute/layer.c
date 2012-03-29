#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include "layer.h"

layer_t *layer_create(void)
{
    layer_t *layer;
    
    layer = malloc(sizeof(layer_t));
    layer->sublayer = NULL;
    layer->offset = 0;
    layer->size = 0;

    return layer;
}

void layer_free(layer_t *layer)
{
    free(layer);
    layer = NULL;
}

int layer_set_protocols(layer_t *layer, char *name1, ...)
{
    layer_t *cur_layer;
    va_list args;
    char *i;

    va_start(args, name1);
    cur_layer = layer;
    for (i = name1; i; i = va_arg(args, char*)) {
        int res;
        /* affects the protocol to the current layer */
        res = layer_set_protocol(cur_layer, i);
        if (res < 0)
            goto error;
        /* We need at least one more sublayer (other protocol or payload) */
        if (!cur_layer->sublayer)
            cur_layer->sublayer = layer_create();
        if (!cur_layer->sublayer)
            goto error;
        cur_layer = cur_layer->sublayer;
    }
    va_end(args);

    /* the last layer is the payload */
    layer_set_payload(cur_layer, NULL);

    return 0;

error:
    /* If the function fails to find a corresponding protocol, or if a sublayer
     * cannot be created, the remaining protocols are ignored and the current
     * layer is defined as the payload
     */
    layer_set_payload(cur_layer, NULL);
    return -1;

}

int layer_set_payload(layer_t *layer, buffer_t *payload)
{
    /* The payload is defined by being a NULL protocol, and it has no sublayer,
     * so we free them in case they exist
     */
    layer->protocol = NULL;
    if (layer->sublayer)
        layer_free(layer->sublayer);
        /* If some payload has been specified, let's write it, otherwise we just
         * create an empty placeholder
         *
         * XXX does it make sense for a packet ? or do we need some default payload value ?
         */
    if (payload)
       /* TODO */;

    return 0;
}

/**
 * \brief Affects a specified protocol to the current layer
 * \param layer Pointer to the layer structure
 * \param name Name of the protocol to affect
 *
 */
int layer_set_protocol(layer_t *layer, char *name)
{
    /* search for the corresponding protocol */
    layer->protocol = protocol_search(name);
    if (!layer->protocol)
        goto error;

    /* copy default header content into the buffer */
    // XXX need to make sure we have sufficient size allocated
    layer->protocol->write_default_header(layer->offset);
    
    return 0;

error:
    return -1;
}

/**
 * \brief Affects a set of fields to the first layers that possess them.
 * \param layer Pointer ot a layer_t structure representing the first layer to
 * search from
 * \param field1 The first of the series of fields to affect
 * \return 0 if successful, -1 otherwise
 */
int layer_set_fields(layer_t *layer, field_t *field1, ...)
{
    int res = 0;
    va_list args;
    field_t *i;

    va_start(args, field1);
    for (i = field1; i; i = va_arg(args, field_t*)) {
        res = layer_set_field(layer, i);
        if (res == -1)
            break;
    }
    va_end(args);

    return res;
}

/**
 * \brief Affects a field to the first layer that posses it.
 * \param layer The layer from which to search for the field name
 * \param field The field to affect
 */
int layer_set_field(layer_t *layer, field_t *field)
{
    layer_t *cur_layer;
    size_t pfield_size;
    protocol_field_t *pfield;
   
    /* Searching for the first layer that has such field */
    for(cur_layer = layer; layer; layer = layer->sublayer) {
        pfield = protocol_get_field(cur_layer->protocol, field->key);
        if (pfield)
            break;
    }

    /* If we reached the last layer, then the field has not been found */
    if (!layer)
        return -1; // field not found

    /* Check we have enough room in the probe buffer */
    pfield_size = field_get_type_size(pfield->type);
    if (pfield->offset + pfield_size > layer->size) {
        /* NOTE the allocation of the buffer might be tricky for headers with
         * variable len (such as IPv4 with options, etc.).
         */
        return -2; // the allocated buffer is not sufficient
    }

    /* Copy the field value into the buffer */
    memcpy(layer->offset + pfield->offset, &field->value, field_get_type_size(pfield->type));

    return 0;
        
}

