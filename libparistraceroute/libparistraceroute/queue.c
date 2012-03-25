#include "queue.h"

queue_t * queue_create(void)
{
    queue_t *queue;
    queue = malloc(sizeof(queue_t));
    queue->eventfd = eventfd(0, 0);
    if (queue->eventfd == -1) {
        free(queue);
        return NULL;
    }
    queue->packets = list_create();
    if (!(queue->packets)) {
        close(queue->eventfd);
        free(queue);
        queue = NULL;
    }
    return queue;
}

void queue_free(queue_t *queue)
{
    list_free(queue->packets, (void(*)(void*))packet_free);
    close(queue->eventfd);
    free(queue);
    queue = NULL;
}

int queue_push_packet(queue_t *queue, packet_t *packet)
{

    list_push_element(queue->packets, packet);
    eventfd_write(queue->eventfd, 1);

    return 0;
}

packet_t *queue_pop_packet(queue_t *queue)
{
    return list_pop_element(queue->packets);
}

int queue_get_fd(queue_t *queue)
{
    return queue->eventfd;
}

