#ifndef PROBE_GROUP_H
#define PROBE_GROUP_H

#include <stddef.h> // size_t

#include "tree.h"   // tree_t
#include "probe.h"  // probe_t

typedef tree_t tree_probe_t;

typedef enum {
    DOUBLE,                 /**< The node stores a double.      */
    PROBE                   /**< The node stores a (probe_t *). */
} tree_node_tag_t;

typedef union {
    double    delay;        /**< Value of a data as a double.  */
    probe_t * probe;        /**< Pointer to probe_t instance.  */
} tree_node_data_t;

typedef struct {
    tree_node_tag_t  tag;   /**< Tag to know the type of data stored in the node. */
    tree_node_data_t data;  /**< Data stored in the node. */
} tree_node_probe_t;


typedef struct {
    tree_probe_t * tree_probes;          /**< Point to the tree of probes (if any), NULL otherwise. */
    int            scheduling_timerfd;   /**< A timerfd which expires when a scheduled probe must be sent. */
    double         last_delay;           /**< The time reference to schedule the probes to send. It is set when the first node is added. */
} probe_group_t;

tree_node_probe_t * get_node_data(const tree_node_t * node);

/**
 * \brief Create a new probe_group_t instance.
 * \param timerfd The timerfd managed by the probe_group
 * \return The newly created probe_group_t instance.
 */

// TODO akram this timerfd should be created by the probe group and reused by the network layer
probe_group_t * probe_group_create(int fd);

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
 * \brief Add a probe in the probe_group.
 * \param probe_group A probe_group_t instance.
 * \param probe A probe instance that we add in the probe group.
 * \return true iif successful.
 */

bool probe_group_add(probe_group_t * probe_group, probe_t * probe);

/**
 * \brief Delete a probe from the probe_group.
 * \param node_caller The node related to the calling instance (pass NULL if not needed).
 * \return true iif successful.
 */

bool probe_group_del(probe_group_t * probe_group, tree_node_t * node_caller, size_t index);

/**
 * \brief Iterate on scheduled probes of probe_group_t structure.
 * \param node Node to explore.
 * \param callback Function called for each probe that be send.
 * \param param_callback This pointer is passed to the callback.
 */

void probe_group_iter_next_scheduled_probes(
    tree_node_t * node,
    void (* callback)(void * param_callback, tree_node_t * node, size_t index),
    void * param_callback
);

/**
 * \brief Retrieve the next delay in the probe_group.
 * \param probe_group The probe_group_t instance.
 * \return The delay, -1 otherwise.
 */

double probe_group_get_next_delay(const probe_group_t * probe_group);

/**
 * \brief Dump A probe_group instance.
 * \param probe_group The probe_group instance to dump.
 */

void probe_group_dump(const probe_group_t * probe_group);

/*
 * \brief Get the new delay contained in a node in the probe_group.
 * \param node The node containing the new delay value.
 * \return The value of the next delay in the node
 */

double get_node_next_delay(const tree_node_t * node);

/*
 * \brief Update delay from the actuel node up to the root in the probe_group.
 * \param probe_group The probe_group instance to handle .
 * \param node The node containing the delay value.
 * \param delay The delay value.
 */

void probe_group_update_delay(probe_group_t * probe_group, tree_node_t * node);

double probe_group_get_last_delay(probe_group_t * probe_group);

void probe_group_set_last_delay(probe_group_t * probe_group, double new_last_delay);
#endif
