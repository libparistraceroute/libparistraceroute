#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include "pt_loop.h"

#define MAXEVENTS 100

pt_loop_t* pt_loop_create(void)
{
    int s;
    pt_loop_t * loop;
    struct epoll_event network_sendq_event;
    struct epoll_event network_recvq_event;
    struct epoll_event network_sniffer_event;
    struct epoll_event algorithm_event;
    int network_sendq_fd;
    int network_recvq_fd;
    int network_sniffer_fd;

    loop = malloc(sizeof(pt_loop_t));
    loop->status = LOOP_STARTED;

    /* epoll file descriptor */
    loop->efd = epoll_create1(0);
    if (loop->efd == -1)
        goto err_epoll;
    
    /* eventfd for algorithms */
    loop->eventfd_algorithm = eventfd(0, 0);
    if (loop->eventfd_algorithm == -1)
        goto err_eventfd;
    algorithm_event.data.fd = loop->eventfd_algorithm;
    algorithm_event.events = EPOLLIN | EPOLLET;
    s = epoll_ctl (loop->efd, EPOLL_CTL_ADD, loop->eventfd_algorithm, &algorithm_event);
    if (s == -1)
        goto err_eventfd_add;
    
    /* network */
    loop->network = network_create();
    if (!(loop->network))
        goto err_network;

    /* sending queue */
    network_sendq_fd = network_get_sendq_fd(loop->network);
    network_sendq_event.data.fd = network_sendq_fd;
    network_sendq_event.events = EPOLLIN | EPOLLET;
    s = epoll_ctl (loop->efd, EPOLL_CTL_ADD, network_sendq_fd, &network_sendq_event);
    if (s == -1)
        goto err_sendq_add;
    
    /* receiving queue */
    network_recvq_fd = network_get_recvq_fd(loop->network);
    network_recvq_event.data.fd = network_recvq_fd;
    network_recvq_event.events = EPOLLIN | EPOLLET;
    s = epoll_ctl (loop->efd, EPOLL_CTL_ADD, network_recvq_fd, &network_recvq_event);
    if (s == -1) 
        goto err_recvq_add;

    /* sniffer */
    network_sniffer_fd = network_get_sniffer_fd(loop->network);
    network_sniffer_event.data.fd = network_sniffer_fd;
    network_sniffer_event.events = EPOLLIN | EPOLLET;
    s = epoll_ctl (loop->efd, EPOLL_CTL_ADD, network_sniffer_fd, &network_sniffer_event);
    if (s == -1)
        goto err_sniffer_add;

    /* Buffer where events are returned */
    loop->events = calloc (MAXEVENTS, sizeof(struct epoll_event));

    loop->next_algorithm_id = 1; // 0 means unaffected ?

    return loop;

err_sendq_add:
err_recvq_add:
err_sniffer_add:
    network_free(loop->network);
err_eventfd_add:
err_network:
    close(loop->eventfd_algorithm);
err_eventfd:
    close(loop->efd);
err_epoll:
    free(loop);
    loop = NULL;

    return NULL;
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
    int network_sendq_fd, network_recvq_fd, network_sniffer_fd;

    network_sendq_fd = network_get_sendq_fd(loop->network);
    network_recvq_fd = network_get_recvq_fd(loop->network);
    network_sniffer_fd = network_get_sniffer_fd(loop->network);

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
            printf("INFO: pt_loop network_sendq received activity\n");
            network_process_sendq(loop->network);
        } else if (loop->events[i].data.fd == network_recvq_fd) {
            printf("INFO: pt_loop network_recvq received activity\n");
            network_process_recvq(loop->network);
        } else if (loop->events[i].data.fd == network_sniffer_fd) {
            printf("INFO: pt_loop network_sniffer received activity\n");
            network_process_sniffer(loop->network);
        } else if (loop->events[i].data.fd == loop->eventfd_algorithm) {
            printf("INFO: pt_loop algorithm received activity\n");
            pt_algorithm_instance_iter(loop, pt_process_algorithms_instance);
        // } else {
        }
    }

    /* Process internal events */

    return (loop->status == LOOP_STOPPED) ? -1 : 0;
}

