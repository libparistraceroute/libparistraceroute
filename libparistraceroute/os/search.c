#include "search.h"

#ifdef FREEBSD
#include <stdlib.h>
#include "../common.h" // ELEMENT_FREE

void _tdestroy_recurse(node_t * node, void (*element_free)(void *)) {
    if (node->llink) _tdestroy_recurse(node->llink, element_free);
    if (node->rlink) _tdestroy_recurse(node->rlink, element_free);
    element_free(node->key);
    free(node);
}

void tdestroy(void *proot, void (*element_free)(void *)) {
    node_t * root = proot;
    if (root) _tdestroy_recurse(root, (ELEMENT_FREE) element_free);
}

#endif
