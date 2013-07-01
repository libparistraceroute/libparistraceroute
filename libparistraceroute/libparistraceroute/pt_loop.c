#include "pt_loop.h"

#include <stdbool.h>       // bool
#include <stdio.h>         // perror
#include <stdlib.h>        // malloc, free
#include <string.h>        // memset
#include <errno.h>         // perror
#include <unistd.h>        // close
#include <sys/epoll.h>     // epoll_ctl
#include <sys/signalfd.h>  // eventfd
#include <signal.h>        // SIGINT, SIGQUIT
#include <netinet/in.h>    // IPPROTO_ICMP, IPPROTO_ICMPV6

#include "algorithm.h"

#define MAXEVENTS 100

/**
 * \brief (Internal usage) Release from the memory loop->user_events;
 * \param loop The main loop
 */

static inline void pt_loop_clear_user_events(pt_loop_t * loop) {
    dynarray_clear(loop->events_user, NULL); //(ELEMENT_FREE) event_free); TODO this provoke a segfault in case of stars
}

/**
 * \brief Register a file descriptor in Paris Traceroute loop
 * \param loop The main loop
 * \param fd A file descriptor
 * \return true iif successfull
 */

static bool register_efd(pt_loop_t * loop, int fd) {
    struct epoll_event event;

    // Check whether the fd is fine or not
    if (fd == -1) goto ERR_FD;

    // Prepare epoll event structure
    memset(&event, 0, sizeof(struct epoll_event));
    event.data.fd = fd;
    event.events = EPOLLIN; // | EPOLLET;

    // Register fd in pt_loop
    if (epoll_ctl(loop->efd, EPOLL_CTL_ADD, fd, &event) == -1) {
        perror("Error epoll_ctl");
        goto ERR_EPOLL_CTL;
    }
    return true;

ERR_EPOLL_CTL:
ERR_FD:
    return false;
}

/**
 * \brief Prepare a EFD_SEMAPHORE event_fd
 * \return The correspnding file descriptor, -1 in case of failure
 */

static inline int make_event_fd() {
    int fd;

    if ((fd = eventfd(0, EFD_SEMAPHORE)) == -1) {
        perror("Error eventfd");
    }
    return fd;
}

/**
 * \brief Prepare a signal file descriptor used to handle SIGINT and SIGQUIT signals
 * \return The correspnding file descriptor, -1 in case of failure
 */

static int make_signal_fd() {
    int       sfd;
    sigset_t  mask;

    // Signal processing
    // Block signals so that they are not managed by their handlers anymore
    sigemptyset(&mask);
    sigaddset(&mask, SIGINT);
    sigaddset(&mask, SIGQUIT);

    if (sigprocmask(SIG_BLOCK, &mask, NULL) == -1) {
        goto ERR_SIGPROCMASK;
    }

    if ((sfd = signalfd(-1, &mask, 0)) == -1) {
        perror("Error signalfd");
        goto ERR_SIGNALFD;
    }

    return sfd;

ERR_SIGPROCMASK:
ERR_SIGNALFD:
    return -1;
}

/**
 * \brief Create an ALGORITHM* event carrying a nested user event.
 * \param loop The main loop
 * \param type Type of event
 */

static bool pt_raise_impl(pt_loop_t * loop, event_type_t type, event_t * nested_event) {
    // Allocate the event
    event_t * event = event_create(
        type,
        nested_event,
        loop->cur_instance,
        nested_event ? (ELEMENT_FREE) event_free : NULL
    );
    if (!event) return false;

    // Raise the event
    pt_algorithm_throw(loop, loop->cur_instance->caller, event);
    return true;
}

