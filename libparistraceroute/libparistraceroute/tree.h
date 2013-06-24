#ifndef TREE_H
#define TREE_H

#include <stddef.h>   // size_t
#include "dynarray.h" // dynarray_t

typedef struct tree_node_s {
    struct tree_node_s * parent;       /** Pointer to the parent node (if any), NULL otherwise */
    dynarray_t         * children;     /**< Stores a set of (tree_node_t *) related to each child node */
    size_t               num_children; /**< Number of children of this node */
    void               * data;         /**< Data stored in this node */
} tree_node_t;

typedef struct {
    struct tree_node_s * root;
    void               (* callback_free)(void *); /**< Callback to free data contained in the tree node */
    void               (* callback_dump)(void *); /**< Callback to dump data contained in the tree node */
} tree_t;

//----------------------------------------------------------------------
//                              tree_node_t
//----------------------------------------------------------------------
tree_node_t * tree_node_create(void * data);

void tree_node_free(tree_node_t * node, void (*callback_free)(void * element));

size_t tree_node_get_num_children(const tree_node_t * node);

void * tree_node_get_data(const tree_node_t * node);

tree_node_t * tree_node_get_ith_child(const tree_node_t * node, size_t i);

tree_node_t * tree_node_add_child(tree_node_t * node, void * element);

bool tree_node_push_child(tree_node_t * parent_node, tree_node_t * node);

void tree_node_set_data(tree_node_t * node, void * data);

bool tree_node_del_ith_child(tree_node_t * node, size_t i);

bool tree_node_is_leaf(const tree_node_t * node);

//-------------------------------------------------------------------------
//                                 tree_t
//-------------------------------------------------------------------------


tree_t * tree_create(void (*callback_free)(void *), void (*callback_dump)(void *));

void tree_free(tree_t * tree);

tree_node_t * tree_add_root(tree_t * tree, void * data);

tree_node_t * tree_get_root(tree_t * tree);

void tree_dump(const tree_t * tree);

#endif
