#include <stdlib.h>
#include "event.h"

// XXX We should allocate the params here
event_t *event_create(event_type_t type, void *params)
{
    event_t *event;

    event = malloc(sizeof(event_t));
    event->type = type;
    event->params = params;

    return event;
}

// XXX We should free the params here
void event_free(event_t *event)
{
    free(event);
}
