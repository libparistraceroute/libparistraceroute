#include "use.h"
#include "config.h"

#include <stdlib.h>         // malloc, free
#include <stdio.h>          // perror
#include <errno.h>          // errno, EINVAL
#include <stdarg.h>         // va_start, va_copy, va_arg
#include <string.h>         // memcpy
#include <sys/socket.h>     // AF_INET*

#include "probe.h"          // probe_t
#include "buffer.h"         // buffer_t
#include "protocol.h"       // protocol_t
#include "common.h"         // ELEMENT_FREE
#include "generator.h"      // generator_*

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
 * \brief Update for each layer of a probe its 'protocol' field
 *   (if any) in order to have a coherent sequence of layers.
 * \param probe The probe we're updating
 * \return true iif successfull
 */

static bool probe_update_protocol(probe_t * probe);

/**
 * \brief Update for each layer of a probe its 'length' field
 *   (if any) in order to have a coherent sequence of layers.
 * \param probe The probe we're updating
 * \return true iif successfull
 */

static bool probe_update_length(probe_t * probe);

//-----------------------------------------------------------
// Layers management
//-----------------------------------------------------------

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
 * \brief Resize the packet managed by a probe instance.
 *   Update nested layer pointers and sizes consequently.
 * \param probe The probe we're updating
 * \param size The new packet size
 * \return true iif successfull
 */

static bool probe_packet_resize(probe_t * probe, size_t size);

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
            if (!(ret &= layer->protocol->finalize(layer->segment))) {
                fprintf(stderr, "W: Can't finalize layer %s\n", layer->protocol->name);
            }
        }
    }
    return ret;
}

static bool layer_set_field_and_free(layer_t * layer, field_t * field) {
    bool ret = false;

    if (field) {
        ret = layer_set_field(layer, field);
        field_free(field);
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
            layer_set_field_and_free(layer, I8("protocol", prev_layer->protocol->protocol));
        }
    }
    return true;
}

static bool probe_update_length(probe_t * probe)
{
    bool      ret = true;
    size_t    i, offset,
              num_layers = probe_get_num_layers(probe),
              packet_size = probe_get_size(probe);
    layer_t * layer;

    for (i = 0, offset = 0; i < num_layers; i++) {
        layer = probe_get_layer(probe, i);
        if (layer->protocol) {
            // Update 'length' field (if any)
            // This protocol field must always corresponds to the size of the
            // header + its contents.
            layer_set_field_and_free(layer, I16("length", packet_size - offset));
            offset += layer->protocol->get_header_size(layer->segment);
        } else {
            // Update payload size
            layer_set_segment_size(layer, packet_size - offset);
        }
    }

    return ret;
}

bool probe_update_checksum(probe_t * probe)
{
    size_t     i, j, num_layers = probe_get_num_layers(probe);
    layer_t  * layer,
             * layer_prev;
    buffer_t * pseudo_header;

    // Update each layers from the (last - 1) one to the first one.
    for (j = 0; j < num_layers; j++) {
        i = num_layers - j - 1;
        layer = probe_get_layer(probe, i);

        // Does the protocol require a pseudoheader to compute its checksum?
        if (layer->protocol && layer->protocol->write_checksum) {

            // Compute the checksum according to the layer's buffer and
            // the pseudo header (if any).
            if (layer->protocol->create_pseudo_header) {
                if (i == 0) {
                    // This layer has no previous layer which is required to compute its checksum.
                    fprintf(stderr, "No previous layer which is required to compute '%s' checksum\n", layer->protocol->name);
                    errno = EINVAL;
                    return false;
                } else {
                    layer_prev = probe_get_layer(probe, i - 1);

                    if (strncmp(layer_prev->protocol->name, "ipv", 3) != 0) {
                        fprintf(stderr,
                            "Trying to calculate %s checksum but the previous layer is not an IP layer (%s)\n",
                            layer->protocol->name,
                            layer_prev->protocol->name
                        );
                        return false;
                    }

                    if (!(pseudo_header = layer->protocol->create_pseudo_header(layer_prev->segment))) {
                        return false;
                    }
                }
            } else pseudo_header = NULL;

            // Update the checksum of this layer
            if (!layer->protocol->write_checksum(layer->segment, pseudo_header)) {
                fprintf(stderr, "Error while updating checksum (layer %s)\n", layer->protocol->name);
                return false;
            }

            // Release the pseudo header (if any) from the memory
            if (pseudo_header) buffer_free(pseudo_header);
        }
    }
    return true;
}

