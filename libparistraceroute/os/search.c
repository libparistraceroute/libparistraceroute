#include "search.h"
#ifdef FREEBSD

void tdestroy(void *root, void (*free_node)(void *nodep)) {
    fprintf(stderr, "os: tdestroy: NOT IMPLEMENTED");
}

#endif
