#include <stdlib.h>
#include <unistd.h>
#include <stdio.h> // DEBUG
#include <sys/signalfd.h>
#include <signal.h>
#include "pt_loop.h"
#include "algorithm.h"

#define MAXEVENTS 100

// Internal usage
static void pt_loop_clear_user_events(pt_loop_t * loop);

pt_loop_t * pt_loop_create(void (*handler_user)(pt_loop_t *, event_t *, void *))
{
    int s, network_sendq_fd, network_recvq_fd, network_sniffer_fd, network_timerfd;
    pt_loop_t * loop;
    struct epoll_event network_sendq_event;
    struct epoll_event network_recvq_event;
    struct epoll_event network_sniffer_event;
    struct epoll_event algorithm_event;
    struct epoll_event user_event;
    struct epoll_event network_timerfd_event;
    /* Signal management */
    sigset_t mask;
    struct epoll_event signal_event;

    loop = malloc(sizeof(pt_loop_t));
    if(!loop) goto ERR_MALLOC;
    loop->handler_user = handler_user;

    /* epoll file descriptor */
    loop->efd = epoll_create1(0);
    if (loop->efd == -1)
        goto ERR_EPOLL;
    
    /* eventfd for algorithms */
    loop->eventfd_algorithm = eventfd(0, EFD_SEMAPHORE);
    if (loop->eventfd_algorithm == -1)
        goto ERR_EVENTFD_ALGORITHM;
    algorithm_event.data.fd = loop->eventfd_algorithm;
    algorithm_event.events = EPOLLIN; // | EPOLLET;
    s = epoll_ctl(loop->efd, EPOLL_CTL_ADD, loop->eventfd_algorithm, &algorithm_event);
    if (s == -1)
        goto ERR_EVENTFD_ADD_ALGORITHM;

    /* eventfd for user */
    loop->eventfd_user = eventfd(0, EFD_SEMAPHORE);
    if (loop->eventfd_user == -1)
        goto ERR_EVENTFD_USER;
    user_event.data.fd = loop->eventfd_user;
    user_event.events = EPOLLIN; // | EPOLLET;
    s = epoll_ctl (loop->efd, EPOLL_CTL_ADD, loop->eventfd_user, &user_event);
    if (s == -1)
        goto ERR_EVENTFD_ADD_USER;
  
    /* Signal processing */

    sigemptyset(&mask);
    sigaddset(&mask, SIGINT);
    sigaddset(&mask, SIGQUIT);

    /* Block signals so that they are not managed by their handlers anymore */
    if (sigprocmask(SIG_BLOCK, &mask, NULL) == -1)
        goto ERR_SIGPROCMASK;

    loop->sfd = signalfd(-1, &mask, 0);
    if (loop->sfd == -1)
        goto ERR_SIGNALFD;

    signal_event.data.fd = loop->sfd;
    signal_event.events = EPOLLIN; // | EPOLLET;
    s = epoll_ctl (loop->efd, EPOLL_CTL_ADD, loop->sfd, &signal_event);
    if (s == -1) 
        goto ERR_SIGNALFD_ADD;
   
    /* network */
    loop->network = network_create();
    if (!(loop->network))
        goto ERR_NETWORK;

    /* sending queue */
    network_sendq_fd = network_get_sendq_fd(loop->network);
    network_sendq_event.data.fd = network_sendq_fd;
    network_sendq_event.events = EPOLLIN; // | EPOLLET;
    s = epoll_ctl (loop->efd, EPOLL_CTL_ADD, network_sendq_fd, &network_sendq_event);
    if (s == -1)
        goto ERR_SENDQ_ADD;
    
    /* receiving queue */
    network_recvq_fd = network_get_recvq_fd(loop->network);
    network_recvq_event.data.fd = network_recvq_fd;
    network_recvq_event.events = EPOLLIN; // | EPOLLET;
    s = epoll_ctl (loop->efd, EPOLL_CTL_ADD, network_recvq_fd, &network_recvq_event);
    if (s == -1) 
        goto ERR_RECVQ_ADD;

    /* sniffer */
    network_sniffer_fd = network_get_sniffer_fd(loop->network);
    network_sniffer_event.data.fd = network_sniffer_fd;
    network_sniffer_event.events = EPOLLIN; // | EPOLLET;
    s = epoll_ctl (loop->efd, EPOLL_CTL_ADD, network_sniffer_fd, &network_sniffer_event);
    if (s == -1)
        goto ERR_SNIFFER_ADD;

    /* Timerfd */
    network_timerfd = network_get_timerfd(loop->network);
    network_timerfd_event.data.fd = network_timerfd;
    network_timerfd_event.events = EPOLLIN; // | EPOLLET;
    s = epoll_ctl (loop->efd, EPOLL_CTL_ADD, network_timerfd, &network_timerfd_event);
    if (s == -1)
        goto ERR_TIMERFD_ADD;

    /* Buffer where events are returned */
    loop->epoll_events = calloc(MAXEVENTS, sizeof(struct epoll_event));
    if (!loop->epoll_events) goto ERR_EVENTS;

    loop->events_user = dynarray_create();
    if (!loop->events_user) goto ERR_EVENTS_USER;

    loop->user_data = NULL;
    loop->stop = PT_LOOP_CONTINUE;

    loop->next_algorithm_id = 1; // 0 means unaffected ?
    loop->cur_instance = NULL;

    loop->algorithm_instances_root = NULL;

    return loop;

//free(loop->events_user);
ERR_EVENTS_USER:
    free(loop->epoll_events);
ERR_EVENTS:
ERR_TIMERFD_ADD:
ERR_SNIFFER_ADD:
ERR_RECVQ_ADD:
ERR_SENDQ_ADD:
    network_free(loop->network);
ERR_NETWORK:
ERR_SIGNALFD_ADD:
    close(loop->sfd);
ERR_SIGNALFD:
ERR_SIGPROCMASK:
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
    free(loop->events_user);
    free(loop->epoll_events);
    network_free(loop->network);
    close(loop->sfd);
    close(loop->eventfd_user);
    close(loop->eventfd_algorithm);
    close(loop->efd);
    pt_loop_clear_user_events(loop);
    free(loop);
}