layer_t * probe_get_layer(const probe_t * probe, size_t i) {
    return dynarray_get_ith_element(probe->layers, i);
}

layer_t * probe_get_layer_payload(const probe_t * probe) {
    size_t    num_layers = probe_get_num_layers(probe);
    layer_t * last_layer;

    if (num_layers == 0) {
        fprintf(stderr, "probe_get_layer_payload: No layer in this probe!\n");
        return NULL;
    }

    last_layer = probe_get_layer(probe, num_layers - 1);
    return last_layer && last_layer->protocol ? NULL : last_layer;
}

static bool probe_push_layer(probe_t * probe, layer_t * layer) {
    return dynarray_push_element(probe->layers, layer);
}

static bool probe_push_payload(probe_t * probe, size_t payload_size) {
    layer_t * payload_layer,
            * first_layer;
    uint8_t * payload_bytes;
    size_t    packet_size;

    // Check whether a payload is already set
    if ((payload_layer = probe_get_layer_payload(probe))) {
        fprintf(stderr, "Payload already set\n");
        goto ERR_PAYLOAD_ALREADY_SET;
    }

    // Get the first protocol layer
    if (!(first_layer = probe_get_layer(probe, 0))) {
        fprintf(stderr, "No protocol layer defined in this probe\n");
        goto ERR_NO_FIRST_LAYER;
    }

    // The first segment stores the packet size, thus:
    // @payload = @first_segment + packet_size - payload_size
    packet_size = probe_get_size(probe) ;
    payload_bytes = packet_get_bytes(probe->packet) + packet_size - payload_size;

    if (!(payload_layer = layer_create_from_segment(NULL, payload_bytes, payload_size))) {
        goto ERR_LAYER_CREATE;
    }

    // Add the payload layer in the probe
    if (!(probe_push_layer(probe, payload_layer))) {
        fprintf(stderr, "Can't push payload layer\n");
        goto ERR_PUSH_LAYER;
    }

    // Resize the payload if required
    if (payload_size > 0) {
        if (!probe_payload_resize(probe, payload_size)) {
            fprintf(stderr, "Can't resize payload\n");
            goto ERR_PAYLOAD_RESIZE;
        }
    }

    return true;

ERR_PAYLOAD_RESIZE:
    dynarray_del_ith_element(probe->layers, probe_get_num_layers(probe) - 1, NULL);
ERR_PUSH_LAYER:
    layer_free(payload_layer);
ERR_LAYER_CREATE:
ERR_NO_FIRST_LAYER:
ERR_PAYLOAD_ALREADY_SET:
    return false;
}

static void probe_layers_free(probe_t * probe) {
    dynarray_free(probe->layers, (ELEMENT_FREE) layer_free);
}

static void probe_layers_clear(probe_t * probe) {
    dynarray_clear(probe->layers, (ELEMENT_FREE) layer_free);
}

static bool probe_packet_resize(probe_t * probe, size_t size) {
    size_t    offset = 0, // offset between the begining of the packet and the current position
              i, num_layers = probe_get_num_layers(probe);
    layer_t * layer;
    uint8_t * segment;

    if (!packet_resize(probe->packet, size)) {
        return false;
    }

    // TODO update bitfield

    // Update each layer's segment
    for (i = 0; i < num_layers; i++) {
        layer = probe_get_layer(probe, i);
        segment = packet_get_bytes(probe->packet) + offset;
        layer_set_segment(layer, segment);

        if (layer->protocol) {
            // We're in a layer related to a protocol. Update "length" field (if any).
            // It concerns: ipv4, ipv6, udp but not tcp, icmpv4, icmpv6
            layer_set_field_and_free(layer, I16("length", size - offset));
            offset += layer->segment_size;
        }
    }

    return true;
}

//-----------------------------------------------------------
// Allocation
//-----------------------------------------------------------

