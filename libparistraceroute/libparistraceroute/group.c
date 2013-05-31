#include <stdlib.h>      // malloc ...
#include <sys/timerfd.h> // timerfd_create, timerfd_settime

#include "group.h"
#include "common.h"


group_t * group_create(double (*callback)(void *)) {
    group_t * group;

    if (!(group = malloc(sizeof(group_t))))                         goto ERR_GROUP;
    if (!(group->elements = dynarray_create()))                     goto ERR_ELEMENTS;
    if ((group->timerfd = timerfd_create(CLOCK_REALTIME, 0)) == -1) goto ERR_TIMERFD;
    group->distribution_callback = callback;
    return group;

ERR_TIMERFD :
    dynarray_free(group->elements, NULL);
ERR_ELEMENTS :
    free(group);
ERR_GROUP :
    return NULL; 
}

void group_free(group_t * group)
{ 
    if (group) {
        dynarray_free(group->elements, (ELEMENT_FREE) probe_free);
        close(group->timerfd);
        free(group);
    }
}

inline int group_get_timerfd(group_t * group) {
    return group->timerfd;
}

probe_t * group_get_probe(group_t * group, size_t i) {

    if (!group) return NULL;
    return dynarray_get_ith_element(group->elements, i);
}

bool group_add_probe(group_t * group, probe_t * probe) {
    
    if(!(group && probe)) return false;
    return dynarray_push_element(group->elements, probe);
}

bool group_add_n_probes(group_t * group, probe_t * probes, size_t n) {
   
    bool   ret = true;
    size_t i;

    for(i = 0; ret && i < n; i++) {
        ret &= group_add_probe(group, probes + i);
    }
    return ret;
}











