#ifndef LAYER_H
#define LAYER_H

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
 */

// TODO we could use buffer_t instead of uint8_t + size_t if we update probe.c
typedef struct {
    const protocol_t * protocol;    /**< Points to the protocol implemented in this layer. Set to NULL if this layer is the payload */
    uint8_t    * buffer;            /**< Points to the begining of the layer in the packet */
    uint8_t    * mask;              /**< Indicates which bits have been set. TODO: not yet implemented, use a bitfield_t. Maybe probe_t is sufficient? */
    size_t       header_size;       /**< Size of the header (0 if this layer is related to the payload */
    // TODO rename data_size
    size_t       buffer_size;       /**< Size of data carried by the layer (header + data) */
} layer_t;

/**
 * \brief Create a new layer structure
 * \return Newly created layer
 */

layer_t * layer_create(void);

/**
 * \brief Duplicate a layer
 * \param layer The original layer
 * \return A pointer to the newly created layer, NULL in case of failure.
 */

//layer_t * layer_dup(const layer_t * layer);

/**
 * \brief Delete a layer structure.
 *   layer->protocol and layer->buffer are not freed (a layer usually points
 *   to a bytes allocated by a probe, so this buffer will be freed by the
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
 * \brief Set the sublayer for a layer
 * \param layer Pointer to the layer structure to change
 * \param sublayer Pointer to the sublayer to be used
 * \return 
 */

// TODO should return bool
int layer_set_sublayer(layer_t * layer, layer_t * sublayer);

/**
 * \brief Set the header fields for a layer
 * \param layer Pointer to the layer structure to change
 * \param arg1 Pointer to a field structure to use in the layer.
 *    Multiple additional parameters of this type may be specified
 *    to set multiple fields
 */

//int layer_set_fields(layer_t * layer, field_t * field1, ...);

/**
 * \brief Update the buffer of a layer according to a field
 *    passed as a parameter. 
 * \param layer Pointer to the layer structure to update.
 * \param field Pointer to the field we assign in this layer.
 * \return true iif successfull 
 */

bool layer_set_field(layer_t * layer, field_t * field);

/**
 * \brief Update bytes managed  
 * \param layer Pointer to a layer_t structure. This layer must
 *   have layer->protocol == NULL, otherwise this layer is related
 *   to a network protocol layer.
 * \param payload The payload to write in the data, or NULL.
 * \return true iif successful
 */

// TODO rename layer_write_payload -> layer_write_payload_ext
// TODO rename layer_set_payload -> layer_write_payload
bool layer_set_payload(layer_t * layer, buffer_t * payload);

/**
 * \brief Write the data stored in a buffer in the layer's payload.
 *   This function can only be used if no layer is nested in the
 *   layer we're altering, otherwise, nothing happens.
 * \param layer A pointer to the layer that we're filling.
 * \param payload The data to duplicate into the layer's payload.
 * \param offset The offset (starting from the beginning of the payload)
 *    added to the payload address to write the data.
 * \return true iif successfull
 */

bool layer_write_payload(layer_t * layer, const buffer_t * payload, unsigned int offset);

/**
 * \brief Set the size of the buffer stored in the layer.
 *    It does not resize the buffer itself.
 * \sa buffer_resize(buffer_t * buffer, size_t size)
 * \param layer A pointer to a layer instance
 * \param buffer_size The new size
 */

void layer_set_buffer_size(layer_t * layer, size_t buffer_size);

/**
 * \brief Retrieve the size of the buffer stored in the layer_t structure.
 * \param layer A pointer to a layer instance.
 */ 

// TODO rename get_data_size
size_t layer_get_buffer_size(const layer_t * layer);
void layer_set_header_size(layer_t * layer, size_t header_size);

// TODO rename set_segment_address
void layer_set_buffer(layer_t * layer, uint8_t * buffer);

// TODO rename get_segment_address
uint8_t * layer_get_buffer(layer_t * layer);

//void layer_set_mask(layer_t * layer, uint8_t * mask);

/**
 * \brief Allocate a field instance based on the content related
 *   to a given field belonging to a layer
 * \param layer The queried layer
 * \param name The name of the queried field
 * \return A pointer to the allocated field, NULL in case of failure
 */

field_t * layer_create_field(const layer_t * layer, const char * name);

/**
 * \brief Print the content of a layer
 * \param layer A pointer to the layer instance to print
 * \param indent The number of space characters to write
 *    before each printed line.
 */

void layer_dump(layer_t * layer, unsigned int indent);

#endif
