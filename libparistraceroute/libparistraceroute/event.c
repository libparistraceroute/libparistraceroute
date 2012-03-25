#include <stdlib.h>
#include "event.h"

event_t *event_create(event_type_t type, void *params)
{
    event_t *event;

    event = (event_t*)malloc(sizeof(event_t));
    event->type = type;
    event->params = params;

    return event;
}

void event_free(event_t *event)
{
    free(event);
}