pt_loop_t * pt_loop_create(void (*handler_user)(pt_loop_t *, event_t *, void *), void * user_data)
{
    pt_loop_t          * loop;

    if (!(loop = malloc(sizeof(pt_loop_t)))) goto ERR_MALLOC;
    loop->handler_user = handler_user;

    // Prepare epoll file descriptor
    if ((loop->efd = epoll_create1(0)) == -1) {
        perror("Error epoll_create1");
        goto ERR_EPOLL;
    }

    // Prepare algorithm events fd and register it in loop->efd
    if ((loop->eventfd_algorithm = make_event_fd()) == -1) goto ERR_MAKE_EVENTFD_ALGORITHM;
    if (!register_efd(loop, loop->eventfd_algorithm))      goto ERR_EVENTFD_ALGORITHM;

    // Prepare user events fd and register it in loop->efd
    if ((loop->eventfd_user = make_event_fd()) == -1)      goto ERR_MAKE_EVENTFD_USER;
    if (!register_efd(loop, loop->eventfd_user))           goto ERR_EVENTFD_USER;

    // Signal processing
    if ((loop->sfd = make_signal_fd()) == -1)              goto ERR_MAKE_SIGNALFD;
    if (!register_efd(loop, loop->sfd))                    goto ERR_SIGNALFD;

    // Prepare network layer and register it in pt_loop
    if (!(loop->network = network_create()))                           goto ERR_NETWORK_CREATE;
    if (!register_efd(loop, network_get_sendq_fd(loop->network)))      goto ERR_EVENTFD_SENDQ;
    if (!register_efd(loop, network_get_recvq_fd(loop->network)))      goto ERR_EVENTFD_RECVQ;
    if (!register_efd(loop, network_get_icmpv4_sockfd(loop->network))) goto ERR_EVENTFD_SNIFFER_ICMPV4;
    if (!register_efd(loop, network_get_icmpv6_sockfd(loop->network))) goto ERR_EVENTFD_SNIFFER_ICMPV6;
    if (!register_efd(loop, network_get_timerfd(loop->network)))       goto ERR_EVENTFD_TIMEOUT;
    if (!register_efd(loop, network_get_group_timerfd(loop->network))) goto ERR_EVENTFD_GROUP;

    // Buffer where pending events are stored
    if (!(loop->epoll_events = calloc(MAXEVENTS, sizeof(struct epoll_event)))) {
        goto ERR_EVENTS;
    }

    if (!(loop->events_user = dynarray_create())) {
        goto ERR_EVENTS_USER;
    }

    loop->user_data = user_data;
    loop->stop = PT_LOOP_CONTINUE;
    loop->next_algorithm_id = 1; // 0 means unaffected ?
    loop->cur_instance = NULL;
    loop->algorithm_instances_root = NULL;

    return loop;

ERR_EVENTS_USER:
    free(loop->epoll_events);
ERR_EVENTS:
ERR_EVENTFD_GROUP:
ERR_EVENTFD_TIMEOUT:
ERR_EVENTFD_SNIFFER_ICMPV6:
ERR_EVENTFD_SNIFFER_ICMPV4:
ERR_EVENTFD_RECVQ:
ERR_EVENTFD_SENDQ:
    network_free(loop->network);
ERR_NETWORK_CREATE:
ERR_SIGNALFD:
    close(loop->sfd);
ERR_MAKE_SIGNALFD:
    close(loop->eventfd_user);
ERR_EVENTFD_USER:
    close(loop->eventfd_algorithm);
ERR_MAKE_EVENTFD_USER:
ERR_EVENTFD_ALGORITHM:
    close(loop->efd);
ERR_MAKE_EVENTFD_ALGORITHM:
ERR_EPOLL:
    free(loop);
ERR_MALLOC:
    return NULL;
}

void pt_loop_free(pt_loop_t * loop)
{
    if (loop) {
        if (loop->events_user)  free(loop->events_user);
        if (loop->epoll_events) free(loop->epoll_events);
        network_free(loop->network);
        close(loop->sfd);
        close(loop->eventfd_user);
        close(loop->eventfd_algorithm);
        close(loop->efd);

        // Events are cleared while destroying algorithm instances
        pt_algorithm_instance_iter(loop, pt_free_algorithms_instance);
        free(loop);
    }
}

// Accessors

inline event_t ** pt_loop_get_user_events(pt_loop_t * loop) {
    return loop ?
        (event_t **) dynarray_get_elements(loop->events_user) :
        NULL;
}

inline size_t pt_loop_get_num_user_events(pt_loop_t * loop) {
    return loop->events_user->size;
}

/**
 * \brief Process every pending user events (e.g. pt_loop_get_num_user_events(loop) events)
 * \param loop The main loop
 * \return true iif successfull
 */

static int pt_loop_process_user_events(pt_loop_t * loop) {
    event_t      ** events        = pt_loop_get_user_events(loop);
    size_t          i, num_events = pt_loop_get_num_user_events(loop);
    uint64_t        ret;

    for (i = 0; i < num_events; i++) {
        if (read(loop->eventfd_user, &ret, sizeof(ret)) == -1) {
            return -1;
        }
        // TODO decrement the associated eventfd counter

        // Call user-defined handler and pass the current user event
        loop->handler_user(loop, events[i], loop->user_data);
    }
    return 1;
}