probe_t * probe_create()
{
    probe_t * probe;

    // We calloc probe to set *_time and caller members to 0
    if (!(probe = calloc(1, sizeof(probe_t))))   goto ERR_PROBE;
    if (!(probe->packet = packet_create())) {
        fprintf(stderr, "Cannot create packet\n");
        goto ERR_PACKET;
    }
    if (!(probe->layers = dynarray_create()))    goto ERR_LAYERS;
//    if (!(probe->bitfield = bitfield_create(0))) goto ERR_BITFIELD;
    probe_set_left_to_send(probe, 1);
    return probe;

    /*
ERR_BITFIELD:
    probe_layers_free(probe);
    */
ERR_LAYERS:
    packet_free(probe->packet);
ERR_PACKET:
    free(probe);
ERR_PROBE:
    return NULL;
}

probe_t * probe_dup(const probe_t * probe) {
    probe_t  * ret;
    packet_t * packet;

    if (!(packet = packet_dup(probe->packet)))            goto ERR_PACKET_DUP;
    if (!(ret = probe_wrap_packet(packet)))               goto ERR_PROBE_WRAP_PACKET;
//    if (!(ret->bitfield = bitfield_dup(probe->bitfield))) goto ERR_BITFIELD_DUP;

    ret->sending_time  = probe->sending_time;
    ret->queueing_time = probe->queueing_time;
    ret->recv_time     = probe->recv_time;
    ret->caller        = probe->caller;
#ifdef USE_SCHEDULING
    ret->delay         = probe->delay ? field_dup(probe->delay): NULL;
#endif
    return ret;

    /*
ERR_BITFIELD_DUP:
    probe_free(ret);
    packet = NULL;
    */
ERR_PROBE_WRAP_PACKET:
    if (packet) packet_free(packet);
ERR_PACKET_DUP:
    return NULL;
}

void probe_free(probe_t * probe) {
    if (probe) {
//        bitfield_free(probe->bitfield);
        probe_layers_free(probe);
        if (probe->packet) {
            packet_free(probe->packet);
        }
        free(probe);
    }
}

void probe_fprintf(FILE * out, const probe_t * probe) {
    size_t    i, num_layers = probe_get_num_layers(probe);
    layer_t * layer;

    fprintf(out, "** PROBE **\n\n");

    if (probe->delay) {
        fprintf(out, "probe delay \n\n");
        field_dump(probe->delay);
        fprintf(out, "number of probes left to send: (%d) \n\n", (int)probe->left_to_send);
        fprintf(out, "probe structure\n\n");
    }

    for (i = 0; i < num_layers; i++) {
        layer = probe_get_layer(probe, i);
        layer_dump(layer, i);
        fprintf(out, "\n");
    }

    fprintf(out, "\n");
}

void probe_dump(const probe_t * probe) {
    probe_fprintf(stdout, probe);
}

void probe_debug(const probe_t * probe) {
    size_t    i, num_layers = probe_get_num_layers(probe);
    layer_t * layer1,
            * layer2;
    probe_t * probe_should_be;

    if ((probe_should_be = probe_dup(probe))) {
        // Compute expected values
        probe_update_fields(probe_should_be);

        // Dump fields
        printf("** PROBE **\n\n");
        for (i = 0; i < num_layers; i++) {
            layer1 = probe_get_layer(probe, i);
            layer2 = probe_get_layer(probe_should_be, i);
            layer_debug(layer1, layer2, i);
            printf("\n");
        }
        printf("\n");

        probe_free(probe_should_be);
    }
}

//-----------------------------------------------------------
// Buffer management
//-----------------------------------------------------------

// TODO use instanceof callback, see protocols/*.c
static const protocol_t * get_first_protocol(const packet_t * packet) {
    const protocol_t * protocol = NULL;

    switch (packet_guess_address_family(packet)) {
        case AF_INET:
            protocol = protocol_search("ipv4");
            break;
        case AF_INET6:
            protocol = protocol_search("ipv6");
            break;
        default:
            fprintf(stderr, "Cannot guess Internet address family\n");
            break;
    }
    return protocol;
}

