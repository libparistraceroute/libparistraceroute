#include <stdlib.h>
#include <stdio.h> // XXX
#include <errno.h>
#include <stdarg.h>
#include <string.h>
#include <netinet/in.h>      // IPPROTO_IPV6, IPPROTO_ICMPV6
#include <netinet/ip_icmp.h> // ICMP_DEST_UNREACH, ICMP_TIME_EXCEEDED

#include "buffer.h"
#include "probe.h"
#include "protocol.h"
#include "pt_loop.h"
#include "common.h"
#include "metafield.h"
#include "bitfield.h"

//-----------------------------------------------------------
// Probe consistency 
//-----------------------------------------------------------

/**
 * \brief (Internal use) Call for each layer its 'finalize'
 *    callback before checksuming. Finalize unset
 *    fields to a coherent value (for example src_ip in ipv4).
 * \param probe The probe we're finalizing
 * \return true iif successfull
 */

static bool probe_finalize(probe_t * probe);

/**
 * \brief Update for each layer of a probe the 'protocol' field
 *   (if any) in order to have a coherent sequence of layers.
 * \param probe The probe we're updating 
 * \return true iif successfull
 */

static bool probe_update_protocol(probe_t * probe);

/**
 * \brief Update for each layer of a probe the 'length' field
 *   (if any) in order to have a coherent sequence of layers.
 * \param probe The probe we're updating 
 * \return true iif successfull
 */

static bool probe_update_length(probe_t * probe);

/**
 * \brief Update for each layer of a probe the 'checksum' field
 *   (if any) in order to have a coherent sequence of layers.
 * \param probe The probe we're updating 
 * \return true iif successfull
 */

static bool probe_update_checksum(probe_t * probe);

//-----------------------------------------------------------
// Layers management
//-----------------------------------------------------------

/**
 * \brief Retrieve the i-th layer stored in a probe.
 * \param probe The queried probe
 * \param The index of the layer (from 0 to probe_get_num_layers(probe) - 1).
 *   The last layer is the payload.
 * \return The corresponding layer, NULL if i is invalid.
 */

static layer_t * probe_get_layer(const probe_t * probe, size_t i);

/**
 * \brief Retrieve the layer related to the payload from a probe
 * \param probe The queried probe
 * \return The corresponding layer, NULL if i is invalid.
 */

static layer_t * probe_get_layer_payload(const probe_t * probe);

/**
 * \brief Add a layer in the probe. Fields 'length', 'checksum' and
 *    so on are not recomputed.
 * \param layer The layer we're adding to the probe. It is not duplicated.
 * \return true iif successfull
 */

static bool probe_push_layer(probe_t * probe, layer_t * layer); 

/**
 * \brief Add a payload layer in the probe. Fields 'length', 'checksum' and
 *    so on are not recomputed.
 * \param payload_size The length of the payload (in bytes)
 * \return true iif successfull
 */

static bool probe_push_payload(probe_t * probe_t, size_t payload_size);

/**
 * \brief Release layers carried by this probe from the memory
 * \param probe The probe we're updating
 */

static void probe_layers_free(probe_t * probe);

/**
 * \brief Reset layers carried by this probe 
 * \param probe The probe we're updating
 */

static void probe_layers_clear(probe_t * probe);

//-----------------------------------------------------------
// Other static functions
//-----------------------------------------------------------

/**
 * \brief Resize the buffer related to the payload. Update layer pointers and sizes.
 * \param probe The probe we're updateing
 * \param size The new size of the payload
 * \return true iif successfull
 */

// TODO layer_set_field => call field_free!
static bool probe_buffer_resize(probe_t * probe, size_t size);

//-----------------------------------------------------------
// Static functions (implementation)
//-----------------------------------------------------------

static bool probe_finalize(probe_t * probe)
{
    bool      ret = true;
    size_t    i, num_layers = probe_get_num_layers(probe);
    layer_t * layer;

    // Allow the protocol to do some processing before computing checksums. 
    for (i = 0; i < num_layers; i++) {
        layer = probe_get_layer(probe, i);
        if (layer->protocol && layer->protocol->finalize) {
            if (!(ret &= layer->protocol->finalize(layer->buffer))) {
                fprintf(stderr, "W: Can't finalize layer %s\n", layer->protocol->name);
            }
        }
    }
    return ret;
}

