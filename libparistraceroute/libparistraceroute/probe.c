#include <stdlib.h>
#include <stdio.h> // XXX
#include <stdarg.h>
#include <string.h>

#include "probe.h"
#include "protocol.h"
#include "pt_loop.h"
#include "common.h"

// TODO update bitfield

probe_t * probe_create(void)
{
    probe_t * probe = malloc(sizeof(probe_t));
    if (!probe) goto ERR_PROBE;

    // Create the buffer to store the field content
    probe->buffer = buffer_create();
    if (!probe->buffer) goto ERR_BUFFER;

    // Initially the probe has no layers
    probe->layers = dynarray_create();
    if (!probe->layers) goto ERR_LAYERS;

    // Bitfield that manages which bits have already been set. 
    // For the moment this is an empty bitfield
    probe->bitfield = bitfield_create(0); // == NULL

    // Save which instance (caller) create this probe
    probe->caller = NULL;
    return probe;

ERR_LAYERS:
    buffer_free(probe->buffer);
ERR_BUFFER:
    free(probe);
ERR_PROBE:
    return NULL;
}

void probe_free(probe_t * probe)
{
    printf(">>> Freeing probe @%x\n", probe);
    if (probe) {
        /*
        bitfield_free(probe->bitfield);
        dynarray_free(probe->layers, (ELEMENT_FREE) layer_free);
        buffer_free(probe->buffer);
        */
        free(probe);
    }
}

// Accessors

inline buffer_t * probe_get_buffer(probe_t * probe) {
    return probe ? probe->buffer : NULL;
}

int probe_set_buffer(probe_t *probe, buffer_t *buffer)
{
    size_t          size;
    unsigned char * data;
    protocol_t    * protocol;
    uint8_t         protocol_id, ipv4_protocol_id;

    probe->buffer = buffer;

    // buffer_dump(probe->buffer);
    
    data = buffer_get_data(buffer);
    size = buffer_get_size(buffer);

    /* Remove the former layer structure */
    dynarray_clear(probe->layers, (void(*)(void*))layer_free);

    /* FIXME Let's suppose we have an IPv4 protocol */
    protocol = protocol_search("ipv4");
    ipv4_protocol_id = protocol->protocol;

    protocol_id = ipv4_protocol_id;

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
        if (!protocol)
            return -1; // Cannot find matching protocol

        layer_set_protocol(layer, protocol);
        hdrlen = protocol->header_len;

        data += hdrlen;
        size -= hdrlen;

        /* In the case of ICMP, while protocol is not really a field, we might
         * provide it by convenience
         */
        field = layer_get_field(layer, "protocol");
        if (!field) {
            /* FIXME: special treatment : Do we have an ICMP header ? */
            field = layer_get_field(layer, "type");
            if (!field)
                return -1; // weird icmp packet !
            if (field->value.int8 == 11) { // TTL expired, an IP packet header is repeated !
                // Length will be wrong !!!
                protocol_id = ipv4_protocol_id;
                continue;
            } else {
                // XXX some icmp packets do not have payload
                // Happened with type 3 !
                /* last field = payload */
                layer = layer_create();
                layer_set_buffer(layer, data);
                layer_set_buffer_size(layer, size);

                dynarray_push_element(probe->layers, layer);
                return 0;
            }
        } 

        /* continue with next layer considering this protocol */
        protocol_id = field->value.int8;
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

int probe_set_caller(probe_t *probe, void *caller)
{
    probe->caller = caller;
    return 0;
}

void *probe_get_caller(probe_t *probe)
{
    return probe->caller;
}

int probe_set_sending_time(probe_t *probe, double time)
{
    probe->sending_time = time;
    return 0;
}

double probe_get_sending_time(probe_t *probe)
{
    return probe->sending_time;
}

int probe_set_queueing_time(probe_t *probe, double time)
{
    probe->queueing_time = time;
    return 0;
}

double probe_get_queueing_time(probe_t *probe)
{
    return probe->queueing_time;
}

// Iterator

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

field_t * probe_get_field(probe_t * probe, const char * name)
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
 * probe_reply_t
 ******************************************************************************/

probe_reply_t *probe_reply_create(void)
{
    probe_reply_t *probe_reply;

    probe_reply = malloc(sizeof(probe_reply));
    probe_reply->probe = NULL;
    probe_reply->reply = NULL;

    return probe_reply;
}

void probe_reply_free(probe_reply_t *probe_reply)
{
    free(probe_reply);
    probe_reply = NULL;
}

// Accessors

int probe_reply_set_probe(probe_reply_t *probe_reply, probe_t *probe)
{
    probe_reply->probe = probe;
    return 0;
}

probe_t * probe_reply_get_probe(probe_reply_t *probe_reply)
{
    return probe_reply->probe;
}

int probe_reply_set_reply(probe_reply_t *probe_reply, probe_t *reply)
{
    probe_reply->reply = reply;
    return 0;
}

probe_t * probe_reply_get_reply(probe_reply_t *probe_reply)
{
    return probe_reply->reply;
}

/******************************************************************************
 * pt_loop_t
 ******************************************************************************/

void pt_probe_send(pt_loop_t *loop, probe_t *probe)
{
    /* We remember which algorithm has generated the probe */
    probe_set_caller(probe, pt_loop_get_algorithm_instance(loop));

    probe_set_queueing_time(probe, get_timestamp());

    network_send_probe(loop->network, probe);
}
