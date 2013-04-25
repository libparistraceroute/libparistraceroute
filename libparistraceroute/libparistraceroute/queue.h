#ifndef QUEUE_H
#define QUEUE_H

#include <sys/eventfd.h>

#include "list.h"
#include "probe.h"

typedef struct {
    list_t *elements;
    int eventfd;
} queue_t;

queue_t * queue_create(void);
void queue_free(queue_t *queue, void (*element_free) (void *element));

int queue_push_element(queue_t *queue, void *element);
void *queue_pop_element(queue_t *queue);
int queue_get_fd(queue_t *queue);

#endif