static bool probe_update_protocol(probe_t * probe)
{
    size_t    i, num_layers = probe_get_num_layers(probe);
    layer_t * layer,
            * prev_layer;

    for (i = 0, prev_layer = NULL; i < num_layers; i++, prev_layer = layer) {
        layer = probe_get_layer(probe, i);
        if (layer->protocol && prev_layer) {
            // Update 'protocol' field (if any)
            layer_set_field(layer, I16("protocol", prev_layer->protocol->protocol));
        }
    }
    return true;
}

static bool probe_update_length(probe_t * probe)
{
    size_t    i, length, num_layers = probe_get_num_layers(probe);
    layer_t * layer;

    for (i = 0; i < num_layers; i++) {
        layer = probe_get_layer(probe, i);
        if (layer->protocol) {
            // TODO: length computation should be achieved in the protocol module
            // IPv6 stores in its header (made of 40 bytes) the payload length
            // whereas the other protocol stores the header + the payload length
            length = layer->protocol->protocol == IPPROTO_IPV6 ?
                layer->buffer_size - 40 :
                layer->buffer_size;

            // Update 'length' field (if any)
            layer_set_field(layer, I16("length", length));
        }
    }
    return true;
}

static bool probe_update_checksum(probe_t * probe)
{
    size_t     i, j, num_layers = probe_get_num_layers(probe);
    layer_t  * layer,
             * layer_prev;
    buffer_t * pseudo_header;

    // Update each layers from the (last - 1) one to the first one.
    for (j = 0; j < num_layers; j++) {
        i = num_layers - j - 1;
        layer = probe_get_layer(probe, i);
        if (layer->protocol) { 
            // Does the protocol require a pseudoheader?
            if (layer->protocol->need_ext_checksum) {
                if (i == 0) {
                    // This layer has no previous layer.
                    // We can't compute the corresponding pseudo-header!
                    errno = EINVAL;
                    return false;
                } else {
                    layer_prev = probe_get_layer(probe, i - 1);
                    if (!(pseudo_header = layer->protocol->create_pseudo_header(layer_prev->buffer))) {
                        return false;
                    }
                }
            } else pseudo_header = NULL;

            // Compute the checksum according to the layer's buffer and
            // the pseudo header (if any).
            layer->protocol->write_checksum(layer->buffer, pseudo_header);

            // Release the pseudo header (if any) from the memory
            if (pseudo_header) free(pseudo_header);
        }
    }
    return true;
}


static layer_t * probe_get_layer(const probe_t * probe, size_t i) {
    return dynarray_get_ith_element(probe->layers, i);
}

static layer_t * probe_get_layer_payload(const probe_t * probe) {
    return probe_get_layer(probe, probe_get_num_layers(probe) - 1);
}

static bool probe_push_layer(probe_t * probe, layer_t * layer) {
    // TODO resize probe->bitfield according to the size of the layer
    // TODO fix layer->*size 
    // TODO resize buffer?
    return dynarray_push_element(probe->layers, layer);
}

static bool probe_push_payload(probe_t * probe, size_t payload_size) {
    layer_t * payload_layer,
            * first_layer;
    size_t    current_size;

    // Check whether a payload is already set
    if ((payload_layer = probe_get_layer_payload(probe))) {
        if (!payload_layer->protocol) {
            perror("Payload already set\n");
            goto ERR_PAYLOAD_ALREADY_SET;
        }
    }

    // Allocate the payload layer
    if (!(payload_layer = layer_create())) {
        perror("Can't create layer\n");
        goto ERR_LAYER_CREATE;
    }

    // Retrieve the size of the probe packet before payload addition
    current_size = (first_layer = probe_get_layer(probe, 0)) ?
        first_layer->buffer_size : 0;

    // Initialize the payload layer
    layer_set_buffer(payload_layer, buffer_get_data(probe->buffer) + current_size);
    layer_set_buffer_size(payload_layer, payload_size);
    layer_set_header_size(payload_layer, 0);

    // Add the payload layer in the probe
    if (!(probe_push_layer(probe, payload_layer))) {
        perror("Can't push layer\n");
        goto ERR_PUSH_LAYER;
    }

    // Resize the payload if required
    if (payload_size > 0) {
       if (!probe_payload_resize(probe, payload_size)) {
           perror("Can't resize payload\n");
           goto ERR_PAYLOAD_RESIZE;
       }
    }
    return true;

ERR_PAYLOAD_RESIZE:
    dynarray_del_ith_element(probe->layers, probe_get_num_layers(probe) - 1);
ERR_PUSH_LAYER:
    layer_free(payload_layer);
ERR_LAYER_CREATE:
ERR_PAYLOAD_ALREADY_SET:
    return false;
}