probe_t * probe_wrap_packet(packet_t * packet)
{
    probe_t          * probe;
    size_t             segment_size, remaining_size;
    layer_t          * layer;
    uint8_t          * segment;
    const protocol_t * protocol;

    if (!(probe = probe_create())) {
        goto ERR_PROBE_CREATE;
    }

    // Clear the probe
    packet_free(probe->packet);
    probe->packet = packet;
    probe_layers_clear(probe);

    // Prepare iteration
    segment = packet_get_bytes(probe->packet);
    remaining_size = packet_get_size(probe->packet);

    // Push layers
    for (protocol = get_first_protocol(packet); protocol; protocol = protocol->get_next_protocol(layer)) {
        if (remaining_size < protocol->write_default_header(NULL)) {
            // Not enough bytes left for the header, packet is truncated
            segment_size = remaining_size;
        } else {
            segment_size = protocol->get_header_size(segment);
        }

        if (!(layer = layer_create_from_segment(protocol, segment, segment_size))) {
            goto ERR_CREATE_LAYER;
        }

        if (!probe_push_layer(probe, layer)) {
            goto ERR_PUSH_LAYER;
        }

        segment += segment_size;
        remaining_size -= segment_size;
        if (remaining_size < 0) {
            fprintf(stderr, "probe_wrap_packet: Truncated packet\n");
            goto ERR_TRUNCATED_PACKET;
        }

        if (!protocol->get_next_protocol) {
            break;
        }
        continue;

ERR_TRUNCATED_PACKET:
ERR_PUSH_LAYER:
        layer_free(layer);
ERR_CREATE_LAYER:
        goto ERR_LAYER_DISCOVER_LAYER;
    }

    // Rq: Some packets (e.g ICMP type 3) do not have payload.
    // In this case we push an empty payload
    probe_push_payload(probe, remaining_size);

    return probe;

ERR_LAYER_DISCOVER_LAYER:
    probe_free(probe);
ERR_PROBE_CREATE:
    return NULL;
}

//-----------------------------------------------------------
// Layer management
//-----------------------------------------------------------

size_t probe_get_num_layers(const probe_t * probe) {
    return dynarray_get_size(probe->layers);
}

uint8_t * probe_get_payload(const probe_t * probe) {
    const layer_t * layer = probe_get_layer_payload(probe);
    return layer ? layer_get_segment(layer) : NULL;
}

size_t probe_get_payload_size(const probe_t * probe) {
    const layer_t * layer = probe_get_layer_payload(probe);
    return layer ? layer_get_segment_size(layer) : 0;
}

const char * probe_get_protocol_name(const probe_t * probe, size_t i) {
    if (i + 1 == probe_get_num_layers(probe)) return "payload";
    const layer_t * layer = probe_get_layer(probe, i);
    return layer ? layer->protocol->name : NULL;
}

