#include "use.h"
#include "config.h"

#include <search.h>             // VISIT
#include <stdbool.h>            // bool
#include <stdio.h>              // perror
#include <stdlib.h>             // malloc, free
#include <string.h>             // memset
#include <errno.h>              // perror
#include <unistd.h>             // close
#include <signal.h>             // SIGINT, SIGQUIT
#include <time.h>               // time_t, time()

#include "os/sys/epoll.h"       // epoll_ctl
#include "os/sys/eventfd.h"     // eventfd
#include "os/sys/signalfd.h"    // signalfd
#include "os/netinet/in.h"      // IPPROTO_ICMP, IPPROTO_ICMPV6
#include "probe.h"              // probe_t
#include "pt_loop.h"            // pt_loop.h
#include "algorithm.h"

#define MAXEVENTS 100

static pt_loop_t * s_loop = NULL; // Needed while we use twalk.

//---------------------------------------------------------------------------
// pt_loop options
//---------------------------------------------------------------------------

//static int    timeout[4]     = {180,    0,   UINT16_MAX, 1};
static double timeout[3] = OPTIONS_PT_LOOP_TIMEOUT;

static option_t pt_loop_options[] = {
    // action              short long          metavar    help    variable
    {opt_store_double_lim, "t",  "--timeout",  "TIMEOUT", HELP_t, timeout},
    END_OPT_SPECS
};

const option_t * pt_loop_get_options() {
    return pt_loop_options;
}

double options_pt_loop_get_timeout() {
    return timeout[0];
}

void options_pt_loop_init(pt_loop_t * loop) {
    pt_loop_set_timeout(loop, options_pt_loop_get_timeout());
}

void pt_loop_set_timeout(pt_loop_t * loop, double new_timeout) {
    loop->timeout = new_timeout;
}

//----------------------------------------------------------------
// Static functions
//----------------------------------------------------------------

/**
 * \brief process algorithm events (internal usage, see visitor for twalk)
 * \param node Current instance
 * \param visit Unused
 * \param level Unused
 */

static void pt_process_instance(
    const void * node,
    VISIT        visit,
    int          level
);

/**
 * \brief Free algorithm instances (internal usage, see visitor for twalk)
 * \param node Current instance
 * \param visit Unused
 * \param level Unused
 */

static void pt_free_instance(
    const void * node,
    VISIT        visit,
    int          level
);

/**
 * \brief process algorithm events (internal usage, see visitor for twalk)
 * \param loop The libparistraceroute loop
 * \param action A pointer to a function that process the current instance, e.g.
 *    that dispatches the events related to this instance.
 */

static void pt_instance_iter(
    struct pt_loop_s * loop,
    void (*action) (const void *, VISIT, int)
);


/**
 * \brief (Internal usage) Release from the memory loop->user_events.
 * \param loop The main loop.
 */

static inline void pt_loop_clear_user_events(pt_loop_t * loop) {
    dynarray_clear(loop->events_user, NULL); //(ELEMENT_FREE) event_free); TODO this provoke a segfault in case of stars
}

/**
 * \brief Register a file descriptor in Paris Traceroute loop.
 * \param loop The main loop.
 * \param fd A file descriptor.
 * \return true iif successful.
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
 * \brief Prepare a EFD_SEMAPHORE event_fd.
 * \return The corresponding file descriptor, -1 in case of failure.
 */

static inline int make_event_fd() {
    int fd;

    if ((fd = eventfd(0, EFD_SEMAPHORE)) == -1) {
        perror("Error eventfd");
    }
    return fd;
}

/**
 * \brief Prepare a signal file descriptor used to handle SIGINT and
 *   SIGQUIT signals.
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
 * \brief Create an ALGORITHM_* event carrying a nested user event.
 * \param loop The main loop.
 * \param type Type of event.
 */

static bool pt_raise_impl(pt_loop_t * loop, event_type_t type, event_t * nested_event) {
    event_t * event;

    // Allocate the event
    if (!(event = event_create(
        type,
        nested_event,
        loop->cur_instance,
        nested_event ? (ELEMENT_FREE) event_free : NULL
    ))) {
        return false;
    }

    // Raise the event
    pt_throw(loop, loop->cur_instance->caller, event);
    return true;
}

/**
 * \brief Process every pending user events (e.g. pt_loop_get_num_user_events(loop) events).
 * \param loop The main loop.
 * \return true iif successful.
 */

