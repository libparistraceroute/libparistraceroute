#include <stdlib.h>
#include <stdio.h> // XXX
#include <stdarg.h>
#include <string.h>
#include <netinet/in.h> // IPPROTO_ICMPV6
#include <netinet/ip_icmp.h> // ICMP_DEST_UNREACH, ICMP_TIME_EXCEEDED

#include "buffer.h"
#include "probe.h"
#include "protocol.h"
#include "pt_loop.h"
#include "common.h"
#include "metafield.h"
#include "bitfield.h"

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
    probe->bitfield = bitfield_create(/* size_in_bits */ 0);

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

probe_t * probe_dup(probe_t * skel)
{
    unsigned int ret;
    probe_t * probe;
    
    probe = probe_create();
    if (!probe) goto ERR_PROBE;

    // Create the buffer to store the field content
    // This will reconstruct the layer structure
    ret = probe_set_buffer(probe, buffer_dup(skel->buffer));
    if (ret < 0) goto ERR_BUFFER;

    // Bitfield that manages which bits have already been set. 
    // For the moment this is an empty bitfield
    probe->bitfield = bitfield_dup(skel->bitfield);
    if (!probe->bitfield) goto ERR_BITFIELD;

    // Save which instance (caller) create this probe
    probe->caller = NULL;
    return probe;

ERR_BITFIELD:
    buffer_free(probe->buffer);
ERR_BUFFER:
    probe_free(probe);
ERR_PROBE:
    return NULL;

}