bool probe_set_protocols(probe_t * probe, const char * name1, ...)
{
    // TODO A similar function should allow hooking into the layer structure
    // and not at the top layer

    va_list            args, args2;
    size_t             packet_size, offset, segment_size;
    const char       * name;
    layer_t          * layer = NULL,
                     * prev_layer;
    const protocol_t * protocol;

    // Remove the former layer structure
    probe_layers_clear(probe);

    // Set up the new layer structure
    va_start(args, name1);

    // Allocate the buffer according to the layer structure
    packet_size = 0;
    va_copy(args2, args);
    for (name = name1; name; name = va_arg(args2, char *)) {
        if (!(protocol = protocol_search(name))) {
            fprintf(stderr, "Cannot find %s protocol, known protocols are:", name);
            protocols_dump();
            goto ERR_PROTOCOL_SEARCH;
        }
        packet_size += protocol->write_default_header(NULL);
    }
    va_end(args2);
    if (!(packet_resize(probe->packet, packet_size))) goto ERR_PACKET_RESIZE;

    // Create each layer
    offset = 0;
    prev_layer = NULL;
    for (name = name1; name; name = va_arg(args, char *)) {
        // Associate protocol to the layer
        if (!(protocol = protocol_search(name))) goto ERR_PROTOCOL_SEARCH2;
        segment_size = protocol->write_default_header(packet_get_bytes(probe->packet) + offset);

        if (!(layer = layer_create_from_segment(protocol, packet_get_bytes(probe->packet) + offset, segment_size))) {
            fprintf(stderr, "Can't create segment for %s header\n", layer->protocol->name);
            goto ERR_LAYER_CREATE;
        }
        // TODO layer_set_mask(layer, bitfield_get_mask(probe->bitfield) + offset);

        // Update 'length' field (if any). It concerns IPv* and UDP, but not TCP or ICMPv*
        layer_set_field_and_free(layer, I16("length", packet_size - offset));

        // Update 'protocol' field of the previous inserted layer (if any)
        if (prev_layer) {
            if (!layer_set_field_and_free(prev_layer, I8("protocol", layer->protocol->protocol))) {
                fprintf(stderr, "Can't set 'protocol' in %s header\n", layer->protocol->name);
                goto ERR_SET_PROTOCOL;
            }
        }

        offset += layer->segment_size;
        if (!probe_push_layer(probe, layer)) {
            fprintf(stderr, "Can't add protocol layer\n");
            goto ERR_PUSH_LAYER;
        }
        prev_layer = layer;
    }
    va_end(args);
    layer = NULL;

    // Payload : initially empty
    if (!probe_push_payload(probe, 0)) {
        goto ERR_PUSH_PAYLOAD;
    }

    // Size and checksum are pending, they depend on payload
    return true;

ERR_PUSH_PAYLOAD:
ERR_PUSH_LAYER:
ERR_SET_PROTOCOL:
    if (layer) layer_free(layer);
ERR_LAYER_CREATE:
ERR_PROTOCOL_SEARCH2:
    probe_layers_clear(probe);
ERR_PACKET_RESIZE:
ERR_PROTOCOL_SEARCH:
    return false;
}

size_t probe_get_size(const probe_t * probe) {
    return packet_get_size(probe->packet);
}

bool probe_payload_resize(probe_t * probe, size_t payload_size)
{
    layer_t * payload_layer;
    size_t    old_packet_size,
              new_packet_size,
              old_payload_size;

    if (!(payload_layer = probe_get_layer_payload(probe))) goto ERR_NO_PAYLOAD;

    old_payload_size = layer_get_segment_size(payload_layer);

    // Compare payload lengths
    if (old_payload_size != payload_size) {
        old_packet_size = probe_get_size(probe);
        if (old_payload_size > old_packet_size) {
            perror("Invalid probe buffer\n");
            goto ERR_INVALID_PROBE_BUFFER;
        }
        new_packet_size = old_packet_size - old_payload_size + payload_size;

        // Resize the buffer
        if (!(probe_packet_resize(probe, new_packet_size))) goto ERR_PACKET_RESIZE;

        // Update 'checksum' and 'length' fields to remain the probe consistant
        probe_update_fields(probe);
    }
    return true;

ERR_INVALID_PROBE_BUFFER:
ERR_PACKET_RESIZE:
ERR_NO_PAYLOAD:
    return false;
}

bool probe_write_payload(probe_t * probe, const void * bytes, size_t num_bytes) {
    return probe_write_payload_ext(probe, bytes, num_bytes, 0);
}

