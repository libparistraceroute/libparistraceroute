#ifndef PT_LOOP_H
#define PT_LOOP_H

#include <sys/epoll.h>
#include <sys/eventfd.h>

#include "algorithm.h"
#include "network.h"
#include "event.h"

typedef struct pt_loop_s {
    // Network
    network_t            * network;

    // Algorithms
    void                 * algorithm_instances_root;
    unsigned int           next_algorithm_id;
    int                    eventfd_algorithm;

    // User
    int                    eventfd_user;            /**< User notification */
    dynarray_t           * events_user;             /**< User events queue */
    int (* handler_user) (struct pt_loop_s *loop);  /**< User handler */

    // Epoll data
    int                    efd;
    struct epoll_event   * epoll_events;
    algorithm_instance_t * cur_instance;
} pt_loop_t;

/**
 * \brief Create the paristraceroute loop
 * \param handler_user A pointer to a function declared in the user's program
 *   called whenever a event concerning the user arises. 
 *   - This handler receives a pointer to the libparistraceroute loop.
 *   - It must return a negative value if the libparistraceroute loop
 *     has to be stopped.
 * \return A pointer to a loop if successfull, NULL otherwise.
 */

pt_loop_t * pt_loop_create(int (*handler_user)(struct pt_loop_s * loop));

/**
 * \brief Close properly the paristraceroute loop
 * \param loop The libparistraceroute loop 
 */

void pt_loop_free(pt_loop_t * loop);

/**
 * \brief This function is called every 'timeout' seconds. It dispatches
 *    events that arised during this interval (network events, user events...),
 *    dispatchs them in the right queue and calls appropriate handler.
 *    It may be :
 *    - the user-defined handler (see pt_loop_create())
 *    - a handler implemented in an algorithm module (algorithm and network events)
 *
 *  Typical usage:
 *
 *   int status;
 *   pt_loop_t * loop = pt_loop_create(handler_user)
 *   if (!loop) {
 *      // error
 *      ...
 *   }
 *
 *   // Instanciate algorithm(s)
 *   ...
 *
 *   // The main loop
 *   do {
 *       status = pt_loop(loop, 0);
 *   } while(status > 0);
 *
 *   if (status == 0) {
 *       // success
 *       ...
 *   } else {
 *       // failure
 *       ...
 *   }
 *
 * \param loop The libparistraceroute loop 
 * \param timeout The interval of time during 
 * \return The loop status. This is the min value among the values returned
 *    by the handlers called during the interval.
 *
 *  - <0: there is failure, the user has to break the main loop,
 *  - =0: algorithm has successfully ended, the user has to break the main loop,
 *  - >0: the algorithm has not yet ended, the user has to continue the main loop.
 */

int pt_loop(pt_loop_t * loop, unsigned int timeout);

/**
 * \brief Retrieve the user events stored in the user queue.
 * \param loop The libparistraceroute loop.
 * \return An array made of pt_loop_get_num_user_events(loop) addresses
 *    pointing on the user events.
 */

event_t ** pt_loop_get_user_events(pt_loop_t * loop);

/**
 * \brief Retrieve the number of user events stored in the user queue.
 * \param loop The libparistraceroute loop.
 * \return The number of user events.
 */

unsigned  pt_loop_get_num_user_events(pt_loop_t * loop);

int pt_loop_set_algorithm_instance(pt_loop_t * loop, algorithm_instance_t * instance);
algorithm_instance_t * pt_loop_get_algorithm_instance(pt_loop_t * loop);

//void pt_probe_reply_callback(struct pt_loop_s *loop, probe_t *probe, probe_t *reply);
//void pt_probe_send(struct pt_loop_s *loop, probe_t *probe);

#endif
