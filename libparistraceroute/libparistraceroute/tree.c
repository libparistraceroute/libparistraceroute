#include <stdlib.h>
#include <stdbool.h>
#include <stdio.h>

#include "tree.h"

tree_node_t * tree_node_create(void * data) {
    tree_node_t * node;
    if (!(node = malloc(sizeof(tree_node_t)))) goto ERR_CALLOC; 
    if (!(node->children = dynarray_create())) goto ERR_DYNARRAY_CREATE;
    node->data = data; 
    node->parent = NULL;
    return node;

ERR_DYNARRAY_CREATE:
ERR_CALLOC:
    return NULL;
}

void tree_node_free(tree_node_t * node, void (*callback_free)(void * element)) {
    size_t i;

    // Be careful because the parent (if any) now points to an invalid address
    if (node) {
        for(i = 0; i < tree_node_get_num_children(node); i++) {       
            tree_node_free(tree_node_get_ith_child(node, i), callback_free);
        }
        dynarray_free(node->children, callback_free);
    }
    free(node);
}

tree_node_t * tree_node_add_child(tree_node_t * node, void * element) {
    tree_node_t * child;

    if (!node)                                         goto ERR_PARENT_NODE;
    if (!(child = tree_node_create(element)))          goto ERR_TREE_NODE_CREATE;
    if (!dynarray_push_element(node->children, child)) goto ERR_PUSH_CHILD;

    child->parent = node;
    return child;

ERR_PUSH_CHILD:
    tree_node_free(child, NULL); // TODO move callback_free of dynarray into dynarray_create
ERR_TREE_NODE_CREATE:
ERR_PARENT_NODE:
    return NULL;
}

bool tree_node_push_child(tree_node_t * parent_node, tree_node_t * node) {

    if (parent_node && node) {
        dynarray_push_element(parent_node->children, node);
        node->parent = parent_node;
        return true;
    }
    return false;
}

tree_node_t * tree_node_get_parent(const tree_node_t * node) {
    return node->parent;
}

bool tree_node_is_leaf(const tree_node_t * node) {
    return tree_node_get_num_children(node) == 0;
}

bool tree_node_is_root(const tree_node_t * node) {
    return tree_node_get_parent(node) == NULL;
}

tree_node_t * tree_node_get_ith_child(const tree_node_t * node, size_t i) {
    return dynarray_get_ith_element(node->children, i);
}

bool tree_node_del_ith_child(tree_node_t * node, size_t i) {
    tree_node_t * child;

    if (!(child = dynarray_get_ith_element(node->children, i))) {
        goto ERR_GET_CHILD;
    }
    child->parent = NULL;
    return dynarray_del_ith_element(node->children, i, NULL);

ERR_GET_CHILD:
    return false;
}

size_t tree_node_get_num_children(const tree_node_t * node) {
    return node->children ? dynarray_get_size(node->children) : 0;
}

void * tree_node_get_data(const tree_node_t * node) {
    return node->data;
}

void tree_node_set_data(tree_node_t * node, void * data) {
    if (node) {
        node->data = data;
    }
}

void tree_node_dump(tree_node_t * node, void (*callback_dump)(void *), size_t indent) {
    size_t i;

    if (indent > 3) return;
    if (node) {
        if (callback_dump) {
            printf("                                   node at level %lu\n\n", indent);
            /*
            for (i = 0; i < 4 * indent; i++) {
                printf("    ");
            }
            */
           if(node->data){ 
            callback_dump(node->data);
            printf("\n");
           } else printf("No data in this node\n\n");
            for (i = 0; i < tree_node_get_num_children(node); i++) {       
                tree_node_dump(tree_node_get_ith_child(node, i), callback_dump, indent + 1);
            }
        }
    }
}

tree_t * tree_create(void(* callback_free)(void *), void(* callback_dump)(void *)) {
    tree_t * tree;

    if (!(tree = calloc(1, sizeof(tree_t)))) goto ERR_CALLOC;
    tree->callback_free = callback_free;
    tree->callback_dump = callback_dump;
    return tree;

ERR_CALLOC:
    return NULL;  
}

void tree_free(tree_t * tree) {
    if (tree) {
        if (tree->root) tree_node_free(tree->root, tree->callback_free);
    }
}

tree_node_t * tree_add_root(tree_t * tree, void * data) {
    tree->root = tree_node_create(data);
    return tree->root;
}

tree_node_t * tree_get_root(tree_t * tree) { 
    return tree ? tree->root : NULL;
}

void tree_dump(const tree_t * tree) {
    if (tree->root) {
        printf("****************** tree *******************\n");
        tree_node_dump(tree->root, tree->callback_dump, 0); 
    }
}
