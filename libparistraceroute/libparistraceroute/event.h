#ifndef EVENT_H
#define EVENT_H

typedef enum {
    ALGORITHM_INIT,
    ALGORITHM_TERMINATED,
    REPLY_RECEIVED
} event_type_t;

typedef struct {
    event_type_t type;
    void *params;
} event_t;

event_t *event_create(event_type_t type, void *params);
void event_free(event_t *event);

#endif
