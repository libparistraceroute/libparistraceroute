#include <stdlib.h>
#include <unistd.h>
#include <stdio.h> // DEBUG
#include "pt_loop.h"

#define MAXEVENTS 100

// Internal usage
static void pt_loop_clear_user_events(pt_loop_t * loop);

pt_loop_t * pt_loop_create(int (*handler_user)(struct pt_loop_s *loop))
{
    int s, network_sendq_fd, network_recvq_fd, network_sniffer_fd;
    pt_loop_t * loop;
    struct epoll_event network_sendq_event;
    struct epoll_event network_recvq_event;
    struct epoll_event network_sniffer_event;
    struct epoll_event algorithm_event;
    struct epoll_event user_event;

    loop = malloc(sizeof(pt_loop_t));
    if(!loop) goto ERR_MALLOC;
    loop->handler_user = handler_user;

    /* epoll file descriptor */
    loop->efd = epoll_create1(0);
    if (loop->efd == -1)
        goto ERR_EPOLL;
    
    /* eventfd for algorithms */
    loop->eventfd_algorithm = eventfd(0, 0);
    if (loop->eventfd_algorithm == -1)
        goto ERR_EVENTFD_ALGORITHM;
    algorithm_event.data.fd = loop->eventfd_algorithm;
    algorithm_event.events = EPOLLIN | EPOLLET;
    s = epoll_ctl(loop->efd, EPOLL_CTL_ADD, loop->eventfd_algorithm, &algorithm_event);
    if (s == -1)
        goto ERR_EVENTFD_ADD_ALGORITHM;

    /* eventfd for user */
    loop->eventfd_user = eventfd(0, 0);
    if (loop->eventfd_user == -1)
        goto ERR_EVENTFD_USER;
    user_event.data.fd = loop->eventfd_user;
    user_event.events = EPOLLIN | EPOLLET;
    s = epoll_ctl (loop->efd, EPOLL_CTL_ADD, loop->eventfd_user, &user_event);
    if (s == -1)
        goto ERR_EVENTFD_ADD_USER;
   
    /* network */
    loop->network = network_create();
    if (!(loop->network))
        goto ERR_NETWORK;

    /* sending queue */
    network_sendq_fd = network_get_sendq_fd(loop->network);
    network_sendq_event.data.fd = network_sendq_fd;
    network_sendq_event.events = EPOLLIN | EPOLLET;
    s = epoll_ctl (loop->efd, EPOLL_CTL_ADD, network_sendq_fd, &network_sendq_event);
    if (s == -1)
        goto ERR_SENDQ_ADD;
    
    /* receiving queue */
    network_recvq_fd = network_get_recvq_fd(loop->network);
    network_recvq_event.data.fd = network_recvq_fd;
    network_recvq_event.events = EPOLLIN | EPOLLET;
    s = epoll_ctl (loop->efd, EPOLL_CTL_ADD, network_recvq_fd, &network_recvq_event);
    if (s == -1) 
        goto ERR_RECVQ_ADD;

    /* sniffer */
    network_sniffer_fd = network_get_sniffer_fd(loop->network);
    network_sniffer_event.data.fd = network_sniffer_fd;
    network_sniffer_event.events = EPOLLIN | EPOLLET;
    s = epoll_ctl (loop->efd, EPOLL_CTL_ADD, network_sniffer_fd, &network_sniffer_event);
    if (s == -1)
        goto ERR_SNIFFER_ADD;

    /* Buffer where events are returned */
    loop->epoll_events = calloc(MAXEVENTS, sizeof(struct epoll_event));
    if (!loop->epoll_events) goto ERR_EVENTS;

    loop->events_user = dynarray_create();
    if (!loop->events_user) goto ERR_EVENTS_USER;

    loop->next_algorithm_id = 1; // 0 means unaffected ?
    loop->cur_instance = NULL;

    return loop;

ERR_EVENTS_USER:
    free(loop->events_user);
ERR_EVENTS:
    free(loop->epoll_events);
ERR_SNIFFER_ADD:
ERR_RECVQ_ADD:
ERR_SENDQ_ADD:
    network_free(loop->network);
ERR_NETWORK:
ERR_EVENTFD_ADD_USER:
    close(loop->eventfd_user);
ERR_EVENTFD_USER:
ERR_EVENTFD_ADD_ALGORITHM:
    close(loop->eventfd_algorithm);
ERR_EVENTFD_ALGORITHM:
    close(loop->efd);
ERR_EPOLL:
    free(loop);
ERR_MALLOC:
    return NULL;
}

