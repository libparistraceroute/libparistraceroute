#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include "probe.h"

#include "pt_loop.h"

probe_t * probe_create(void)
{
    probe_t *probe;

    probe = malloc(sizeof(probe_t));

    probe->fields = stackedlist_create();
    return probe;
}

void probe_free(probe_t *probe)
{
    free(probe);
    probe = NULL;
}

void probe_add_field(probe_t *probe, field_t *field)
{
    stackedlist_add_element(probe->fields, (void*)field);
}

void probe_set_fields(probe_t *probe, field_t *arg1, ...) {
    va_list ap;
    field_t *i;

    va_start(ap, arg1);
    for (i = arg1; i; i = va_arg(ap, field_t*)) {
        /* What about existing fields in the same level ? */
        probe_add_field(probe, i);
    }
    va_end(ap);
}

/* Iterator */

typedef struct {
    void *data;
    void (*callback)(field_t *field, void *data);
} iter_fields_data_t;

void probe_iter_fields_callback(void *element, void *data) {
    iter_fields_data_t *d = (iter_fields_data_t*)data;
    d->callback((field_t*)element, d->data);
}

void probe_iter_fields(probe_t *probe, void *data, void (*callback)(field_t *field, void *data))
{
    iter_fields_data_t tmp = {
        .data = data,
        .callback = callback
    };
    stackedlist_iter_elements(probe->fields, &tmp, probe_iter_fields_callback);
}

unsigned int probe_get_num_proto(probe_t *probe)
{
    return 0; // TODO
}

field_t ** probe_get_fields(probe_t *probe)
{
    return NULL; // TODO
}

unsigned char *probe_get_payload(probe_t *probe)
{
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
 * pt_loop_t
 ******************************************************************************/

void pt_probe_reply_callback(pt_loop_t *loop, probe_t *probe, probe_t *reply)
{
    // Search for probe and find the caller
    //algorithm_instance_add_event(instance->caller->caller_algorithm,
    //        event_create(PROBE_REPLY_RECEIVED, NULL));
    // Delete the probe, what about multiple answers ?
    return;
}

void pt_probe_send(pt_loop_t *loop, probe_t *probe)
{
    /* We need to remember who has been sending the probe */
    /* + Reply callback */
    network_send_probe(loop->network, probe, pt_probe_reply_callback);
}
