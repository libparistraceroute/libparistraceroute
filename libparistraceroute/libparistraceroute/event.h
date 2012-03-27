#ifndef EVENT_H
#define EVENT_H

#include "probe.h"

typedef enum {
    ALGORITHM_INIT,        /**< We start the current algorithm */
    ALGORITHM_TERMINATED,  /**< An algorithm called by the current algorithm has finished */
    REPLY_RECEIVED         /**< The current algorithm has received a reply */
} event_type_t;

typedef struct reply_received_params_s {
    probe_t * probe;       /**< Probe related to this event */
 // reply_t * reply;       /**< The reply related to this probe */
} reply_received_params_t;

typedef struct {
    event_type_t type;     /**< Kind of event */
    void * params;         /**< REPLY_RECEIVED: reply_received_params_t */
} event_t;

event_t *event_create(event_type_t type, void *params);
void event_free(event_t *event);

#endif