void probe_free(probe_t * probe)
{
    //printf(">>> Freeing probe @%x\n", probe);
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

inline buffer_t * probe_get_buffer(const probe_t * probe) {
    return probe ? probe->buffer : NULL;
}

int probe_set_buffer(probe_t * probe, buffer_t * buffer)
{
    size_t          size; // to prevent underflow
    size_t          offset;
    unsigned char * data;
    protocol_t    * protocol;
    uint8_t         protocol_id,
                    ipv4_protocol_id = 4;
    probe->buffer = buffer;

    // buffer_dump(probe->buffer);
    
    data = buffer_get_data(buffer);
    size = buffer_get_size(buffer);

    /* Remove the former layer structure */
    dynarray_clear(probe->layers, (ELEMENT_FREE) layer_free);

    unsigned char ip_version = buffer_guess_ip_version(buffer);
             
////////////////////////:
    protocol = ip_version == 6 ? protocol_search("ipv6") :
               ip_version == 4 ? protocol_search("ipv4") :
               NULL;
    if (!protocol) {
        perror("E: probe_set_buffer: cannot guess IP version");
    }
    protocol_id = protocol-> protocol;
///////////////

    offset = 0;

    for(;;) {
        layer_t       * layer;
        protocol_t    * protocol;
        const field_t * field;
        size_t          hdrlen;
        //printf("-------> coucou\n");

        layer = layer_create();
        layer_set_buffer(layer, data + offset);
        layer_set_buffer_size(layer, size);

        dynarray_push_element(probe->layers, layer);

        protocol = protocol_search_by_id(protocol_id);
        if (!protocol)
            return -1; // Cannot find matching protocol

        hdrlen = protocol->header_len;

        layer_set_protocol(layer, protocol);
        layer_set_header_size(layer, hdrlen);

        offset += hdrlen;
        size -= hdrlen;
        if (size < 0)
            return -1; 

        // In the case of ICMP, while protocol is not really a field, we might
        // provide it by convenience
        // Need for heuristics // source port hook to parse packet content

        // Loop until reaching the payload or an ICMP layer
        // layer_get_field returns NULL iif we've reached an ICMP layer or the payload 
        field = layer_get_field(layer, "protocol");
        if (field) {
            protocol_id = field->value.int8;
            continue;
        } //else {
            //protocol_id = 6; // ICMP6 + IPv6
        //} 

        else if (strcmp(layer->protocol->name, "icmp") == 0) {
            // We are in an ICMP layer
            field = layer_get_field(layer, "type");

            if (!field) {
                // Weird ICMP packet !
                return -1; 
            }

            // 3 == Destination unreachable
            // 11 == Time exceed 
            if ((field->value.int8 == ICMP_DEST_UNREACH)
            ||  (field->value.int8 == ICMP_TIME_EXCEEDED)) {
                // Length will be wrong !!!
                protocol_id = ipv4_protocol_id;
            } else {
                // Set protocol_id to payload
                protocol_id = 0;
                break;
            }
        } else if (protocol_id == IPPROTO_ICMPV6) { // MARKERIP
            // printf(" we think this is payload\n");
            protocol_id = IPPROTO_IPV6; // ICMP6 + IPv6            
          //  protocol_id = 0; // payload 
           // printf("break 1");
            break;
        } else {
            // We are in the payload
            protocol_id = 0;
            break;
        }
    } 

    // payload
    if (protocol_id == 0) {
        // XXX some icmp packets do not have payload
        // Happened with type 3 !
        layer_t *layer = layer_create();
        layer_set_buffer(layer, data + offset);
        layer_set_buffer_size(layer, size);
        layer_set_header_size(layer, 0);

        dynarray_push_element(probe->layers, layer);
    }
    return 0; 

}

// Dump
void probe_dump(const probe_t *probe)
{
    size_t size;
    unsigned int i;

    // Let's loop through the layers and print all fields
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

int probe_finalize(probe_t * probe)
{
    unsigned int   i, size;
    layer_t      * layer;

    size = dynarray_get_size(probe->layers);

    // Allow the protocol to do some processing before checksumming
    for (i = 0; i < size; i++) {
        layer = dynarray_get_ith_element(probe->layers, i);

        // finalize callback
        if (!layer->protocol)
            continue;
        if (layer->protocol->finalize)
            layer->protocol->finalize(layer->buffer);
    }

    return 0;
}

int probe_update_protocol(probe_t * probe)
{
    layer_t * layer, * prev_layer = NULL;
    protocol_field_t * pfield;
    unsigned int i, size;

    size = dynarray_get_size(probe->layers);

    for (i = 0; i < size; i++) {
        layer = dynarray_get_ith_element(probe->layers, i);

        if (!layer->protocol)
            continue;
        if (prev_layer) {
            pfield = protocol_get_field(layer->protocol, "protocol");
            if (pfield) {
                layer_set_field(layer, I16("protocol", (uint16_t)prev_layer->protocol->protocol));
            }
        }
        prev_layer = layer;
    }

    return 0;
}

int probe_update_length(probe_t * probe)
{
    unsigned int       i, size;
    protocol_field_t * pfield;
    layer_t          * layer;

 // XXX IPv6 Hacks
    // TODO refactor
   // int proto_ipv6 = protocol_search("ipv6")->protocol;
   // int proto_ipv6_hdrlen = 40;

    size = dynarray_get_size(probe->layers);

    // Allow the protocol to do some processing before checksumming
    for (i = 0; i<size; i++) {
        layer = dynarray_get_ith_element(probe->layers, i);

        if (!layer->protocol)
            continue;
        pfield = protocol_get_field(layer->protocol, "length");
        if (pfield) {
                layer_set_field(layer, I16("length", (uint16_t)(layer->buffer_size)));		
        //	if (layer->protocol->protocol == proto_ipv6){
        //		layer_set_field(layer, I16("length", (uint16_t)((layer->buffer_size) - proto_ipv6_hdrlen)));
        //	}else {
        //		layer_set_field(layer, I16("length", (uint16_t)(layer->buffer_size)));
        //	}
        }
    }

    return 0;
}
int probe_update_checksum(probe_t * probe)
{
    unsigned int   size;
    int            i;
    layer_t      * layer;

    size = dynarray_get_size(probe->layers);

    // Go though the layers of the probe in the reverse order to write
    // checksums
    // XXX layer_t will require parent layer, and probe_t bottom_layer
    for (i = size - 1; i >= 0; i--) {
        layer = dynarray_get_ith_element(probe->layers, i);
        if (!layer->protocol)
            continue;
        /* Does the protocol require a pseudoheader ? */
        if (layer->protocol->need_ext_checksum) {
            layer_t    * layer_prev;
            buffer_t   * psh;

            if (i == 0)
                return -1;

            // XXX todo compute udp checksum !!!!
            //
            /* for example, udp gets a pointer to the upper ipv4 layer */
            layer_prev = dynarray_get_ith_element(probe->layers, i-1);

            // XXX We should specify which layer we have
            psh = layer->protocol->create_pseudo_header(layer_prev->buffer);
            if (!psh)
                return -1;
            layer->protocol->write_checksum(layer->buffer, psh);
            free(psh); 

        } else {
            // could be a function in layer ?
            layer->protocol->write_checksum(layer->buffer, NULL);
        }
    }
    return 0;
}

int probe_update_fields(probe_t *probe)
{
    int res;

    res = probe_finalize(probe);
    if (res < 0) goto error;

    res = probe_update_protocol(probe);
    if (res < 0) goto error;

    res = probe_update_length(probe);
    if (res < 0) goto error;

    res = probe_update_checksum(probe);
    if (res < 0) goto error;

    return 0;
error:
    return -1;
}

// TODO A similar function should allow hooking into the layer structure
// and not at the top layer
int probe_set_protocols(probe_t * probe, char * name1, ...)
{
    va_list   args, args2;
    size_t    buflen, offset;
    char    * i;
    layer_t * layer, *prev_layer;

    // Remove the former layer structure
    dynarray_clear(probe->layers, (ELEMENT_FREE) layer_free);

    // Set up the new layer structure
    va_start(args, name1);

    // Allocate the buffer according to the layer structure
    buflen = 0;
    va_copy(args2, args);
    for (i = name1; i; i = va_arg(args2, char *)) {
        protocol_t * protocol = protocol_search(i);
        if (!protocol) goto ERROR;
        buflen += protocol->header_len; 
    }
    va_end(args2);
    buffer_resize(probe->buffer, buflen);

    // Create each layer
    offset = 0;
    prev_layer = NULL;
    for (i = name1; i; i = va_arg(args, char *)) {
        protocol_field_t * pfield;
        protocol_t       * protocol;

        // Associate protocol to the layer
        layer = layer_create();
        protocol = protocol_search(i);
        if (!protocol) goto ERROR_LAYER;

        layer_set_protocol(layer, protocol);

        // Initialize the buffer with default protocol values
        protocol->write_default_header(buffer_get_data(probe->buffer) + offset);
        layer_set_header_size(layer, protocol->header_len);

        // TODO consider variable length headers
        layer_set_buffer(layer, buffer_get_data(probe->buffer) + offset);
        layer_set_buffer_size(layer, buflen - offset);
        layer_set_mask(layer, bitfield_get_mask(probe->bitfield) + offset);

        pfield = protocol_get_field(layer->protocol, "length");
        if (pfield) {
            layer_set_field(layer, I16("length", (uint16_t)(buflen - offset)));
        }

        if (prev_layer) {
            pfield = protocol_get_field(layer->protocol, "protocol");
            if (pfield) {
                layer_set_field(layer, I16("protocol", (uint16_t) prev_layer->protocol->protocol));
            }
        }

        offset += protocol->header_len; 
        dynarray_push_element(probe->layers, layer);
        prev_layer = layer;
    }
    va_end(args);

    // Payload : initially empty
    layer = layer_create();
    layer_set_buffer(layer, buffer_get_data(probe->buffer) + buflen);
    layer_set_buffer_size(layer, 0); // XXX unless otherwise specified
    layer_set_header_size(layer, 0);

    dynarray_push_element(probe->layers, layer);
    // buflen += 0;

    // Size and checksum are pending, they depend on payload 
    
    return 0;

ERROR_LAYER:
    dynarray_clear(probe->layers, (ELEMENT_FREE) layer_free);
ERROR:
    return -1;
}

int probe_set_field_ext(probe_t *probe, field_t *field, unsigned int depth)
{
    unsigned int i;
    int res = -1;
    size_t size;
    layer_t *layer;

    /* We go through the layers until we get the required field */
    res = 0;
    size = dynarray_get_size(probe->layers);
    for(i = depth; i < size; i++) {
        layer = dynarray_get_ith_element(probe->layers, i);
        
        res = layer_set_field(layer, field);
        /* We stop as soon as a layer succeeds */
        if (res == 0)
            break;
    }

    return res;
}

inline int probe_set_field(probe_t * probe, field_t * field)
{
    return probe_set_field_ext(probe, field, 0);
}


int probe_set_metafield(probe_t * probe, field_t * field)
{
    metafield_t * metafield;
    field_t     * f;

    /* TEMP HACK : we only have IPv4 and flow_id, let's encode it into the
     * src_port */
    if (strcmp(field->key, "flow_id") != 0)
        return -1;

    f = field_create_int16("src_port", 24000 + (uint16_t)(field->value.int16));
    return probe_set_field(probe, f);
    
    /* XXX */

    metafield = metafield_search(field->key);
    if (!metafield)
        return -1; // Metafield not found

    /* Does the probe verifies one metafield pattern */

    /* Does the value conflict with a previously set field ? */

    
}

int probe_resize_buffer(probe_t * probe, unsigned int size)
{
    unsigned int       offset, num_layers, i;
    protocol_field_t * pfield;
    /* We can only resize the last layer (payload) */
    /* TODO */

    buffer_resize(probe->buffer, size);

    num_layers = dynarray_get_size(probe->layers);

    offset = 0;

    /* We need to reaffect the different layer pointers, and reassign length */
    for (i = 0; i < num_layers; i++) {
        layer_t *layer;

        layer = dynarray_get_ith_element(probe->layers, i);
        layer_set_buffer(layer, buffer_get_data(probe->buffer) + offset);

        if (layer->protocol) {
            pfield = protocol_get_field(layer->protocol, "length");
            if (pfield) {
                layer_set_field(layer, I16("length", (uint16_t)(size - offset)));
            }

            offset += layer->protocol->header_len; 

        } else {
            /* Otherwise, we are at the payload, which is the last layer */
            layer_set_buffer_size(layer, size - offset);
            layer_set_header_size(layer, 0);
        }
    }

    return 0;
    
}

int probe_set_payload_size(probe_t * probe, unsigned int payload_size)
{
    // TODO factorize with probe_set_min_payload_size
    unsigned int size;
    unsigned int old_buffer_size, old_payload_size;
    layer_t * payload_layer;
    
    /* The payload is the last layer */
    size = dynarray_get_size(probe->layers);
    payload_layer = dynarray_get_ith_element(probe->layers, size - 1);

    old_buffer_size = buffer_get_size(probe->buffer);
    old_payload_size = layer_get_buffer_size(payload_layer);

    if (old_payload_size != payload_size) {
        /* Resize probe buffer */
        probe_resize_buffer(probe, old_buffer_size - old_payload_size + payload_size);

        /* Change the buffer, and fix layers offsets */
        layer_set_buffer_size(payload_layer, payload_size);
    }

    // XXX probe_update_fields(probe);

    return 0;
}

int probe_set_min_payload_size(probe_t * probe, unsigned int payload_size)
{
    unsigned int size;
    unsigned int old_buffer_size, old_payload_size;
    layer_t * payload_layer;
    
    /* The payload is the last layer */
    size = dynarray_get_size(probe->layers);
    payload_layer = dynarray_get_ith_element(probe->layers, size - 1);

    old_buffer_size = buffer_get_size(probe->buffer);
    old_payload_size = layer_get_buffer_size(payload_layer);

    if (old_payload_size < payload_size) {
        /* Resize probe buffer */
        probe_resize_buffer(probe, old_buffer_size - old_payload_size + payload_size);

        /* Change the buffer, and fix layers offsets */
        layer_set_buffer_size(payload_layer, payload_size);
    }

    // XXX probe_update_fields(probe);

    return 0;
}

int probe_set_payload(probe_t *probe, buffer_t * payload)
{
    unsigned int size;
    layer_t * payload_layer;

    probe_set_payload_size(probe, buffer_get_size(payload));
    
    /* The payload is the last layer */
    size = dynarray_get_size(probe->layers);
    payload_layer = dynarray_get_ith_element(probe->layers, size - 1);

    layer_set_payload(payload_layer, payload);

    // XXX probe_update_fields(probe); // done twice probe_set_payload_size

    return 0;
}

int probe_write_payload(probe_t *probe, buffer_t * payload, unsigned int offset)
{
    unsigned int size;
    layer_t * payload_layer;

    /* We need to ensure we have sufficient buffer size  */
    probe_set_min_payload_size(probe, offset + buffer_get_size(payload));
    
    /* The payload is the last layer */
    size = dynarray_get_size(probe->layers);
    payload_layer = dynarray_get_ith_element(probe->layers, size - 1);

    layer_write_payload(payload_layer, payload, offset);

    // XXX probe_update_fields(probe); // done twice probe_set_min_payload_size

    return 0;
}

// Internal use
const field_t * probe_get_metafield(const probe_t * probe, const char * name)
{
    const field_t * field;
    field_t       * ret_field;

    // TODO to generalize to any metafield
    if (strcmp(name, "flow_id") != 0) return NULL;

    // TODO to adapt for IPv6 support 
    field = probe_get_field(probe, "src_port");
    ret_field = IMAX("flow_id", field->value.int16 - 24000);
    return ret_field;
}

// TODO same function starting at a given layer
int probe_set_fields(probe_t *probe, field_t *field1, ...) {
    va_list   args;
    field_t * field;
    int res;

    va_start(args, field1);

    for (field = field1; field; field = va_arg(args, field_t*)) {
        /* Going from the first layer, we set the field to the first layer that
         * possess it */
        res = probe_set_field(probe, field);
        if (res == 0)
            continue;

        /* Metafield ? */
        res = probe_set_metafield(probe, field);
        if (res == 0)
            continue;

        printf("W: could not set field %s\n", field->key);
        // break; // field cannot be set in any subfield
    }

    va_end(args);

    // XXX probe_update_fields(probe);
    
    /* 0 if all fields could be set */
    return res;
}

int probe_set_caller(probe_t * probe, void * caller)
{
    probe->caller = caller;
    return 0;
}

void * probe_get_caller(const probe_t * probe)
{
    return probe->caller;
}

int probe_set_sending_time(probe_t * probe, double time)
{
    probe->sending_time = time;
    return 0;
}

double probe_get_sending_time(const probe_t * probe)
{
    return probe->sending_time;
}

int probe_set_queueing_time(probe_t * probe, double time)
{
    probe->queueing_time = time;
    return 0;
}

double probe_get_queueing_time(const probe_t *probe)
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

unsigned int probe_get_num_proto(const probe_t *probe)
{
    return 0; // TODO
}

field_t ** probe_get_fields(const probe_t * probe)
{
    return NULL; // TODO
}

const field_t * probe_get_field_ext(const probe_t * probe, const char * name, unsigned int depth)
{
    size_t          size;
    size_t          i;
    layer_t       * layer;
    const field_t * field;

    
    // We go through the layers until we get the required field
    size = dynarray_get_size(probe->layers);
    for(i = depth; i < size; i++) {
        layer = dynarray_get_ith_element(probe->layers, i);

        if ((field = layer_get_field(layer, name))) return field;
    }
    // Not found, this is maybe a metafield
    return probe_get_metafield(probe, name);
}

const field_t * probe_get_field(const probe_t * probe, const char * name)
{
    return probe_get_field_ext(probe, name, 0);
}

unsigned char * probe_get_payload(const probe_t * probe)
{
    // point into the packet structure
    return NULL; // TODO
}

unsigned int probe_get_payload_size(const probe_t * probe)
{
    return 0; // TODO
}

char * probe_get_protocol_by_index(unsigned int i)
{
    return NULL; // TODO
}


/******************************************************************************
 * probe_reply_t
 ******************************************************************************/

probe_reply_t * probe_reply_create(void)
{
    probe_reply_t * probe_reply;

    probe_reply = malloc(sizeof(probe_reply_t));
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

int probe_reply_set_probe(probe_reply_t * probe_reply, probe_t * probe)
{
    probe_reply->probe = probe;
    return 0;
}

probe_t * probe_reply_get_probe(const probe_reply_t * probe_reply)
{
    return probe_reply->probe;
}

int probe_reply_set_reply(probe_reply_t * probe_reply, probe_t * reply)
{
    probe_reply->reply = reply;
    return 0;
}

probe_t * probe_reply_get_reply(const probe_reply_t * probe_reply)
{
    return probe_reply->reply;
}
