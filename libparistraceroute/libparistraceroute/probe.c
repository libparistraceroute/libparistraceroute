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

int probe_set_buffer(probe_t *probe, buffer_t *buffer)
{
    size_t          size;
    unsigned char * data;
    protocol_t    * protocol;
    uint8_t         protocol_id;

    probe->buffer = buffer;

    // XXX dump buffer
    buffer_dump(probe->buffer);
    
    data = buffer_get_data(buffer);
    size = buffer_get_size(buffer);

    /* Remove the former layer structure */
    dynarray_clear(probe->layers, (void(*)(void*))layer_free);

    /* FIXME Let's suppose we have an IPv4 protocol */
    protocol = protocol_search("ipv4");
    protocol_id = protocol->protocol;

    for(;;) {
        layer_t *layer;
        protocol_t *protocol;
        field_t *field;
        size_t hdrlen;

        layer = layer_create();
        layer_set_buffer(layer, data);
        layer_set_buffer_size(layer, size);

        dynarray_push_element(probe->layers, layer);

        protocol = protocol_search_by_id(protocol_id);
        printf("Searching protocol of id %hhu\n", protocol_id);
        if (!protocol)
            return -1; // Cannot find matching protocol
        printf("found: %s\n", protocol->name);

        layer_set_protocol(layer, protocol);
        hdrlen = protocol->header_len;

        data += hdrlen;
        size -= hdrlen;

        /* In the case of ICMP, while protocol is not really a field, we might
         * provide it by convenience
         */
        field = layer_get_field(layer, "protocol");
        if (!field) {
            printf("Protocol field not found in current layer [%s]... doing payload\n", layer->protocol->name);
            /* last field = payload */
            layer = layer_create();
            layer_set_buffer(layer, data);
            layer_set_buffer_size(layer, size);

            dynarray_push_element(probe->layers, layer);
            return 0;
        } 

        /* continue with next layer considering this protocol */
        protocol_id = field->int8_value;
    }
    return 0; 

}

// Dump
void probe_dump(probe_t *probe)
{
    size_t size;
    unsigned int i;

    /* Let's loop through the layers and print all fields */
    printf("\n\n** PROBE **\n\n");
    size = dynarray_get_size(probe->layers);
    for(i = 0; i < size; i++) {
        layer_t *layer;
        layer = dynarray_get_ith_element(probe->layers, i);
        layer_dump(layer, i);
        printf("\n");
    }
    printf("\n");
}

// TODO A similar function should allow hooking into the layer structure
// and not at the top layer
int probe_set_protocols(probe_t *probe, char *name1, ...)
{
    va_list  args, args2;
    size_t   buflen, offset;
    char   * i;
    layer_t *layer, *prev_layer;

    /* Remove the former layer structure */
    dynarray_clear(probe->layers, (void(*)(void*))layer_free);

    /* Set up the new layer structure */
    va_start(args, name1);

    buflen = 0;
    /* allocate the buffer according to the layer structure */
    va_copy(args2, args);
    for (i = name1; i; i = va_arg(args2, char*)) {
        protocol_t *protocol;
        protocol = protocol_search(i);
        if (!protocol)
            goto error;
        buflen += protocol->header_len; 
    }
    va_end(args2);

    buffer_resize(probe->buffer, buflen);

    offset = 0;
    prev_layer = NULL;
    for (i = name1; i; i = va_arg(args, char*)) {
        protocol_field_t *pfield;
        protocol_t *protocol;

        /* Associate protocol to the layer */
        layer = layer_create();
        protocol = protocol_search(i);
        if (!protocol)
            goto error;

        layer_set_protocol(layer, protocol);

        /* Initialize the buffer with default protocol values */
        protocol->write_default_header(buffer_get_data(probe->buffer) + offset);
        layer_set_header_size(layer, protocol->header_len);

        // TODO consider variable length headers */
        layer_set_buffer(layer, buffer_get_data(probe->buffer) + offset);
        layer_set_buffer_size(layer, buflen - offset);

        pfield = protocol_get_field(layer->protocol, "length");
        if (pfield) {
            layer_set_field(layer, I16("length", (uint16_t)(buflen - offset)));
        }

        if (prev_layer) {
            pfield = protocol_get_field(layer->protocol, "protocol");
            if (pfield) {
                layer_set_field(layer, I16("protocol", (uint16_t)prev_layer->protocol->protocol));
            }
        }

        offset += protocol->header_len; 

        dynarray_push_element(probe->layers, layer);

        prev_layer = layer;
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
    int res;

    va_start(args, field1);

    for (field = field1; field; field = va_arg(args, field_t*)) {
        /* Going from the first layer, we set the field to the first layer that
         * possess it */
        unsigned int i;
        size_t size;
        layer_t *layer;

        /* We go through the layers until we get the required field */
        res = 0;
        size = dynarray_get_size(probe->layers);
        for(i = 0; i < size; i++) {
            layer = dynarray_get_ith_element(probe->layers, i);
            
            res = layer_set_field(layer, field);
            /* We stop as soon as a layer succeeds */
            if (res == 0)
                break;
        }

        if (res < 0)
            break; // field cannot be set in any subfield
    }

    va_end(args);
    
    /* 0 if all fields could be set */
    return res;
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
