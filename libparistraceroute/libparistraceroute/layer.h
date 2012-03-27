#ifndef LAYER_H
#define LAYER_H

/**
 * \file layer.h
 * \brief Header for layers*/

#include "protocol.h"
#include "stackedlist.h"
#include "field.h"

/**
 * \struct layer_t
 * \brief Structure describing a layer
 */
typedef struct layer_s {
	/** Pointer to a structure describing the protocol */
    protocol_t *protocol;
	/** Pointer to a structure describing a sublayer */
    struct layer_s *sublayer;
	/** Pointer to a stacked list of header fields */
    stackedlist_t *fields;
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

/**
 * \brief Set the protocol for a layer
 * \param layer Pointer to the layer structure to change
 * \param name Name of the protocol to use
 * \return 
 * */
int layer_set_protocol(layer_t *layer, char *name);
/**
 * \brief Set the sublayer for a layer
 * \param layer Pointer to the layer structure to change
 * \param sublayer Pointer to the sublayer to be used
 * \return 
 * */
int layer_set_sublayer(layer_t *layer, layer_t *sublayer);
/**
 * \brief Set the header fields for a layer
 * \param layer Pointer to the layer structure to change
 * \param arg1 Pointer to a field structure to use in the layer. Multiple additional parameters of this type may be specified to set multiple fields
 * \return 
 * */
int layer_set_fields(layer_t *layer, field_t *arg1, ...);

/* What about padding, payload */

#endif
