#ifndef PT_LOOP_H
#define PT_LOOP_H

#include <sys/epoll.h>
#include <sys/eventfd.h>
#include "algorithm.h"
#include "network.h"

typedef enum { LOOP_STARTED, LOOP_STOPPED } loop_status_t;

typedef struct pt_loop_s {
    /* loop status */
    loop_status_t status;
    /* network */
    network_t *network;
    /* algorithms */
    void *algorithm_instances_root;
    unsigned int next_algorithm_id;
    int eventfd_algorithm;
    /* epoll data */
    int efd;
    struct epoll_event *events;
} pt_loop_t;

pt_loop_t* pt_loop_create(void);
void pt_loop_free(pt_loop_t *loop);
void pt_loop_stop(pt_loop_t *loop);

int pt_loop(pt_loop_t *loop, unsigned int timeout);

//void pt_probe_reply_callback(struct pt_loop_s *loop, probe_t *probe, probe_t *reply);
//void pt_probe_send(struct pt_loop_s *loop, probe_t *probe);

#endif