static void probe_layers_free(probe_t * probe) {
    dynarray_free(probe->layers, (ELEMENT_FREE) layer_free);
}

static void probe_layers_clear(probe_t * probe) {
    dynarray_clear(probe->layers, (ELEMENT_FREE) layer_free);
}

static bool probe_buffer_resize(probe_t * probe, size_t size)
{
    size_t    offset = 0, // offset between the begining of the packet and the current position
              i, num_layers = probe_get_num_layers(probe);
    layer_t * layer;

    if (!buffer_resize(probe->buffer, size)) {
        return false;
    }

    // We must reset properly for each layer's buffer
    for (i = 0; i < num_layers; i++) {
        layer = probe_get_layer(probe, i);
        layer_set_buffer(layer, buffer_get_data(probe->buffer) + offset);
        layer_set_buffer_size(layer, size - offset);

        if (layer->protocol) {
            // Update length field (if any)
            if (!layer_set_field(layer, I16("length", size - offset))) {
                return false;
            }
            offset += layer->protocol->header_len; 
        } else {
            // Otherwise, we are at the payload, which is the last layer
            layer_set_header_size(layer, 0);
        }
    }

    return true;
}

//-----------------------------------------------------------
// Allocation 
//-----------------------------------------------------------

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
    if (!(probe->bitfield = bitfield_create(0))) goto ERR_BITFIELD;

    // Save which instance (caller) create this probe
    probe->caller = NULL;
    return probe;

ERR_BITFIELD:
    probe_layers_free(probe);
ERR_LAYERS:
    buffer_free(probe->buffer);
ERR_BUFFER:
    free(probe);
ERR_PROBE:
    return NULL;
}

probe_t * probe_dup(probe_t * probe)
{
    probe_t    * ret;
    buffer_t   * buffer;
    bitfield_t * bitfield;
    
    if (!(ret = probe_create()))                     goto ERR_PROBE;
    if (!(buffer = buffer_dup(probe->buffer)))       goto ERR_BUFFER_DUP;
    if (!(probe_set_buffer(ret, buffer)))            goto ERR_SET_BUFFER;
    if (!(bitfield = bitfield_dup(probe->bitfield))) goto ERR_BITFIELD_DUP;
    if (!(ret->bitfield = bitfield))                 goto ERR_BITFIELD;

    ret->caller = probe->caller;
    return ret;

ERR_BITFIELD:
    bitfield_free(bitfield);
ERR_BITFIELD_DUP:
ERR_SET_BUFFER:
    buffer_free(ret->buffer);
ERR_BUFFER_DUP:
    probe_free(ret);
ERR_PROBE:
    return NULL;
}

void probe_free(probe_t * probe)
{
    if (probe) {
        bitfield_free(probe->bitfield);
        probe_layers_free(probe);
        buffer_free(probe->buffer);
        free(probe);
    }
}

void probe_dump(const probe_t * probe)
{
    size_t    i, num_layers = probe_get_num_layers(probe); 
    layer_t * layer;

    printf("\n\n** PROBE **\n\n");
    for (i = 0; i < num_layers; i++) {
        layer = probe_get_layer(probe, i);
        layer_dump(layer, i);
        printf("\n");
    }
    printf("\n");
}

//-----------------------------------------------------------
// Buffer management 
//-----------------------------------------------------------

inline buffer_t * probe_get_buffer(const probe_t * probe) {
    return probe ? probe->buffer : NULL;
}

