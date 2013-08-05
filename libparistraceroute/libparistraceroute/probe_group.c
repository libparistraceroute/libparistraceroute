#include "config.h"

#include <stdio.h>   // printf
#include <stdlib.h>  // malloc, free
#include <float.h>   // DBL_MAX

#include "probe_group.h"

#include "network.h" // update_timer
#include "common.h"  // MIN

tree_node_probe_t * get_node_data(const tree_node_t * node) {
    return (tree_node_probe_t *) tree_node_get_data(node);
}

static bool is_probe(const tree_node_t * node) {
    tree_node_probe_t * data = get_node_data(node);

    if (node) {
        switch (data->tag) {
            case DOUBLE:
                return false;
            case PROBE:
                return true;
            default:
                return false;
        }
    }
    return false;
}

static double get_node_delay(const tree_node_t * node) {
    tree_node_probe_t * data = get_node_data(node);

    switch (data->tag) {
        case DOUBLE:
            return data->data.delay;
        case PROBE:
            return probe_get_delay((probe_t *)(data->data.probe));
        default:
            return DBL_MAX;
    }
}

double get_node_next_delay(const tree_node_t * node) {
    tree_node_probe_t * data = get_node_data(node);

    switch (data->tag) {
        case DOUBLE:
            return data->data.delay;
        case PROBE:
            return probe_next_delay((probe_t *)(data->data.probe));
        default:
            return DBL_MAX;
    }
}

static void set_node_delay(tree_node_t * node, double delay) {
    field_t           * delay_field;
    tree_node_probe_t * data = get_node_data(node);

    switch (data->tag) {
        case DOUBLE:
            data->data.delay = delay;
            break;
        case PROBE:
            delay_field = DOUBLE("delay", delay);
            probe_set_delay((probe_t *)(data->data.probe), delay_field);
            field_free(delay_field);
            break;
        default:
            fprintf(stderr, "Uknown type of data\n");
            break;
    }
}

void  probe_group_update_delay(probe_group_t * probe_group, tree_node_t * node) {
    double parent_node_delay, node_delay = get_node_delay(node);

    if (node->parent) {
        parent_node_delay = get_node_delay(node->parent);
        if (parent_node_delay > node_delay) {
            set_node_delay(node->parent, node_delay);
            probe_group_update_delay(probe_group, node->parent);
        }
    } else {
        if (probe_group_get_next_delay(probe_group) < DBL_MAX) {
            if (probe_group_get_next_delay(probe_group) - probe_group_get_last_delay(probe_group) < 0) probe_group_set_last_delay(probe_group, 0);
            update_timer(probe_group->scheduling_timerfd, probe_group_get_next_delay(probe_group) - probe_group_get_last_delay(probe_group));
            probe_group_set_last_delay(probe_group, probe_group_get_next_delay(probe_group));
        } else update_timer(probe_group->scheduling_timerfd, 0);
    }
}

void tree_node_probe_free(tree_node_probe_t * tree_node_probe) {
    if (tree_node_probe) {
        if (tree_node_probe->tag == PROBE) probe_free((probe_t *)tree_node_probe);
        free(tree_node_probe);
    }
}

static void tree_node_probe_dump(const tree_node_probe_t * tree_node_probe) {
    if (tree_node_probe) {
        switch (tree_node_probe->tag) {
            case PROBE:
                probe_dump((probe_t *)(tree_node_probe->data.probe));
                break;
            case DOUBLE:
                printf("%lf", (double)tree_node_probe->data.delay);
                break;
        }
    }
}

tree_node_probe_t * tree_node_probe_create(tree_node_tag_t tag, void * data) {
    tree_node_probe_t * tree_node_probe;

    if (!(tree_node_probe = calloc(1, sizeof(tree_node_probe_t)))) goto ERR_CALLOC;
    tree_node_probe->tag = tag;
    switch (tag) {
        case DOUBLE:
            tree_node_probe->data.delay = *((double *) data);
            break;
        case PROBE:
            tree_node_probe->data.probe = (probe_t *) data;
            break;
        default:
            break;
    }
    return tree_node_probe;

ERR_CALLOC:
    return NULL;
}

probe_group_t * probe_group_create(int fd) {
    probe_group_t           * probe_group;
    tree_node_probe_t       * tree_node_probe;
    double                    delay = DBL_MAX;

    if (!(probe_group = malloc(sizeof(probe_group_t)))) goto ERR_MALLOC;
    if (!(tree_node_probe = tree_node_probe_create(DOUBLE, &delay)))  goto ERR_TREE_NODE_PROBE_CREATE;
    if (!(probe_group->tree_probes = tree_create(
        tree_node_probe_free,
        tree_node_probe_dump
    ))) goto ERR_TREE_CREATE;

    if (!(tree_add_root(probe_group->tree_probes, (tree_node_probe_t *)tree_node_probe))) goto ERR_ADD_ROOT;

    probe_group->scheduling_timerfd = fd;
    probe_group->last_delay = 0;
    return probe_group;

ERR_ADD_ROOT:
    tree_free(probe_group->tree_probes);
ERR_TREE_CREATE:
    tree_node_probe_free(tree_node_probe);
ERR_TREE_NODE_PROBE_CREATE:
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

static bool probe_group_add_impl(probe_group_t * probe_group, tree_node_t * node_caller, tree_node_tag_t tag, void * data) {
    bool                ret = false;
    tree_node_t       * new_child;
    tree_node_probe_t * tree_node_probe;

    if (!node_caller) node_caller = probe_group_get_root(probe_group);
    if (!(tree_node_probe = tree_node_probe_create(tag, data))) goto ERR_CREATE;
    if ((new_child = tree_node_add_child(node_caller, tree_node_probe))) {
        probe_group_update_delay(probe_group, new_child);
        ret = true;
    }

    return ret;
ERR_CREATE:
    return false;
}

bool probe_group_add(probe_group_t * probe_group, probe_t * probe) {
    return probe_group_add_impl(probe_group, NULL, PROBE, probe);
}

tree_node_t * probe_group_get_root(probe_group_t * probe_group) {
    return tree_get_root(probe_group->tree_probes);
}

bool probe_group_del(probe_group_t * probe_group, tree_node_t * node_caller, size_t index) {
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
        set_node_delay(node_caller, delay);
        probe_group_update_delay(probe_group, node_caller);

        return true;
    }

ERR_DEL_CHILD:
ERR_CHILD_NOT_FOUND:
    return false;
}

void probe_group_iter_next_scheduled_probes(
    tree_node_t * node,
    void (* callback)(void * param_callback, tree_node_t * node, size_t index),
    void * param_callback
) {
    tree_node_t * child;
    size_t        i, num_children;
    double        delay = get_node_delay(node);

    // Explore each next scheduled probes
    num_children = tree_node_get_num_children(node);
    for (i = 0; i < num_children; ++i) {
        child = tree_node_get_ith_child(node, i);
        if (!child) {
            fprintf(stderr, "child not found\n");
        } else if (is_probe(child) && get_node_delay(child) == delay) {
            if (callback) callback(param_callback, child, i);
            num_children = tree_node_get_num_children(node);
            i = 0;
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

void probe_group_set_last_delay(probe_group_t * probe_group, double new_last_delay) {
    probe_group->last_delay = new_last_delay;
}

double probe_group_get_last_delay(probe_group_t * probe_group) {
    return probe_group->last_delay;
}
