#ifndef LIBPT_LAYER_H
#define LIBPT_LAYER_H

/**
 * \file layer.h
 * \brief Header for layers.
 *
 *   A packet is made of several layers.
 *   The last layer corresponds to the payload of the packet and
 *   is not related to a network protocol (layer->protocol == NULL).
 *   The other layers correspond to each protocol header involved
 *   in the packet.
 *
 *   For instance a IPv4/ICMP packet is made of 3 layers
 *   (namely an IPv4 layer, nesting an ICMP layer, nesting
 *   a "payload" layer).
 */

#include <stdbool.h>

#include "protocol.h"
#include "field.h"
#include "buffer.h"

/**
 * \struct layer_t
 * \brief Structure describing a layer
 * A layer points to a segment of bytes transported by this probe_t instance.
 * For a given probe_t instance, a layer can be either related
 *  - to a network protocol
 *    - in this case, the segment both covers the header and its data
 *  - to its payload.
 *    - in this case, the segment covers the whole payload
 */

typedef struct layer_s {
    const protocol_t * protocol;     /**< Points to the protocol implemented in this layer. Set to NULL if this layer is the payload */
    uint8_t          * segment;      /**< Points to the begining of the segment (header + data) of this layer in the packet */
    uint8_t          * mask;         /**< TODO (not yet implemented)
                                          Indicates which bits have been set.
                                          Should points to probe's bitfield */
    size_t             segment_size; /**< Size of segment (e.g. header) related to this layer */
} layer_t;

/**
 * \brief Create a new layer structure
 * \return Newly created layer
 */

layer_t * layer_create();

/**
 * \brief Duplicate a layer
 * \param layer The original layer
 * \return A pointer to the newly created layer, NULL in case of failure.
 */

//layer_t * layer_dup(const layer_t * layer);

/**
 * \brief Delete a layer structure.
 *   layer->protocol and layer->segment are not freed (a layer usually points
 *   to a bytes allocated by a probe, so this segment will be freed by the
 *   probe_free function.
 * \param layer Pointer to the layer structure to delete
 */

void layer_free(layer_t * layer);

// Accessors

/**
 * \brief Set the protocol for a layer
 * \param layer Pointer to the layer structure to change
 * \param name Name of the protocol to use
 */

void layer_set_protocol(layer_t * layer, const protocol_t * protocol);

/**
 * \brief Set the header fields for a layer
 * \param layer Pointer to the layer structure to change
 * \param arg1 Pointer to a field structure to use in the layer.
 *    Multiple additional parameters of this type may be specified
 *    to set multiple fields
 */

//int layer_set_fields(layer_t * layer, field_t * field1, ...);

/**
 * \brief Update the segment managed by layer according to a field
 *    passed as a parameter.
 * \param layer Pointer to the layer structure to update.
 * \param field Pointer to the field we assign in this layer.
 * \return true iif successfull
 */

bool layer_set_field(layer_t * layer, const field_t * field);

const protocol_field_t * layer_get_protocol_field(const layer_t * layer, const char * key);
uint8_t * layer_get_field_segment(const layer_t * layer, const char * key);
bool layer_write_field(layer_t * layer, const char * key, const void * bytes, size_t num_bytes);

/**
 * \brief Update bytes managed.
 * \param layer Pointer to a layer_t structure. This layer must
 *   have layer->protocol == NULL, otherwise this layer is related
 *   to a network protocol layer.
 * \param bytes Bytes copied in the payload.
 * \param num_bytes Number of bytes copied from bytes in the payload.
 * \return true iif successful.
 */

bool layer_write_payload(layer_t * layer, const void * bytes, size_t num_bytes);

/**
 * \brief Write the data stored in a buffer in the layer's payload.
 *   This function can only be used if no layer is nested in the
 *   layer we're altering, otherwise, nothing happens.
 * \param layer A pointer to the layer that we're filling.
 * \param bytes Bytes copied in the payload.
 * \param num_bytes Number of bytes copied from bytes in the payload.
 * \param offset The offset (starting from the beginning of the payload)
 *    in bytes added to the payload address to write the data.
 * \return true iif successfull.
 */

bool layer_write_payload_ext(layer_t * layer, const void * bytes, size_t num_bytes, size_t offset);

/**
 * \brief Retrieve the size of the buffer stored in the layer_t structure.
 * \param layer A pointer to a layer_t instance.
 */

size_t    layer_get_segment_size(const layer_t * layer);
void      layer_set_segment_size(layer_t * layer, size_t segment_size);
uint8_t * layer_get_segment(const layer_t * layer);
void      layer_set_segment(layer_t * layer, uint8_t * segment);
uint8_t * layer_get_mask(const layer_t * layer);
void      layer_set_mask(layer_t * layer, uint8_t * mask);

/**
 * \brief Allocate a field instance based on the content related
 *   to a given field belonging to a layer
 * \param layer The queried layer
 * \param name The name of the queried field
 * \return A pointer to the allocated field, NULL in case of failure
 */

field_t * layer_create_field(const layer_t * layer, const char * name);

/**
 * \brief Create a layer_t instance based on a segment of packet (a
 *   segment is a sequence of bytes stored in a packet related to a
 *   layer).
 * \param protocol The protocol_t instance related to this segment.
 *   See also protocol_search()
 * \param segment The sequence of bytes we're dissecting
 * \param segment_size The segment size
 * \return The corresponding layer_t instance, NULL in case of failure.
 */

layer_t * layer_create_from_segment(const protocol_t * protocol, uint8_t * segment, size_t segment_size);

/**
 * \brief Extract a value from a field involved in a layer.
 * \param layer The queried layer instance.
 * \param key The name of a field involved in this layer.
 * \param value A preallocated buffer which will contain the corresponding value.
 * \return true if successful, false otherwise.
 */

bool layer_extract(const layer_t * layer, const char * key, void * value);

/**
 * \brief Print the content of a layer.
 * \param layer A pointer to the layer instance to print.
 * \param indent The number of space characters to write.
 *    before each printed line.
 */

void layer_dump(const layer_t * layer, unsigned int indent);

/**
 * \brief Print side by side 2 layers (only the checksum, protocol, and length fields).
 * \param layer1 The first layer.
 * \param layer2 The second layer.
 */

void layer_debug(const layer_t * layer1, const layer_t * layer2, unsigned int indent);

#endif // LIBPT_LAYER_H
