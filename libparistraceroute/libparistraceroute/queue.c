#include <unistd.h>
#include "queue.h"

queue_t * queue_create(void)
{
    queue_t *queue;

    queue = malloc(sizeof(queue_t));
    if (!queue)
        goto err_queue;

    /* Create an eventfd */
    queue->eventfd = eventfd(0, 0);
    if (queue->eventfd == -1)
        goto err_eventfd;

    /* Create the list that will contain the elements */
    queue->elements = list_create();
    if (!queue->elements)
        goto err_elements;

    return queue;

err_elements:
    close(queue->eventfd);
err_eventfd:
    free(queue);
    queue = NULL;
err_queue:
    return NULL;
}

void queue_free(queue_t *queue, void (*element_free) (void *element))
{
    list_free(queue->elements, element_free);
    close(queue->eventfd);
    free(queue);
    queue = NULL;
}

int queue_push_element(queue_t *queue, void *element)
{

    list_push_element(queue->elements, element);
    eventfd_write(queue->eventfd, 1);

    return 0;
}

void * queue_pop_element(queue_t *queue)
{
    return list_pop_element(queue->elements);
}

int queue_get_fd(queue_t *queue)
{
    return queue->eventfd;
}

