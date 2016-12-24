#ifndef OS_SEARCH
#define OS_SEARCH

#include "os.h"

#ifdef LINUX
#  include <search.h>
#endif

#ifdef FREEBSD
#  include <search.h>
#  include <stdio.h>

void tdestroy(void *root, void (*free_node)(void *nodep));

#endif

#endif // OS_SEARCH
