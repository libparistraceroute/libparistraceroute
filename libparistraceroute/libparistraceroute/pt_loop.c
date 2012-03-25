#include <stdio.h>
#include "pt_loop.h"

#define MAXEVENTS 100

pt_loop_t* pt_loop_create(void)
{
    int s;
    pt_loop_t * loop;
    struct epoll_event network_sendq_event;
    struct epoll_event network_recvq_event;
    struct epoll_event algorithm_event;
    int network_sendq_fd;
    int network_recvq_fd;

    loop = malloc(sizeof(pt_loop_t));
    loop->status = LOOP_STARTED;

    /* epoll file descriptor */
    loop->efd = epoll_create1(0);
    if (loop->efd == -1) {
        free(loop);
        loop = NULL;
        perror("epoll_create");
        return NULL;
    }
    
    /* eventfd for algorithms */
    loop->eventfd_algorithm = eventfd(0, 0);
    if (loop->eventfd_algorithm == -1) {
        close(loop->efd);
        return NULL;
    }
    algorithm_event.data.fd = loop->eventfd_algorithm;
    algorithm_event.events = EPOLLIN | EPOLLET;
    s = epoll_ctl (loop->efd, EPOLL_CTL_ADD, loop->eventfd_algorithm, &algorithm_event);
    if (s == -1) {
        close(loop->eventfd_algorithm);
        close(loop->efd);
        free(loop);
        loop = NULL;
        perror ("epoll_ctl");
        return NULL;
    }
    
    /* network */
    loop->network = network_create();
    if (!(loop->network)) {
        close(loop->eventfd_algorithm);
        close(loop->efd);
        free(loop);
        loop = NULL;
        return NULL;
    }
    // Error catching

    /* sending queue */
    network_sendq_fd = network_get_sendq_fd(loop->network);
    network_sendq_event.data.fd = network_sendq_fd;
    network_sendq_event.events = EPOLLIN | EPOLLET;
    s = epoll_ctl (loop->efd, EPOLL_CTL_ADD, network_sendq_fd, &network_sendq_event);
    if (s == -1) {
        network_free(loop->network);
        close(loop->eventfd_algorithm);
        close(loop->efd);
        free(loop);
        loop = NULL;
        perror ("epoll_ctl");
        return NULL;
    }
    
    /* receiving queue */
    network_recvq_fd = network_get_recvq_fd(loop->network);
    network_recvq_event.data.fd = network_recvq_fd;
    network_recvq_event.events = EPOLLIN | EPOLLET;
    s = epoll_ctl (loop->efd, EPOLL_CTL_ADD, network_recvq_fd, &network_recvq_event);
    if (s == -1) {
        network_free(loop->network);
        close(loop->eventfd_algorithm);
        close(loop->efd);
        free(loop);
        loop = NULL;
        perror ("epoll_ctl");
        return NULL;
    }

    /* Buffer where events are returned */
    loop->events = calloc (MAXEVENTS, sizeof(struct epoll_event));

    loop->next_algorithm_id = 1; // 0 means unaffected ?

    return loop;
}

void pt_loop_free(pt_loop_t *loop)
{
    network_free(loop->network);
    close(loop->eventfd_algorithm);
    close(loop->efd);
    free(loop);
    loop = NULL;
}

void pt_loop_stop(pt_loop_t *loop)
{
    loop->status = LOOP_STOPPED;
}

void pt_notify_algorithm_fd(pt_loop_t *loop)
{
    eventfd_write(loop->eventfd_algorithm, 1);
}

int pt_loop(pt_loop_t *loop, unsigned int timeout)
{
    int n, i;
    int network_sendq_fd, network_recvq_fd;

    network_sendq_fd = network_get_sendq_fd(loop->network);
    network_recvq_fd = network_get_recvq_fd(loop->network);

    /* Wait for events */
    n = epoll_wait (loop->efd, loop->events, MAXEVENTS, -1);

    /* XXX What kind of events do we have
     * - sockets (packets received, timeouts, etc.)
     * - internal queues where probes and packets are stored
     * - ...
     */

    /* Dispatch events */
    for (i = 0; i < n; i++) {

        /* Handle errors on fds */
        if ((loop->events[i].events & EPOLLERR) ||
            (loop->events[i].events & EPOLLHUP) ||
            (!(loop->events[i].events & EPOLLIN)))
        {
            /* An error has occured on this fd */
            fprintf (stderr, "epoll error\n");
            //close(events[i].data.fd);
            continue;
        }

        
        if (loop->events[i].data.fd == network_sendq_fd) {
            network_process_sendq(loop->network);
        } else if (loop->events[i].data.fd == network_recvq_fd) {
            /* TODO, probe reply dispatch ??? */
            network_process_recvq(loop->network);
        } else if (loop->events[i].data.fd == loop->eventfd_algorithm) {
            pt_algorithm_instance_iter(loop, pt_process_algorithms_instance);
        // } else {
        }
    }

    /* Process internal events */

    return (loop->status == LOOP_STOPPED) ? -1 : 0;
}