// Accessors

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

/**
 * \brief User-defined handler called by libparistraceroute 
 * \param loop The libparistraceroute loop
 * \param algorithm_outputs Information updated by the algorithm
 * \return An integer used by pt_loop()
 *   - <0: there is failure, stop pt_loop
 *   - =0: algorithm has successfully ended, stop the loop
 *   - >0: the algorithm has not yet ended, continue
 */

int pt_loop_process_user_events(pt_loop_t * loop) {
    unsigned        i;
    event_t      ** events     = pt_loop_get_user_events(loop); 
    unsigned int    num_events = pt_loop_get_num_user_events(loop);
    uint64_t        ret;
    ssize_t         count;

    for (i = 0; i < num_events; i++) {
        // decrement the associated eventfd counter
        count = read(loop->eventfd_user, &ret, sizeof(ret));
        if (count == -1)
            return -1;

        loop->handler_user(loop, events[i], loop->user_data);
    }
    return 1;
}


int pt_loop(pt_loop_t *loop, unsigned int timeout)
{
    int n, i, cur_fd;

    // TODO set a flag to avoid issues due to several threads
    // and put a critical section to manage this flag

    int network_sendq_fd   = network_get_sendq_fd(loop->network);
    int network_recvq_fd   = network_get_recvq_fd(loop->network);
    int network_sniffer_fd = network_get_sniffer_fd(loop->network);
    int network_timerfd    = network_get_timerfd(loop->network);
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
                    printf("Read unexpected signal\n");
                }
            } else if (cur_fd == network_timerfd) {
                /* Timeout for first packet in network->probes */
                network_process_timeout(loop->network);
            }
        }
    } while (loop->stop == PT_LOOP_CONTINUE);

    // Process internal events
    return loop->stop == PT_LOOP_TERMINATE ? 0 : -1;
}

// not the right callback here
int pt_send_probe(pt_loop_t *loop, probe_t *probe)
{
    /* We remember which algorithm has generated the probe */
    probe_set_caller(probe, loop->cur_instance);
    probe_set_queueing_time(probe, get_timestamp());

    /* The probe gets assigned a unique ID for the algorithm */
    /* TODO */

    //probe_dump(probe);
    queue_push_element(loop->network->sendq, probe);

    return 0;
}

int pt_loop_terminate(pt_loop_t * loop)
{
    loop->stop = PT_LOOP_TERMINATE;
    return 0;
}

int pt_raise_event(pt_loop_t * loop, event_type_t event_type, void * data)
{
    if (!loop->cur_instance)
        return -1; /* user program cannot raise events */
    pt_algorithm_throw(loop, loop->cur_instance->caller, event_create(event_type, data, loop->cur_instance));
    return 0;
}
