#ifndef PROBE_GROUP_H
#define PROBE_GROUP_H

#include "tree.h"
#include "probe.h"

typedef tree_t tree_probe_t;

typedef enum {
    DOUBLE,         /**< double */
    PROBE           /**< pobe_t */
} tree_node_tag_t;

typedef union {
    double    delay; /**< value of a data as a double */
    probe_t * probe; /**< pointer to probe_t instance  */
} tree_node_data_t;

typedef struct {
    tree_node_tag_t  tag;   /**< tag to know the type of data contained in the node of tree_probe */
    tree_node_data_t data;  /**< data contained in the tree_node of tree_probe                    */
} tree_node_probe_t;

typedef struct {
    tree_probe_t * tree_probes;         /** Pointer to the tree of probes (if any), NULL otherwise */
    int            scheduling_timerfd;
} probe_group_t;

tree_node_probe_t * get_node_data(const tree_node_t * node);

/**
 * \brief Create a new probe_group_t instance.
 * \return The newly created probe_group_t instance.
 */

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
 * \brief Retrieve the delay of the next scheduled probe(s) .
 * \return The delay of the next probes.
 */

double probe_group_get_next_delay(const probe_group_t * probe_group);

/**
 * \brief Add a probe in the probe_group
 * \param probe_group the probe_group_t instance containing the node_caller
 * \param node_caller The node related to the calling instance (pass NULL if not needed)
 * \param tag tag to the type of data contained in The node to add (double or probe_t)
 * \param pointer to the data contained in The node to add (pointer to double or probe_t)
 * \return true iif successful
 */

bool probe_group_add(probe_group_t * probe_group, tree_node_t * node_caller, tree_node_tag_t tag, void * data);

/**
 * \brief Delete a probe in the probe_group
 * \param node_caller The node related to the calling instance (pass NULL if not needed)
 * \return true iif successful
 */

bool probe_group_del(probe_group_t * probe_group, tree_node_t * node_caller, size_t index);

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

/*
 * \brief Get the new delay contained in a node in the probe_group.
 * \param node the node containing the new delay value.
 * \return the value of the next delay in the node
 */

double get_node_next_delay(const tree_node_t * node);

/*
 * \brief update delay from the actuel node up to the root in the probe_group.
 * \param probe_group the probe_group instance to handle .
 * \param node the node containing the delay value.
 * \param delay the delay value.
 */

void update_delay(probe_group_t * probe_group, tree_node_t * node, double delay);

#endif
