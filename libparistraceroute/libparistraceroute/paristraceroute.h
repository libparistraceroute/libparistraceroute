#ifndef PARISTRACEROUTE_H
#define PARISTRACEROUTE_H

/**
 * \file paristraceroute.h
 * \brief Main header for the library
 */

#include "algorithm.h"
#include "probe.h"
#include "network.h"
#include "list.h"
#include "pt_loop.h"

/******************************************************************************
 * ALGORITHMS                                                                 *
 ******************************************************************************/

/**
 * \brief Add an algorithm to a loop
 * \param loop Pointer to a pt_loop_t structure containing the loop to use
 * \param name Pointer to the name of the algorithm to use
 * \param options Pointer to an array of options to use for the algorithm
 * \param probe_skel Pointer to an incomplete probe_t structure that will contain the probe to be used
 * \return None
 */
void pt_algorithm_add(pt_loop_t *loop, char *name, void *options, probe_t *probe_skel);
/**
 * \brief Terminate an algorithm
 * \param loop Pointer to a pt_loop_t structure containing the loop in which the algorithm is being used
 * \param instance Pointer to an instance of an algorithm_instance_t structure to delete
 * \return None
 */
void pt_algorithm_terminate(pt_loop_t *loop, algorithm_instance_t *instance);

/******************************************************************************
 * PROBES                                                                     *
 ******************************************************************************/

/**
 * \brief Send a probe
 * \param loop Pointer to a pt_loop_t structure containing the loop to use
 * \param probe Pointer to a probe_t structure describing the probe to send
 * \return None
 */
void pt_probe_send(pt_loop_t *loop, probe_t *probe);

#endif
