#include "config.h"

#include <stdlib.h> // malloc

#include "event.h"

event_t * event_create(
    event_type_t type,
    void * data,
    struct algorithm_instance_s * issuer,
    void (*data_free) (void * data)
) {
    event_t * event;
    if ((event = malloc(sizeof(event_t)))) {
        event->type = type;
        event->data = data;
        event->issuer = issuer;
        event->data_free = data_free;
    }
    return event;
}

void event_free(event_t * event)
{
    if (event) {
        if (event->data && event->data_free) {
            event->data_free(event->data);
        }
        // TODO: trigger a double free. Ex: paris-traceroute -n 8.8.8.8
        // free(event);
    }
}