bool probe_set_buffer(probe_t * probe, buffer_t * buffer)
{
    size_t       size; // to prevent underflow
    size_t       offset;
    uint8_t    * data;
    const protocol_t * protocol;
    uint8_t      protocol_id,
                 ipv4_protocol_id = 4;

    probe->buffer = buffer;
    data = buffer_get_data(buffer);
    size = buffer_get_size(buffer);

    probe_layers_clear(probe);

    unsigned char ip_version = buffer_guess_ip_version(buffer);
             
////////////////////////:
    protocol = ip_version == 6 ? protocol_search("ipv6") :
               ip_version == 4 ? protocol_search("ipv4") :
               NULL;
    if (!protocol) {
        perror("E: probe_set_buffer: cannot guess IP version");
        return false;
    }
    protocol_id = protocol-> protocol;
///////////////

    offset = 0;

    for(;;) {
        layer_t       * layer;
        const field_t * field;
        size_t          hdrlen;
        //printf("-------> coucou\n");

        layer = layer_create();
        layer_set_buffer(layer, data + offset);
        layer_set_buffer_size(layer, size);

        if (!probe_push_layer(probe, layer)) return false;

        protocol = protocol_search_by_id(protocol_id);
        if (!protocol)
            return false; // Cannot find matching protocol

        hdrlen = protocol->header_len;

        layer_set_protocol(layer, protocol);
        layer_set_header_size(layer, hdrlen);

        offset += hdrlen;
        size -= hdrlen;
        if (size < 0)
            return false;

        // In the case of ICMP, while protocol is not really a field, we might
        // provide it by convenience
        // Need for heuristics // source port hook to parse packet content

        // Loop until reaching the payload or an ICMP layer
        // layer_create_field returns NULL iif we've reached an ICMP layer or the payload 
        field = layer_create_field(layer, "protocol");
        if (field) {
            protocol_id = field->value.int8;
            continue;
        } //else {
            //protocol_id = 6; // ICMP6 + IPv6
        //} 

        else if (strcmp(layer->protocol->name, "icmp") == 0) {
            // We are in an ICMP layer
            field = layer_create_field(layer, "type");

            if (!field) {
                // Weird ICMP packet !
                return false; 
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
        layer_t * layer = layer_create();
        layer_set_buffer(layer, data + offset);
        layer_set_buffer_size(layer, size);
        layer_set_header_size(layer, 0);
        if (!probe_push_layer(probe, layer)) return false;
    }
    return true; 
}

//-----------------------------------------------------------
// Layer management 
//-----------------------------------------------------------

size_t probe_get_num_layers(const probe_t * probe) {
    return dynarray_get_size(probe->layers); 
}

uint8_t * probe_get_payload(const probe_t * probe) {
    const layer_t * layer = probe_get_layer_payload(probe);
    return layer ? layer->buffer : NULL;
}

size_t probe_get_payload_size(const probe_t * probe) {
    const layer_t * layer = probe_get_layer_payload(probe);
    return layer ? layer->buffer_size : 0;
}

const char * probe_get_protocol_name(const probe_t * probe, size_t i) {
    if (i + 1 == probe_get_num_layers(probe)) return "payload";
    const layer_t * layer = probe_get_layer(probe, i); 
    return layer ? layer->protocol->name : NULL; 
}

int probe_set_protocols(probe_t * probe, const char * name1, ...)
{
    // TODO A similar function should allow hooking into the layer structure
    // and not at the top layer

    va_list            args, args2;
    size_t             buflen, offset;
    const char       * name;
    layer_t          * layer, *prev_layer;
    const protocol_t * protocol;

    // Remove the former layer structure
    probe_layers_clear(probe);

    // Set up the new layer structure
    va_start(args, name1);

    // Allocate the buffer according to the layer structure
    buflen = 0;
    va_copy(args2, args);
    for (name = name1; name; name = va_arg(args2, char *)) {
        if (!(protocol = protocol_search(name))) goto ERR_PROTOCOL_SEARCH;
        buflen += protocol->header_len; 
    }
    va_end(args2);
    buffer_resize(probe->buffer, buflen);

    // Create each layer
    offset = 0;
    prev_layer = NULL;
    for (name = name1; name; name = va_arg(args, char *)) {
        // Associate protocol to the layer
        layer = layer_create();
        protocol = protocol_search(name);
        if (!protocol) goto ERR_LAYER;

        layer_set_protocol(layer, protocol);

        // Initialize the buffer with default protocol values
        protocol->write_default_header(buffer_get_data(probe->buffer) + offset);
        layer_set_header_size(layer, protocol->header_len);

        // TODO consider variable length headers
        layer_set_buffer(layer, buffer_get_data(probe->buffer) + offset);
        layer_set_buffer_size(layer, buflen - offset);
//        layer_set_mask(layer, bitfield_get_mask(probe->bitfield) + offset);

        // Update 'length' field
        if (!layer_set_field(layer, I16("length", buflen - offset))) {
            fprintf(stderr, "Can't set length in %s header\n", layer->protocol->name);
            goto ERR_SET_LENGTH;
        }

        if (prev_layer) {
            // Update 'protocol' field (if any)
            layer_set_field(layer, I16("protocol", prev_layer->protocol->protocol));
        }

        offset += protocol->header_len; 
        if (!probe_push_layer(probe, layer)) {
            perror("Can't add protocol layer\n");
            goto ERR_PUSH_LAYER;
        }
        prev_layer = layer;
    }
    va_end(args);

    // Payload : initially empty
    if (!probe_push_payload(probe, 0)) {
        perror("Can't push payload\n");
        goto ERR_PUSH_PAYLOAD;
    }

    // Size and checksum are pending, they depend on payload 
    return 0;

ERR_PUSH_PAYLOAD:
ERR_PUSH_LAYER:
ERR_SET_LENGTH:
    layer_free(layer);
ERR_LAYER:
    probe_layers_clear(probe);
ERR_PROTOCOL_SEARCH:
    return -1;
}

bool probe_payload_resize(probe_t * probe, size_t payload_size)
{
    layer_t * payload_layer;
    size_t    old_buffer_size,
              new_buffer_size,
              old_payload_size;
    
    if (!(payload_layer = probe_get_layer_payload(probe))) goto ERR_NO_PAYLOAD; 

    old_payload_size = layer_get_buffer_size(payload_layer);

    // Compare payload lenths
    if (old_payload_size != payload_size) {
        old_buffer_size = buffer_get_size(probe->buffer);
        if (old_payload_size > old_buffer_size) {
            perror("Invalid probe buffer\n");
            goto ERR_INVALID_PROBE_BUFFER;
        }
        new_buffer_size = old_buffer_size - old_payload_size + payload_size;

        // Resize the buffer
        if (!(probe_buffer_resize(probe, new_buffer_size))) goto ERR_RESIZE_BUFFER;

        // Update 'checksum' and 'length' fields to remain the probe consistant
        probe_update_fields(probe);
    }
    return true;

ERR_INVALID_PROBE_BUFFER:
ERR_RESIZE_BUFFER:
ERR_NO_PAYLOAD:
    return false;
}

bool probe_set_payload(probe_t *probe, buffer_t * payload) {
    return probe_write_payload(probe, payload, 0);
}

bool probe_write_payload(probe_t * probe, buffer_t * payload, unsigned int offset)
{
    layer_t * payload_layer;
    size_t    payload_size = buffer_get_size(payload);

    return (payload_layer = probe_get_layer_payload(probe))
        && (probe_payload_resize(probe, payload_size))
        && (layer_write_payload(payload_layer, payload, offset));
}

//-----------------------------------------------------------
// Fields management 
//-----------------------------------------------------------

bool probe_update_fields(probe_t * probe)
{
    return probe_finalize(probe)
        && probe_update_protocol(probe)
        && probe_update_length(probe)
        && probe_update_checksum(probe);
}

bool probe_set_field_ext(probe_t * probe, size_t depth, field_t * field)
{
    bool      ret = false;
    size_t    i, num_layers = probe_get_num_layers(probe);
    layer_t * layer;

    for(i = depth; i < num_layers; i++) {
        layer = probe_get_layer(probe, i);
        if (layer_set_field(layer, field)) {
            ret = true;
            break;
        }
    }
    return ret;
}

bool probe_set_field(probe_t * probe, field_t * field) {
    return probe_set_field_ext(probe, 0, field);
}

bool probe_set_metafield_ext(probe_t * probe, size_t depth, field_t * field)
{
    metafield_t * metafield;

    // TODO: TEMP HACK IPv4 flow id is encoded in src_port
    if (strcmp(field->key, "flow_id") != 0) return false;
    return probe_set_field(probe, I16("src_port", 24000 + field->value.int16));
    
    metafield = metafield_search(field->key);
    if (!metafield) return false; // Metafield not found

    // Does the probe verifies one metafield pattern ?
    // TODO
    // Does the value conflict with a previously set field ?
    // TODO
}

bool probe_set_metafield(probe_t * probe, field_t * field) {
    return probe_set_metafield_ext(probe, 0, field);
}

// Internal use
field_t * probe_create_metafield_ext(const probe_t * probe, const char * name, size_t depth)
{
    uint16_t        src_port; 

    // TODO to generalize to any metafield
    if (strcmp(name, "flow_id") != 0) return NULL;

    // TODO We've hardcoded the flow-id in the src_port and we only support the "flow_id" metafield
    // TODO to adapt for IPv6 support 
    return probe_extract(probe, "src_port", &src_port) ?
        IMAX("flow_id", src_port - 24000) :
        NULL;
}

const field_t * probe_create_metafield(const probe_t * probe, const char * name) {
    return probe_create_metafield_ext(probe, name, 0);
}

bool probe_set_fields(probe_t * probe, field_t * field1, ...) {
    va_list   args;
    field_t * field;
    bool      ret = true;

//    printf("111111111111111111111111111111111\n");
//    probe_dump(probe);

    va_start(args, field1);
    for (field = field1; field; field = va_arg(args, field_t *)) {
        // Update the first matching field
        if (!probe_set_field(probe, field)) { 
            // No matching field found, update the first matching metafield
            if (!probe_set_metafield(probe, field)) { 
                // No matching {field, metafield}, ignore this field.
                ret = false;
                fprintf(stderr, "probe_set_fields: Cannot not set field %s\n", field->key);
            }
        }
        field_free(field);
    }
    va_end(args);
    probe_update_fields(probe);

    printf("111111111111111\n");
    probe_dump(probe);

    return ret;
}

void probe_set_caller(probe_t * probe, void * caller) {
    probe->caller = caller;
}

void * probe_get_caller(const probe_t * probe) {
    return probe->caller;
}

void probe_set_sending_time(probe_t * probe, double time) {
    probe->sending_time = time;
}

double probe_get_sending_time(const probe_t * probe) {
    return probe->sending_time;
}

void probe_set_queueing_time(probe_t * probe, double time) {
    probe->queueing_time = time;
}

double probe_get_queueing_time(const probe_t *probe) {
    return probe->queueing_time;
}

// Iterator

typedef struct {
    void  * data;
    void (* callback)(field_t * field, void * data);
} iter_fields_data_t;

void probe_iter_fields_callback(void * element, void * data) {
    iter_fields_data_t * d = (iter_fields_data_t*) data;
    d->callback((field_t*) element, d->data);
}

void probe_iter_fields(probe_t *probe, void * data, void (*callback)(field_t *, void *))
{
    /*
    iter_fields_data_t tmp = {
        .data = data,
        .callback = callback
    };
    */
    
    // not implemented : need to iter over protocol fields of each layer
}

field_t * probe_create_field_ext(const probe_t * probe, const char * name, size_t depth)
{
    size_t    i, num_layers = probe_get_num_layers(probe); 
    layer_t * layer;
    field_t * field;

    // We go through the layers until we get the required field
    for(i = depth; i < num_layers; i++) {
        layer = probe_get_layer(probe, i);
        if ((field = layer_create_field(layer, name))) {
            return field;
        }
    }

    // No matching field found, this is maybe a metafield
    return probe_create_metafield_ext(probe, name, depth);
}

field_t * probe_create_field(const probe_t * probe, const char * name) {
    return probe_create_field_ext(probe, name, 0);
}

bool probe_extract_ext(const probe_t * probe, const char * name, size_t depth, void * dst) {
    field_t *  field;

    if (!(field = probe_create_field_ext(probe, name, depth))) goto ERR_CREATE_FIELD;

    if (field->type == TYPE_STRING) {
        *((char **) dst) = strdup(field->value.string);
    } else {
        memcpy(dst, &field->value, field_get_size(field));
    } 
    field_free(field);
    return true;

ERR_CREATE_FIELD:
    return false;
}

bool probe_extract(const probe_t * probe, const char * name, void * dst) {
    return probe_extract_ext(probe, name, 0, dst);
}

/******************************************************************************
 * probe_reply_t
 ******************************************************************************/

probe_reply_t * probe_reply_create(void) {
    return calloc(1, sizeof(probe_reply_t));
}

void probe_reply_free(probe_reply_t * probe_reply) {
    if (probe_reply) free(probe_reply);
}

// Accessors

void probe_reply_set_probe(probe_reply_t * probe_reply, probe_t * probe) {
    probe_reply->probe = probe;
}

probe_t * probe_reply_get_probe(const probe_reply_t * probe_reply) {
    return probe_reply->probe;
}

void probe_reply_set_reply(probe_reply_t * probe_reply, probe_t * reply) {
    probe_reply->reply = reply;
}

probe_t * probe_reply_get_reply(const probe_reply_t * probe_reply) {
    return probe_reply->reply;
}