static int pt_loop_process_user_events(pt_loop_t * loop) {
    event_t  ** events        = pt_loop_get_user_events(loop);
    size_t      i, num_events = pt_loop_get_num_user_events(loop);
    uint64_t    ret;

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

/**
 * \brief Called when pt_loop handles a SIGINT|SIGQUIT to notify algorithms
 *   they must terminate. Sends a ALGORITHM_TERM event to each running instance.
 * \param node The current algorithm_instance_t.
 * \param visit (unused).
 * \param level (unused).
 */

static void pt_process_algorithms_terminate(const void * node, VISIT visit, int level) {
    algorithm_instance_t * instance = *((algorithm_instance_t * const *) node);

    // The pt_loop_t must send a TERM event to the current instance
    pt_throw(NULL, instance, event_create(ALGORITHM_TERM, NULL, NULL, NULL));
}

//----------------------------------------------------------------
// Non static functions
//----------------------------------------------------------------

pt_loop_t * pt_loop_create(void (*handler_user)(pt_loop_t *, event_t *, void *), void * user_data)
{
    pt_loop_t * loop;

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
#ifdef USE_IPV4
    if (!register_efd(loop, network_get_icmpv4_sockfd(loop->network))) goto ERR_EVENTFD_SNIFFER_ICMPV4;
#endif
#ifdef USE_IPV6
    if (!register_efd(loop, network_get_icmpv6_sockfd(loop->network))) goto ERR_EVENTFD_SNIFFER_ICMPV6;
#endif
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
    loop->status = PT_LOOP_CONTINUE;
    loop->next_algorithm_id = 1; // 0 means unaffected ?
    loop->cur_instance = NULL;
    loop->algorithm_instances_root = NULL;

    return loop;

ERR_EVENTS_USER:
    free(loop->epoll_events);
ERR_EVENTS:
ERR_EVENTFD_GROUP:
ERR_EVENTFD_TIMEOUT:
#ifdef USE_IPV4
ERR_EVENTFD_SNIFFER_ICMPV4:
#endif
#ifdef USE_IPV6
ERR_EVENTFD_SNIFFER_ICMPV6:
#endif
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
        if (loop->events_user)  dynarray_free(loop->events_user, (ELEMENT_FREE) event_free);
        if (loop->epoll_events) free(loop->epoll_events);
        network_free(loop->network);
        close(loop->sfd);
        close(loop->eventfd_user);
        close(loop->eventfd_algorithm);
        close(loop->efd);

        // Events are cleared while destroying algorithm instances
        pt_instance_iter(loop, pt_free_instance);
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

inline void pt_instance_iter(
    pt_loop_t * loop,
    void     (* action) (const void *, VISIT, int))
{
    twalk(loop->algorithm_instances_root, action);
}

void pt_process_instance(const void * node, VISIT visit, int level)
{
    algorithm_instance_t * instance = *((algorithm_instance_t * const *) node);
    size_t                 i, num_events;
    uint64_t               ret;
    ssize_t                count;

    // Save temporarily this algorithm context.
    instance->loop->cur_instance = instance;

    // Execute algorithm handler for each events.
    num_events = dynarray_get_size(instance->events);
    for (i = 0; i < num_events; i++) {
        event_t * event;

        count = read(instance->loop->eventfd_algorithm, &ret, sizeof(ret));
        if (count == -1) return;

        event = dynarray_get_ith_element(instance->events, i);
        instance->algorithm->handler(
            instance->loop, event,
            &instance->data,
            instance->probe_skel,
            instance->options
        );

        // Next events for this instance are ignored.
        if (event->type == ALGORITHM_TERM) {
            break;
        }
    }

    // Restore the algorithm context
    instance->loop->cur_instance = NULL;

    // Flush events queue
    algorithm_instance_clear_events(instance);
}

// Notify the called algorithm that it can start

void pt_free_instance(
    const void * node,
    VISIT        visit,
    int          level
) {
    algorithm_instance_t * instance = *((algorithm_instance_t * const *) node);
    algorithm_instance_free(instance); // No notification
}

int pt_loop(pt_loop_t * loop) {
    int n, i, cur_fd;

    // TODO set a flag to avoid issues due to several threads
    // and put a critical section to manage this flag

    int network_sendq_fd      = network_get_sendq_fd(loop->network);
    int network_recvq_fd      = network_get_recvq_fd(loop->network);
#ifdef USE_IPV4
    int network_icmpv4_sockfd = network_get_icmpv4_sockfd(loop->network);
#endif
#ifdef USE_IPV6
    int network_icmpv6_sockfd = network_get_icmpv6_sockfd(loop->network);
#endif
    int network_timerfd       = network_get_timerfd(loop->network);
    int network_group_timerfd = network_get_group_timerfd(loop->network);
    ssize_t s;
    struct signalfd_siginfo fdsi;

    // This boolean is used to avoid to terminate twice when --timeout is used.
    bool max_time_has_expired = false;
    double max_time = loop->timeout;

    // Take the time for the timeout of the algorithm.
    time_t starting_time_algorithm = time(&starting_time_algorithm);

    do {
        // Case where the algorithm timeout, send a terminating event to the algorithm.
        double elapsed_time = difftime(time(NULL), starting_time_algorithm);
        if (max_time && elapsed_time > max_time && !max_time_has_expired) {
            pt_instance_iter(loop, pt_process_algorithms_terminate);
            fprintf(stdout, "Algorithm terminated because of a time expiry\n");
            max_time_has_expired = true;
        }

        // Wait for events.
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

            if (loop->status != PT_LOOP_INTERRUPTED && cur_fd == network_sendq_fd) {
                if (!network_process_sendq(loop->network)) {
                    if (loop->network->is_verbose) fprintf(stderr, "pt_loop: Can't send packet\n");
                }
            } else if (loop->status != PT_LOOP_INTERRUPTED && cur_fd == network_recvq_fd) {
                if (!network_process_recvq(loop->network)) {
                    if (loop->network->is_verbose) fprintf(stderr, "pt_loop: Cannot fetch packet\n");
                }
            } else if (loop->status != PT_LOOP_INTERRUPTED && cur_fd == network_group_timerfd) {
                 //printf("pt_loop processing scheduled probes\n");
                network_process_scheduled_probe(loop->network);
#ifdef USE_IPV4
            } else if (loop->status != PT_LOOP_INTERRUPTED && cur_fd == network_icmpv4_sockfd) {
                network_process_sniffer(loop->network, IPPROTO_ICMP);
#endif
#ifdef USE_IPV6
            } else if (loop->status != PT_LOOP_INTERRUPTED && cur_fd == network_icmpv6_sockfd) {
                network_process_sniffer(loop->network, IPPROTO_ICMPV6);
#endif
            } else if (cur_fd == loop->eventfd_algorithm) {

                // There is one common queue shared by every instancied algorithms.
                // We call pt_process_algorithms_iter() to find for which instance
                // the event has been raised. Then we process this event thanks
                // to pt_process_algorithms_instance() that calls the handler.

                // << This must be thread safe!!
                s_loop = loop;
                pt_instance_iter(loop, pt_process_instance);
                s_loop = NULL;
                // >> This must be thread safe!!

            } else if (cur_fd == loop->eventfd_user) {

                // Throw this event to the user-defined handler
                pt_loop_process_user_events(loop);

                // Flush the queue
                pt_loop_clear_user_events(loop);

            } else if (loop->status != PT_LOOP_INTERRUPTED && cur_fd == loop->sfd) {

                // Handling signals (ctrl-c, etc.)
                s = read(loop->sfd, &fdsi, sizeof(struct signalfd_siginfo));
                if (s != sizeof(struct signalfd_siginfo)) {
                    perror("read");
                    continue;
                }

                if (fdsi.ssi_signo == SIGINT || fdsi.ssi_signo == SIGQUIT) {
                    pt_instance_iter(loop, pt_process_algorithms_terminate);
                } else {
                    perror("Read unexpected signal\n");
                }
                loop->status = PT_LOOP_INTERRUPTED;
                break;

            } else if (loop->status != PT_LOOP_INTERRUPTED && cur_fd == network_timerfd) {

                // Timer managing timeout in network layer has expired
                // At least one probe has expired
                if (!network_drop_expired_flying_probe(loop->network)) {
                    fprintf(stderr, "Error while processing timeout\n");
                }
            }
        }
    } while (loop->status == PT_LOOP_CONTINUE || loop->status == PT_LOOP_INTERRUPTED);

    // Process internal events
    return loop->status == PT_LOOP_TERMINATE ? 0 : -1;
}

bool pt_send_probe(pt_loop_t * loop, probe_t * probe) {
    // Annotate which algorithm has generated this probe
    probe_set_caller(probe, loop->cur_instance);

    // Tagging is achieved by network layer
    return network_send_probe(loop->network, probe);
}

void pt_loop_terminate(pt_loop_t * loop) {
    loop->status = PT_LOOP_TERMINATE;
}

bool pt_raise_event(pt_loop_t * loop, event_t * event) {
    return pt_raise_impl(loop, ALGORITHM_EVENT, event);
}

bool pt_raise_error(pt_loop_t * loop) {
    return pt_raise_impl(loop, ALGORITHM_ERROR, NULL);
}

bool pt_raise_terminated(pt_loop_t * loop) {
    return pt_raise_impl(loop, ALGORITHM_HAS_TERMINATED, NULL);
}
