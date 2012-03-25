#ifndef PARISTRACEROUTE_H
#define PARISTRACEROUTE_H

#include "algorithm.h"
#include "probe.h"
#include "network.h"
#include "list.h"
#include "pt_loop.h"

/******************************************************************************
 * ALGORITHMS                                                                 *
 ******************************************************************************/

void pt_algorithm_add(pt_loop_t *loop, char *name, void *options, probe_t *probe_skel);
void pt_algorithm_terminate(pt_loop_t *loop, algorithm_instance_t *instance);

/******************************************************************************
 * PROBES                                                                     *
 ******************************************************************************/

pt_probe_send(pt_loop_t *loop, probe_t *probe);

#endif