int pt_loop(pt_loop_t *loop, unsigned int timeout)
{
    int n, i, cur_fd;

    // TODO set a flag to avoid issues due to several threads
    // and put a critical section to manage this flag

    int network_sendq_fd      = network_get_sendq_fd(loop->network);
    int network_recvq_fd      = network_get_recvq_fd(loop->network);
    int network_icmpv4_sockfd = network_get_icmpv4_sockfd(loop->network);
    int network_icmpv6_sockfd = network_get_icmpv6_sockfd(loop->network);
    int network_timerfd       = network_get_timerfd(loop->network);
    int network_group_timerfd = network_get_group_timerfd(loop->network);
    ssize_t s;
    struct signalfd_siginfo fdsi;

    do {
        /* Wait for events */
        n = epoll_wait(loop->efd, loop->epoll_events, MAXEVENTS, -1);

        /* XXX What kind of events do we have
         * - sockets (packets received, timeouts, etc.)
         * - internal queues where probes and packets are stored
         * - ...
         */
        // Dispatch events
        for (i = 0; i < n; i++) {
            // cur_fd has been activated, so we have to manage
            // the corresponding event.
            cur_fd = loop->epoll_events[i].data.fd;

            // Handle errors on fds
            if ((loop->epoll_events[i].events & EPOLLERR)
            ||  (loop->epoll_events[i].events & EPOLLHUP)
            || !(loop->epoll_events[i].events & EPOLLIN)
            ) {
                // An error has occured on this fd
                perror("epoll error");
                close(cur_fd);
                continue;
            }

            if (cur_fd == network_sendq_fd) {
                if (!network_process_sendq(loop->network)) {
                    fprintf(stderr, "pt_loop: Can't send packet\n");
                }
            } else if (cur_fd == network_recvq_fd) {
                if (!network_process_recvq(loop->network)) {
                    fprintf(stderr, "pt_loop: Cannot fetch packet\n");
                }
            } else if (cur_fd == network_group_timerfd) {
               // printf("pt_loop processing scheduled probes\n");
                network_process_scheduled_probe(loop->network);
            } else if (cur_fd == network_icmpv4_sockfd) {
                network_process_sniffer(loop->network, IPPROTO_ICMP);
            } else if (cur_fd == network_icmpv6_sockfd) {
                network_process_sniffer(loop->network, IPPROTO_ICMPV6);
            } else if (cur_fd == loop->eventfd_algorithm) {

                // There is one common queue shared by every instancied algorithms.
                // We call pt_process_algorithms_iter() to find for which instance
                // the event has been raised. Then we process this event thanks
                // to pt_process_algorithms_instance() that calls the handler.
                pt_algorithm_instance_iter(loop, pt_process_algorithms_instance);

            } else if (cur_fd == loop->eventfd_user) {

                // Throw this event to the user-defined handler
                pt_loop_process_user_events(loop);

                // Flush the queue
                pt_loop_clear_user_events(loop);
            } else if (cur_fd == loop->sfd) {
                // Handling signals
                //
                s = read(loop->sfd, &fdsi, sizeof(struct signalfd_siginfo));
                if (s != sizeof(struct signalfd_siginfo)) {
                    perror("read");
                    continue;
                }

                if (fdsi.ssi_signo == SIGINT) {
                    loop->stop = PT_LOOP_TERMINATE;
                } else if (fdsi.ssi_signo == SIGQUIT) {
                    exit(EXIT_SUCCESS);
                } else {
                    perror("Read unexpected signal\n");
                }
            } else if (cur_fd == network_timerfd) {

                // Timer managing timeout in network layer has expired
                // At least one probe has expired
                if (!network_drop_expired_flying_probe(loop->network)) {
                    perror("Error while processing timeout");
                }
            }
        }
    } while (loop->stop == PT_LOOP_CONTINUE);

    // Process internal events
    return loop->stop == PT_LOOP_TERMINATE ? 0 : -1;
}

bool pt_send_probe(pt_loop_t * loop, probe_t * probe)
{
    // Annotate which algorithm has generated this probe
    probe_set_caller(probe, loop->cur_instance);
    probe_set_queueing_time(probe, get_timestamp());

    // Tagging is achieved by network layer
    return network_send_probe(loop->network, probe);
}

void pt_loop_terminate(pt_loop_t * loop) {
    loop->stop = PT_LOOP_TERMINATE;
}

bool pt_raise_event(pt_loop_t * loop, event_t * event) {
    return pt_raise_impl(loop, ALGORITHM_EVENT, event);
}

bool pt_raise_error(pt_loop_t * loop) {
    return pt_raise_impl(loop, ALGORITHM_ERROR, NULL);
}

bool pt_raise_terminated(pt_loop_t * loop) {
    return pt_raise_impl(loop, ALGORITHM_TERMINATED, NULL);
}
