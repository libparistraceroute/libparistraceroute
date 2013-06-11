#ifndef PROBE_GROUP_H
#define PROBE_GROUP_H

#include "tree.h"
#include "probe.h"

typedef struct {
    tree_t      * tree_probes;  /** Pointer to the tree of probes (if any), NULL otherwise */
} probe_group_t;


/**
 * \brief Create a new probe_group_t instance.
 * \return The newly created probe_group_t instance.
 */

probe_group_t * probe_group_create();


/**
 * \brief Release a probe_group_t instance from the memory.
 * \param probe_group A pointer to a probe_group_t instance.
 */

void probe_group_free(probe_group_t * probe_group);

/**
 * \brief Retrieve the root of a probe_group_t instance
 * \param probe_group A pointer to a probe_group_t instance.
 * \return A pointer to the root (if any), NULL otherwise
 */

tree_node_t * probe_group_get_root(probe_group_t * probe_group);

/**
 * \brief Retrieve the delay of the next scheduled probe(s) .
 * \return The delay of the next probes. 
 */

double probe_group_get_next_delay(const probe_group_t * probe_group);

/**
 * \brief Add a probe in the probe_group
 * \param node_caller The node related to the calling instance (pass NULL if not needed)
 * \return true iif successful 
 */

bool probe_group_add(probe_group_t * probe_group, tree_node_t * node_caller, probe_t * probe);

/**
 * \brief Delete a probe in the probe_group
 * \param node_caller The node related to the calling instance (pass NULL if not needed)
 * \return true iif successful 
 */

bool probe_group_del(tree_node_t * node_caller, size_t index); 

/**
 * \brief Iter on scheduled probes of preo group structure the probe_group.
 * \param node node to explore .
 * \param callback to handle the scheduled probe if found.
 * \param param_callback nedded in the callback to handle the probe.
 */

void probe_group_iter_next_scheduled_probes(tree_node_t * node, void (* callback)(void * param_callback, tree_node_t * node, size_t index), void * param_callback);

/**
 * \brief Retirieve the next delay in the probe_group.
 * \param probe_group the group group instance.
 * \return the delay , -1 otherwise.
 */

double probe_group_get_next_delay(const probe_group_t * probe_group);

/**
 * \brief Dump a probe_group instance.
 * \param probe_group the probe_group instance to dump.
 */

void probe_group_dump(const probe_group_t * probe_group);

#endif
