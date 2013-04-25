#ifndef QUEUE_H
#define QUEUE_H

#include <sys/eventfd.h>
#include <stdbool.h>

#include "list.h"

typedef struct {
    list_t * elements; /**< Elements stored in the queue */
    int      eventfd;  /**< File descriptor notifying an update in the queue */
} queue_t;

/**
 * \brief Create a new queue
 * \return A pointer to the newly created queue, NULL otherwise.
 */

queue_t * queue_create(void);

/**
 * \brief Delete a queue structure.
 * \param queue Points to the queue structure to delete.
 */

void queue_free(queue_t * queue, void (*element_free) (void * element));

/**
 * \brief Push an element in the queue
 * \param queue Points to the impacted queue instance
 * \param element Points to the pushed element
 * \return true iif successfull 
 */

bool queue_push_element(queue_t * queue, void * element);

/**
 * \brief Pop an element from the queue.
 * \param queue The queue from which we pop an element.
 * \param element_free Function called back to free the poped element.
 * \return The address of the poped element, NULL in case of failure.
 */

void * queue_pop_element(queue_t * queue, void (*element_free)(void * element));

/**
 * \brief Retrieve the file descriptor stored in a queue_t instance.
 * \param queue A pointer to a queue instance.
 * \return The corresponding file descriptor.
 */

int queue_get_fd(const queue_t * queue);

#endif

