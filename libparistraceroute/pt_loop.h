#ifndef LIBPT_PT_LOOP_H
#define LIBPT_PT_LOOP_H

/**
 * \file pt_loop.h
 * \brief Main loop managing events.
 *
 * pt_loop aims at processing events triggered:
 * - by the user:
 *      ctrl c...
 * - by libparistraceroute:
 *      reply handled,
 *      timeout...
 * -  by the algorithms built above libparistraceroute:
 *      see libparistraceroute/algorithms/
 */

#include <limits.h>      // INT_MAX

// Do not include "algorithm.h" to avoid mutual inclusion
#include "options.h"
#include "probe.h"
#include "network.h"
#include "event.h"

#ifdef __cplusplus
extern "C" {
#endif

//---------------------------------------------------------------------------
// pt_loop options
//---------------------------------------------------------------------------

// Maximum time spent inside the pt_loop
#define PT_LOOP_DEFAULT_TIMEOUT 180

#define OPTIONS_PT_LOOP_TIMEOUT {PT_LOOP_DEFAULT_TIMEOUT, 0, INT_MAX}
#define HELP_t "Set the timeout in seconds of the measurement (default is 180 seconds, pass 0 to set it to infinity)."

/**
 * \brief Retrieve the timeout defined for the pt_loop.
 * \return The value set in the network layer (in seconds)
 */

double options_pt_loop_get_timeout();

/**
 * \brief Get the command-line options related to the pt_loop.
 * \return A pointer to a structure containing the options.
 */

const option_t * pt_loop_get_options();

//---------------------------------------------------------------------------
// pt_loop
//---------------------------------------------------------------------------

typedef enum pt_loop_status_e {
    PT_LOOP_CONTINUE,    /**< Process and wait for next events */
    PT_LOOP_TERMINATE,   /**< Stop properly the pt_loop */
    PT_LOOP_INTERRUPTED  /**< Abrupt interruption (ctrl c): process last pending events, ignore new events. */
} pt_loop_status_t;

typedef struct pt_loop_s {
    // Network
    network_t                   * network;                  /**< The network layer */

    // Algorithms
    void                        * algorithm_instances_root;
    unsigned int                  next_algorithm_id;
    int                           eventfd_algorithm;

    // User
    int                           eventfd_user;             /**< User notification */
    int                           eventfd_terminate;        /**< This eventfd_terminate is set when the pt_loop_t must break */
    dynarray_t                  * events_user;              /**< User events queue (events raised from the library to a program */

    void (*handler_user)(
        struct pt_loop_s *,
        event_t          *,
        void             *
    );                                                      /**< Points to a user-defined handler called whenever the library
                                                                 raised an event for a program. */
    void                        * user_data;                /**< Data shared by the all algorithms running thanks to this pt_loop. */

    pt_loop_status_t              status;                   /**< State of the loop. See pt_loop_status_t for further details. */
    double                        timeout;                  /**< Lifetime of the pt-loop. 0 means infinite lifetime. */

    // Signal data
    int                           sfd;                      // signalfd

    // Epoll data
    int                           efd;
    struct epoll_event          * epoll_events;
    struct algorithm_instance_s * cur_instance;

} pt_loop_t;

/**
 * \brief Create the event loop. The loop is typically created in the main()
 *    of an application and stopped once libparistraceroute is not anymore
 *    needed. The library raise some event to the application via its user
 *    handler.
 * \param handler_user A pointer to a function declared in the user's program
 *   called whenever a event concerning the user arises. This handler
 *   - receives a pointer to the libparistraceroute loop,
 *   - must return a value
 *     < 0: if the libparistraceroute loop has to be stopped (failure)
 *     = 0: if the libparistraceroute loop has to be stopped (success)
 *     > 0: if the libparistraceroute loop has to continue   (pending)
 * \param user_data A pointer forwarded by the loop to user_handler.
 *    This pointer offers an opportunity to pass data from the main() to
 *    the handler_user function.
 * \return A pointer to a loop if successfull, NULL otherwise.
 */

pt_loop_t * pt_loop_create(void (*handler_user)(pt_loop_t *, event_t *, void *), void * user_data);

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
 * Example: See libparistraceroute/paris-traceroute/paris-traceroute.c.
 *
 * \param loop The libparistraceroute loop.
 * \return The loop status. This is the min value among the values returned
 *    by the handlers called during the interval.
 *
 *  - <0: there is failure, the user has to break the main loop,
 *  - =0: algorithm has successfully ended, the user has to break the main loop,
 *  - >0: the algorithm has not yet ended, the user has to continue the main loop.
 */

int pt_loop(pt_loop_t * loop);

/**
 * \brief Init the options related to pt_loop.
 * \param loop The libparistraceroute loop.
 */

void options_pt_loop_init(pt_loop_t * loop);

/**
 * \brief Set a new timeout for the libparistraceroute loop.
 * \param loop The libparistraceroute loop.
 * \param new_timeout The new timeout.
 */

void pt_loop_set_timeout(pt_loop_t * loop, double new_timeout);

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

size_t pt_loop_get_num_user_events(pt_loop_t * loop);

/**
 * \brief Send a probe packet across a network
 * \param network Pointer to the network to use
 * \param probe Pointer to the probe to use
 * \param callback Function pointer to a callback function
 *     (Does not appear to be used currently)
 * \return true iif successful
 */

bool pt_send_probe(pt_loop_t * loop, probe_t * probe);

/**
 * \brief Stop the main loop. It is usually used to break the pt_loop call in the main program.
 * \param loop The main loop
 */

void pt_loop_terminate(pt_loop_t * loop);

/**
 * \brief (Used by algorithm) Notify pt_loop that the algorithm has raised a algorithm specific event.
 * \param loop The main loop
 * \param event The event nested in the ALGORITHM_EVENT event we will raise
 * \return true iif successful
 */

bool pt_raise_event(pt_loop_t * loop, event_t * event);

/**
 * \brief (Used by algorithm) Notify pt_loop that the algorithm has an error has occured.
 * \param loop The main loop
 * \return true iif successful
 */

bool pt_raise_error(pt_loop_t * loop);

/**
 * \brief (Used by algorithm) Notify pt_loop that the algorithm has terminated.
 * \param loop The main loop
 * \return true iif successful
 */

bool pt_raise_terminated(pt_loop_t * loop);

#ifdef __cplusplus
}
#endif

#endif // LIBPT_PT_LOOP_H
