#include <errno.h>
#include <stdlib.h>
#include <unistd.h>

#include "queue.h"

queue_t * queue_create(void)
{
    queue_t * queue;

    // Alloc queue
    if (!(queue = malloc(sizeof(queue_t)))) {
        errno = ENOMEM;
        goto ERR_QUEUE;
    }

    // Create an eventfd
    if ((queue->eventfd = eventfd(0, EFD_SEMAPHORE)) == -1) {
        goto ERR_EVENTFD;
    }

    // Create the list that will contain the elements
    if (!(queue->elements = list_create())) {
        goto ERR_ELEMENTS;
    }
    return queue;

ERR_ELEMENTS:
    close(queue->eventfd);
ERR_EVENTFD:
    free(queue);
ERR_QUEUE:
    return NULL;
}

void queue_free(queue_t * queue, void (*element_free) (void * element))
{
    if (queue) {
        if (queue->elements) list_free(queue->elements, element_free);
        close(queue->eventfd);
        free(queue);
    }
}

inline bool queue_push_element(queue_t *queue, void * element)
{
    // Push an element in the queue
    // If successfull, write 1 in the file descriptor.
    return list_push_element(queue->elements, element)
        && (eventfd_write(queue->eventfd, 1) != -1);
}

void * queue_pop_element(queue_t *queue, void (*element_free)(void * element))
{
    eventfd_t value;
    return (read(queue->eventfd, &value, sizeof(value)) != -1) ?
        list_pop_element(queue->elements, element_free) :
        NULL;
}

inline int queue_get_fd(const queue_t * queue)
{
    return queue->eventfd;
}

