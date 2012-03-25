#ifndef LAYER_H
#define LAYER_H

#include "protocol.h"
#include "stackedlist.h"
#include "field.h"

typedef struct layer_s {
    protocol_t *protocol;
    struct layer_s *sublayer;
    stackedlist_t *fields;
} layer_t;

layer_t *layer_create(void);
void layer_free(layer_t *layer);

int layer_set_protocol(layer_t *layer, char *name);
int layer_set_sublayer(layer_t *layer, layer_t *sublayer);
int layer_set_fields(layer_t *layer, field_t *arg1, ...);

/* What about padding, payload */

#endif
