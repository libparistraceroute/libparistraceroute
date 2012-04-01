#include <stdlib.h>
#include <stdio.h> // XXX
#include <stdarg.h>
#include <string.h>

#include "probe.h"
#include "protocol.h"
#include "pt_loop.h"

probe_t * probe_create(void)
{
    probe_t *probe;

    probe = malloc(sizeof(probe_t));
    if (!probe)
        goto err_probe;

    /* Create the buffer to store the field content */
    probe->buffer = buffer_create();
    if (!(probe->buffer))
        goto err_buffer;

    /* Initially the probe has no layers */
    probe->layers = dynarray_create();
    if (!probe->layers)
        goto err_layers;

    /* ... and an empty bitfield */
    probe->bitfield = bitfield_create(0);
    if (!probe->bitfield)
        goto err_bitfield;

    return probe;

err_bitfield:
    dynarray_free(probe->layers, (void(*)(void*))layer_free);
err_layers:
    buffer_free(probe->buffer);
err_buffer:
    free(probe);
    probe = NULL;
err_probe:
    return NULL;
}

void probe_free(probe_t *probe)
{
    bitfield_free(probe->bitfield);
    dynarray_free(probe->layers, (void(*)(void*))layer_free);
    buffer_free(probe->buffer);
    free(probe);
    probe = NULL;
}

// Accessors

buffer_t *probe_get_buffer(probe_t *probe)
{
    return probe->buffer;
}

// Dump
void probe_dump(probe_t *probe)
{
    size_t size;
    unsigned int i;

    /* Let's loop through the layers and print all fields */
    size = dynarray_get_size(probe->layers);
    for(i = 0; i < size; i++) {
        layer_t *layer;
        layer = dynarray_get_ith_element(probe->layers, i);
        layer_dump(layer, i);
    }
}

// TODO A similar function should allow hooking into the layer structure
// and not at the top layer
int probe_set_protocols(probe_t *probe, char *name1, ...)
{
    va_list  args, args2;
    size_t   buflen = 0;
    char   * i;
    layer_t *layer;

    /* Remove the former layer structure */
    dynarray_clear(probe->layers, (void(*)(void*))layer_free);

    /* Set up the new layer structure */
    va_start(args, name1);

    /* allocate the buffer according to the layer structure */
    va_copy(args2, args);
    for (i = name1; i; i = va_arg(args2, char*)) {
        protocol_t *protocol;
        printf("searching protocol %s\n", i);
        protocol = protocol_search(i);
        if (!protocol)
            goto error;
        printf("headerlen = %lu\n", protocol->header_len);
        buflen += protocol->header_len; 
    }
    va_end(args2);

    printf("buffer resize to %lu\n", buflen);
    buffer_resize(probe->buffer, buflen);

    buflen = 0;
    for (i = name1; i; i = va_arg(args, char*)) {
        protocol_t *protocol;

        /* Associate protocol to the layer */
        layer = layer_create();
        printf("searching protocol %s\n", i);
        protocol = protocol_search(i);
        if (!protocol)
            goto error;

        layer_set_protocol(layer, protocol);

        /* Initialize the buffer with default protocol values */
        protocol->write_default_header(buffer_get_data(probe->buffer) + buflen);
        layer_set_header_size(layer, protocol->header_len);

        // TODO consider variable length headers */
        layer_set_buffer(layer, buffer_get_data(probe->buffer) + buflen);
        // layer_set_buffer_size unknown (useful for checksum ?)

        buflen += protocol->header_len; 

        dynarray_push_element(probe->layers, layer);
    }
    va_end(args);

    /* Payload : initially empty */
    layer = layer_create();
    layer_set_buffer(layer, buffer_get_data(probe->buffer) + buflen);
    // buflen += 0;

    // Size and checksum are pending, they depend on payload 
    
    return 0;

error: // TODO free()
    printf("ERROR\n");
    return -1;
}

// TODO same function starting at a given layer
int probe_set_fields(probe_t *probe, field_t *field1, ...) {
    va_list args;
    field_t *field;

    va_start(args, field1);
    //res = layer_set_fields(probe->top_layer, field1, args);
    for (field = field1; field; field = va_arg(args, field_t*)) {
        // XXX
        /* Going from the first layer, we set the field to the first layer that
         * possess it */
        unsigned int i;
        protocol_field_t *pfield;
        size_t pfield_size, size;
        layer_t *layer;

        printf("setting field %s\n", field->key);

        /* We go through the layers until we get the required field */
        size = dynarray_get_size(probe->layers);
        for(i = 0; i < size; i++) {
            
            printf("looking layer %u/%lu\n", i, size);
            layer = dynarray_get_ith_element(probe->layers, i);
            pfield = protocol_get_field(layer->protocol, field->key);
            if (pfield)
                break;
        }

        /* If we reached the last layer, then the field has not been found */
        if (i == size)
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
    }
    va_end(args);
    
    return 0;
}

/* Iterator */

typedef struct {
    void *data;
    void (*callback)(field_t *field, void *data);
} iter_fields_data_t;

void probe_iter_fields_callback(void *element, void *data) {
    iter_fields_data_t *d = (iter_fields_data_t*)data;
    d->callback((field_t*)element, d->data);
}

void probe_iter_fields(probe_t *probe, void *data, void (*callback)(field_t *, void *))
{
    /*
    iter_fields_data_t tmp = {
        .data = data,
        .callback = callback
    };
    */
    
    // not implemented : need to iter over protocol fields of each layer
}

unsigned int probe_get_num_proto(probe_t *probe)
{
    return 0; // TODO
}

field_t ** probe_get_fields(probe_t *probe)
{
    return NULL; // TODO
}

field_t *probe_get_field(probe_t *probe, char *name)
{
    size_t size;
    unsigned int i;
    layer_t *layer;
    field_t *field;

    /* We go through the layers until we get the required field */
    size = dynarray_get_size(probe->layers);
    for(i = 0; i < size; i++) {
        
        printf("looking layer %u/%lu\n", i, size);
        layer = dynarray_get_ith_element(probe->layers, i);

        field = layer_get_field(layer, name);
        if (field)
            break;
    }

    return field;
}

unsigned char *probe_get_payload(probe_t *probe)
{
    // point into the packet structure
    return NULL; // TODO
}

unsigned int probe_get_payload_size(probe_t *probe)
{
    return 0; // TODO
}

char* probe_get_protocol_by_index(unsigned int i)
{
    return NULL; // TODO
}

/******************************************************************************
 * pt_loop_t
 ******************************************************************************/

void pt_probe_reply_callback(pt_loop_t *loop, probe_t *probe, probe_t *reply)
{
    // Search for probe and find the caller
    //algorithm_instance_add_event(instance->caller->caller_algorithm,
    //        event_create(PROBE_REPLY_RECEIVED, NULL));
    // Delete the probe, what about multiple answers ?
    return;
}

void pt_probe_send(pt_loop_t *loop, probe_t *probe)
{
    /* We need to remember who has been sending the probe */
    /* + Reply callback */
    network_send_probe(loop->network, probe, pt_probe_reply_callback);
}