void pt_loop_free(pt_loop_t * loop)
{
    if (loop) {
        printf("pt_loop_free: calling network_free\n");
        network_free(loop->network);
        printf("pt_loop_free: network_free OK\n");
        close(loop->eventfd_algorithm);
        close(loop->efd);
        pt_loop_clear_user_events(loop);
        free(loop);
    }
}

// Accessors

inline int pt_loop_set_algorithm_instance(
    pt_loop_t            * loop,
    algorithm_instance_t * instance
) {
    loop->cur_instance = instance;
    return 0;
}

inline algorithm_instance_t * pt_loop_get_algorithm_instance(pt_loop_t *loop)
{
    return loop ? loop->cur_instance : NULL;
}

inline event_t ** pt_loop_get_user_events(pt_loop_t * loop)
{
    return loop ?
        (event_t**) dynarray_get_elements(loop->events_user) :
        NULL;
}

inline unsigned pt_loop_get_num_user_events(pt_loop_t * loop)
{
    return loop->events_user->size;
}

inline void pt_loop_clear_user_events(pt_loop_t * loop)
{
    if (loop) {
        dynarray_clear(loop->events_user, (void (*)(void *)) event_free);
    }
}

static inline int min(int x, int y) {
    return x < y ? x : y;
}

int pt_loop(pt_loop_t *loop, unsigned int timeout)
{
    int n, i, cur_fd, ret = 1;

    // TODO set a flag to avoid issues due to several threads
    // and put a critical section to manage this flag

    int network_sendq_fd   = network_get_sendq_fd(loop->network);
    int network_recvq_fd   = network_get_recvq_fd(loop->network);
    int network_sniffer_fd = network_get_sniffer_fd(loop->network);

    /* Wait for events */
    n = epoll_wait(loop->efd, loop->epoll_events, MAXEVENTS, -1);

    /* XXX What kind of events do we have
     * - sockets (packets received, timeouts, etc.)
     * - internal queues where probes and packets are stored
     * - ...
     */

    // Dispatch events
    for (i = 0; i < n; i++) {
        cur_fd = loop->epoll_events[i].data.fd;

        // Handle errors on fds
        if ((loop->epoll_events[i].events & EPOLLERR)
        ||  (loop->epoll_events[i].events & EPOLLHUP)
        || !(loop->epoll_events[i].events & EPOLLIN)
        ) {
            // An error has occured on this fd
            fprintf(stderr, "epoll error\n");
            close(cur_fd);
            continue;
        }
        
        if (cur_fd == network_sendq_fd) {
            network_process_sendq(loop->network);
        } else if (cur_fd == network_recvq_fd) {
            network_process_recvq(loop->network);
        } else if (cur_fd == network_sniffer_fd) {
            network_process_sniffer(loop->network);
        } else if (cur_fd == loop->eventfd_algorithm) {
            
            // There is one common queue shared by every instancied algorithms.
            // We call pt_process_algorithms_iter() to find for which instance
            // the event has been raised. Then we process this event thanks
            // to pt_process_algorithms_instance() that calls the handler.
            pt_algorithm_instance_iter(loop, pt_process_algorithms_instance);

        } else if (cur_fd == loop->eventfd_user) {

            // Throw this event to the user-defined handler
            ret = min(ret, loop->handler_user(loop));
            
            // Flush the queue 
            pt_loop_clear_user_events(loop);
        }
    }

    // Process internal events
    return ret;
}

