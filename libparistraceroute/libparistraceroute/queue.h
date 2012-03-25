#ifndef QUEUE_H
#define QUEUE_H

#include <sys/eventfd.h>

#include "list.h"
#include "packet.h"

typedef struct {
    list_t *packets;
    int eventfd;
} queue_t;

queue_t * queue_create(void);
void queue_free(queue_t *queue);

int queue_push_packet(queue_t *queue, packet_t *packet);
packet_t *queue_pop_packet(queue_t *queue);
int queue_get_fd(queue_t *queue);

#endif

