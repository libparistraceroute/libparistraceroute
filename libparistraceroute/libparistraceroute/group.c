#include <stdlib.h>      // malloc ...
#include <sys/timerfd.h> // timerfd_create, timerfd_settime
#include <stdio.h>       //fprintf


#include "group.h"
#include "common.h"


group_t * group_create(double (*callback)(void *)) {
    group_t * group;

    if (!(group = malloc(sizeof(group_t))))                         goto ERR_GROUP;
    if (!(group->probes = dynarray_create()))                       goto ERR_PROBES;
    if ((group->timerfd = timerfd_create(CLOCK_REALTIME, 0)) == -1) goto ERR_TIMERFD;
    group->delay_callback = callback;
    return group;

ERR_TIMERFD :
    dynarray_free(group->probes, NULL);
ERR_PROBES :
    free(group);
ERR_GROUP :
    return NULL; 
}

void group_free(group_t * group)
{ 
    if (group) {
        dynarray_free(group->probes, (ELEMENT_FREE) probe_free);
        close(group->timerfd);
        free(group);
    }
}

inline int group_get_timerfd(const group_t * group) {
    return group->timerfd;
}

probe_t * group_get_probe(const group_t * group, size_t i) {

    if (!group) return NULL;
    return dynarray_get_ith_element(group->probes, i);
}

bool group_add_probe(group_t * group, const probe_t * probe) {
    
    if (!(group && probe)) return false;
    return dynarray_push_element(group->probes, probe);
}

bool group_add_n_probes(group_t * group, probe_t * probes, size_t n) {
   
    bool   ret = true;
    size_t i;

    for (i = 0; ret && i < n; i++) {
        ret &= group_add_probe(group, probes + i);
    }
    return ret;
}

bool group_set_timer(group_t * group) {
    struct itimerspec new_value;
    double delay;
    time_t delay_sec;
    
    if (!(group->delay_callback)) {
        fprintf(stderr, "No delay callback has been defined\n");
        return false;
    }
    delay = group->delay_callback(NULL);    
    delay_sec = (time_t) delay;
    new_value.it_value.tv_sec = delay_sec; 
    new_value.it_value.tv_nsec = 1000000 * (delay_sec - delay);
    new_value.it_interval.tv_sec = 0;
    new_value.it_interval.tv_nsec = 0;
    if (timerfd_settime(group->timerfd, 0, &new_value, NULL) == -1) {
        fprintf(stderr, "Can not update group timerfd\n");
        return false;
    }
    return true;
}










