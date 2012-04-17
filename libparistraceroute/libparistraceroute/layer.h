#ifndef LAYER_H
#define LAYER_H

/**
 * \file layer.h
 * \brief Header for layers*/

#include "protocol.h"
#include "field.h"
#include "buffer.h"

/**
 * \struct layer_t
 * \brief Structure describing a layer
 */
typedef struct {
	/** Pointer to a structure describing the protocol */
    protocol_t *protocol;
    unsigned char *buffer;
    size_t header_size;
    size_t buffer_size;
} layer_t;

/**
 * \brief Create a new layer structure
 * \return Newly created layer
 */
layer_t *layer_create(void);
/**
 * \brief Delete a layer structure
 * \param layer Pointer to the layer structure to delete
 */
void layer_free(layer_t *layer);

// Accessors

/**
 * \brief Set the protocol for a layer
 * \param layer Pointer to the layer structure to change
 * \param name Name of the protocol to use
 * */
void layer_set_protocol(layer_t *layer, protocol_t *protocol);

/**
 * \brief Set the sublayer for a layer
 * \param layer Pointer to the layer structure to change
 * \param sublayer Pointer to the sublayer to be used
 * \return 
 */

int layer_set_sublayer(layer_t *layer, layer_t *sublayer);
/**
 * \brief Set the header fields for a layer
 * \param layer Pointer to the layer structure to change
 * \param arg1 Pointer to a field structure to use in the layer. Multiple additional parameters of this type may be specified to set multiple fields
 * \return 
 */
int layer_set_fields(layer_t *layer, field_t *field1, ...);

/**
 * \brief Set the header fields for a layer
 * \param layer Pointer to the layer structure to change
 * \param arg1 Pointer to a field structure to use in the layer. Multiple additional parameters of this type may be specified to set multiple fields
 * \return 
 */
int layer_set_field(layer_t *layer, field_t *field);

/**
 * \brief Sets the specified layer as payload
 * \param layer Pointer to a layer_t structure
 * \param payload The payload to affect to the probe, or NULL.
 * \return 0 if successful
 */
int layer_set_payload(layer_t *layer, buffer_t *payload);

void layer_set_buffer_size(layer_t *layer, size_t buffer_size);
void layer_set_header_size(layer_t *layer, size_t header_size);
void layer_set_buffer(layer_t *layer, unsigned char *buffer);

field_t * layer_get_field(layer_t *layer, const char * name);

// Dump

void layer_dump(layer_t *layer, unsigned int indent);

#endif
