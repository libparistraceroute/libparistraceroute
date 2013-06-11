#include "probe_group.h"
#include <float.h> // DBL_MAX
#include <stdio.h>

#include "probe.h"
#include "common.h" // MIN

static probe_t * get_node_probe(const tree_node_t * node) {
    return (probe_t *) node->data;
}

static double get_node_delay(const tree_node_t * node) {
    return probe_get_delay(get_node_probe(node));
}

static void update_delay(tree_node_t * node, double delay)
{
    double parent_delay;

    if (node->parent) {
        parent_delay = get_node_delay(node->parent);
        if (parent_delay > delay) {
            probe_set_delay(get_node_probe(node->parent), delay);
            update_delay(node->parent, delay);
        } 
    }
}

probe_group_t * probe_group_create() {
    probe_group_t * probe_group;
    probe_t       * probe; // TODO to remove and tree_node_t points to union {double, probe_t *}

    if (!(probe_group = malloc(sizeof(probe_group_t)))) goto ERR_MALLOC;
    if (!(probe = probe_create())) goto ERR_PROBE_CREATE;
    probe_set_delay(probe, DBL_MAX);
    if (!(probe_group->tree_probes = tree_create(
        (ELEMENT_FREE) probe_free,
        (ELEMENT_DUMP) probe_dump)
    )) goto ERR_TREE_CREATE; 

    if (!(tree_add_root(probe_group->tree_probes, probe))) goto ERR_ADD_ROOT;

    return probe_group;

ERR_ADD_ROOT:
    tree_free(probe_group->tree_probes);
ERR_TREE_CREATE:
    probe_free(probe);
ERR_PROBE_CREATE:
    free(probe_group);
ERR_MALLOC:
    return NULL;
}

void probe_group_free(probe_group_t * probe_group) {
    if (probe_group) {
        if (probe_group->tree_probes) tree_free(probe_group->tree_probes);
        free(probe_group);
    }
}

bool probe_group_add(probe_group_t * probe_group, tree_node_t * node_caller, probe_t * probe) {
    bool   ret = false;
    double delay;

    if (!node_caller) node_caller = probe_group_get_root(probe_group);

    if (tree_node_add_child(node_caller, probe)) {
        delay = probe_get_delay(probe);
        if (get_node_delay(node_caller) > delay) {
            probe_set_delay(get_node_probe(node_caller), delay);            
            update_delay(node_caller, delay);
        }
        ret = true;
    }

    return ret;
}

tree_node_t * probe_group_get_root(probe_group_t * probe_group) {
    return tree_get_root(probe_group->tree_probes);
}

bool probe_group_del(tree_node_t * node_caller, size_t index) 
{
    tree_node_t * node_del;
    size_t        i, num_children;
    double        delay_del, delay;

    if (!(node_del = tree_node_get_ith_child(node_caller, index))) goto ERR_CHILD_NOT_FOUND;

    delay_del = get_node_delay(node_del);
    if (delay_del <= get_node_delay(node_caller)) {
        if (!(tree_node_del_ith_child(node_caller, index))) goto ERR_DEL_CHILD;
        num_children = tree_node_get_num_children(node_caller);
        delay = DBL_MAX;
        for (i = 0; i < num_children; ++i) {
            delay = MIN(delay, get_node_delay(tree_node_get_ith_child(node_caller, i))); 
        }
        probe_set_delay(get_node_probe(node_caller), delay);
        update_delay(node_caller, delay);
        return true;
    }

ERR_DEL_CHILD:
ERR_CHILD_NOT_FOUND:
    return false;
}

void probe_group_iter_next_scheduled_probes(tree_node_t * node, void (* callback)(void * param_callback, tree_node_t * node, size_t index), void * param_callback) {
    tree_node_t * child;
    size_t        i, num_children;
    double        delay = get_node_delay(node);

    // Explore each next scheduled probes
    num_children = tree_node_get_num_children(node);
    for (i = 0; i < num_children; ++i) {
        child = tree_node_get_ith_child(node, i);
        if (!child) {
            fprintf(stderr, "child NOT FOUUUUUND !!!! arg,gklgklgkglkg!!!");
        } else if (tree_node_is_leaf(child) && get_node_delay(child) == delay) { // on ne veut appeler la callback que si la probe est schedulée
            if (callback) callback(param_callback, node, i);
        } else probe_group_iter_next_scheduled_probes(child, callback, param_callback);
    }
}

double probe_group_get_next_delay(const probe_group_t * probe_group) {
    if (!(probe_group)) goto ERR_PROBE_GROUP;
    return get_node_delay(tree_get_root(probe_group->tree_probes)); 

ERR_PROBE_GROUP:
    return -1;
}

void probe_group_dump(const probe_group_t * probe_group) {
    if (probe_group) tree_dump(probe_group->tree_probes);
}