bool probe_write_payload_ext(probe_t * probe, const void * bytes, size_t num_bytes, size_t offset)
{
    layer_t * payload_layer;

    if (!(payload_layer = probe_get_layer_payload(probe))) {
        goto ERR_PROBE_GET_LAYER_PAYLOAD;
    }

    if (num_bytes > probe_get_payload_size(probe)) {
        if(!probe_payload_resize(probe, num_bytes)) {
            goto ERR_PROBE_PAYLOAD_RESIZE;
        }
    }

    if (!layer_write_payload_ext(payload_layer, bytes, num_bytes, offset)) {
        goto ERR_LAYER_WRITE_PAYLOAD_EXT;
    }

    return true;

ERR_LAYER_WRITE_PAYLOAD_EXT:
ERR_PROBE_PAYLOAD_RESIZE:
ERR_PROBE_GET_LAYER_PAYLOAD:
    return false;
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

bool probe_set_field_ext(probe_t * probe, size_t depth, const field_t * field)
{
    bool      ret = false;
    size_t    i, num_layers = probe_get_num_layers(probe);
    layer_t * layer;

    for (i = depth; i < num_layers; i++) {
        layer = probe_get_layer(probe, i);
        if (layer_set_field(layer, field)) { // TODO: replace by layer_set_field_and_free
            ret = true;
            break;
        }
    }
    return ret;
}

bool probe_set_field(probe_t * probe, const field_t * field) {
    return probe_set_field_ext(probe, 0, field);
}

bool probe_write_field_ext(probe_t * probe, size_t depth, const char * name, void * bytes, size_t num_bytes) {
    bool      ret = false;
    size_t    i, num_layers = probe_get_num_layers(probe);
    layer_t * layer;

    for (i = depth; i < num_layers; i++) {
        layer = probe_get_layer(probe, i);
        if (layer_write_field(layer, name, bytes, num_bytes)) {
            ret = true;
            break;
        }
    }
    return ret;
}

bool probe_write_field(probe_t * probe, const char * name, void * bytes, size_t num_bytes) {
    return probe_write_field_ext(probe, 0, name, bytes, num_bytes);
}

bool probe_set_metafield_ext(probe_t * probe, size_t depth, field_t * field)
{
    bool          ret = true;
    field_t     * hacked_field;

    // TODO: TEMP HACK IPv4 flow id is encoded in src_port
    if (strcmp(field->key, "flow_id") != 0) {
        fprintf(stderr, "probe_set_metafield_ext: cannot set %s\n", field->key);
        return false;
    }

    // We add 24000 to use port to increase chances to traverse firewalls
    if ((hacked_field = I16("src_port", 24000 + field->value.int16))) {
        ret = probe_set_field(probe, hacked_field);
        field_free(hacked_field);
    }

    return ret;

    /*
    metafield = metafield_search(field->key);
    if (!metafield) return false; // Metafield not found

    // Does the probe verifies one metafield pattern ?
    // Does the value conflict with a previously set field ?
    */
}

bool probe_set_metafield(probe_t * probe, field_t * field) {
    return probe_set_metafield_ext(probe, 0, field);
}

// Internal use
static field_t * probe_create_metafield_ext(const probe_t * probe, const char * name, size_t depth)
{
    uint16_t src_port;

    // TODO to generalize to any metafield
    if (strcmp(name, "flow_id") != 0) return NULL;

    // TODO We've hardcoded the flow-id in the src_port and we only support the "flow_id" metafield
    // In IPv6, flow_id should be set thanks to probe_set_field
    // We substract 24000 to the port (see probe_set_metafield_ext)
    return probe_extract(probe, "src_port", &src_port) ?
        IMAX("flow_id", src_port - 24000) :
        NULL;
}

field_t * probe_create_metafield(const probe_t * probe, const char * name) {
    return probe_create_metafield_ext(probe, name, 0);
}

bool probe_set_fields(probe_t * probe, field_t * field1, ...) {
    va_list   args;
    field_t * field;
    bool      ret = true;

    va_start(args, field1);
    for (field = field1; field; field = va_arg(args, field_t *)) {
        // Update the first matching field
        if (!probe_set_field(probe, field)) {
            // No matching field found, update the first matching metafield
            if (!(ret &= probe_set_metafield(probe, field))) {
                fprintf(stderr, "probe_set_fields: Cannot set field '%s'\n", field->key);
            }
        }
        field_free(field);
    }
    va_end(args);
    probe_update_fields(probe);

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

double probe_get_queueing_time(const probe_t * probe) {
    return probe->queueing_time;
}

void probe_set_recv_time(probe_t * probe, double time) {
    probe->recv_time = time;
}

double probe_get_recv_time(const probe_t * probe) {
    return probe->recv_time;
}

#ifdef USE_SCHEDULING
bool probe_set_delay(probe_t * probe, field_t * delay)
{
    field_t * field;
    if (probe->delay) field_free(probe->delay);
    if (!(field = field_dup(delay))) goto ERR_DUP;
    probe->delay = field;
    return true;

ERR_DUP:
    return false;
}

double probe_get_delay(const probe_t * probe)
{
    const field_t * field_delay = probe->delay;
    double          delay;

    if (field_delay) {
        switch (field_delay->type) {
            case TYPE_DOUBLE :
                delay = field_delay->value.dbl;
                break;
            case TYPE_GENERATOR :
                delay = generator_get_value(field_delay->value.generator);
                break;
            default :
                delay = 0;
                fprintf(stderr, "Invalid 'delay' type field\n");
                break;
        }
    } else {
        delay = DELAY_BEST_EFFORT;
    }
    return delay;
}

double probe_next_delay(probe_t * probe)
{
    field_t * field_delay = probe->delay;
    double    delay       = DELAY_BEST_EFFORT;

    if (field_delay) {
        switch (field_delay->type) {
            case TYPE_DOUBLE :
                field_delay->value.dbl += field_delay->value.dbl;
                delay = field_delay->value.dbl;
                break;
            case TYPE_GENERATOR :
                delay = generator_next_value(field_delay->value.generator);
                break;
            default:
                fprintf(stderr, "Invalid 'delay' type field\n");
                break;
        }
    }
    return delay;
}

#endif

size_t probe_get_left_to_send(probe_t * probe) {
    return probe->left_to_send;
}

void probe_set_left_to_send(probe_t * probe, size_t num_left) {
    probe->left_to_send = num_left;
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

bool probe_extract_ext(const probe_t * probe, const char * name, size_t depth, void * value) {
    size_t                   i, num_layers = probe_get_num_layers(probe);
    const layer_t          * layer;
    const protocol_field_t * protocol_field;

    // We go through the layers until we get the required field
    for(i = depth; i < num_layers; i++) {
        layer = probe_get_layer(probe, i);
        if (!(protocol_field = layer_get_protocol_field(layer, name))) continue;

        // Hack to convert ipv*_t extracted into address_t value.
        switch (protocol_field->type) {
#ifdef USE_IPV4
            case TYPE_IPV4:
                memset(value, 0, sizeof(address_t));
                ((address_t *) value)->family = AF_INET;
                value = &((address_t *) value)->ip.ipv4;
                break;
#endif
#ifdef USE_IPV6
            case TYPE_IPV6:
                memset(value, 0, sizeof(address_t));
                ((address_t *) value)->family = AF_INET6;
                value = &((address_t *) value)->ip.ipv6;
                break;
#endif
            default: break;
        }

        if ((layer = probe_get_layer(probe, i))
        &&   layer_extract(layer, name, value)) {
            return true;
        }
    }

    return false;
}

bool probe_extract(const probe_t * probe, const char * name, void * dst) {
    field_t * flow_id_field;

    // TEMPORARY HACK TO MANAGE flow_id metafield
    if (!strcmp(name, "flow_id")) {
        if ((flow_id_field = probe_create_metafield(probe, "flow_id")) != NULL) {
            memcpy(dst, &flow_id_field->value.int16, sizeof(uint16_t));
            field_free(flow_id_field);
            return true;
        }
        return false;
    }

    return probe_extract_ext(probe, name, 0, dst);
}

packet_t * probe_create_packet(probe_t * probe) {
    // TODO
    // See packet.c: we store in packet.c the destination IP.
    // This will be removed once the bitfield will be supported in probe.c.

    // The destination IP is a mandatory field

    if (!(probe_extract(probe, "dst_ip", probe->packet->dst_ip))) {
        fprintf(stderr, "probe_create_packet: This probe does not carry 'dst_ip' field!\n");
        goto ERR_EXTRACT_DST_IP;
    }

    return probe->packet;

ERR_EXTRACT_DST_IP:
    return NULL;
}

//---------------------------------------------------------------------------
// probe_reply_t
//---------------------------------------------------------------------------

probe_reply_t * probe_reply_create() {
    return calloc(1, sizeof(probe_reply_t));
}

void probe_reply_free(probe_reply_t * probe_reply) {
    if (probe_reply) {
        free(probe_reply);
    }
}

void probe_reply_deep_free(probe_reply_t * probe_reply) {
    if (probe_reply) {
        if (probe_reply->probe) probe_free(probe_reply->probe);
        if (probe_reply->reply) probe_free(probe_reply->reply);
        probe_reply_free(probe_reply);
    }
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


