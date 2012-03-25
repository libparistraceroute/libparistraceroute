#include "stat_test.h"
#include "../pt_loop.h"
#include "../algorithm.h"

void stat_test_handler(pt_loop_t *loop, algorithm_instance_t *instance)
{
    unsigned int i;
    void *options;
    probe_t *probe_skel;
    void **data;
    event_t** events;
    unsigned int num_events;

    options = algorithm_instance_get_options(instance);
    probe_skel = algorithm_instance_get_probe_skel(instance);
    data = algorithm_instance_get_data(instance);
    events = algorithm_instance_get_events(instance);
    num_events = algorithm_instance_get_num_events(instance);
    
    for (i = 0; i < num_events; i++) {
        switch (events[i]->type) {
            case REPLY_RECEIVED:
                break;
            default:
                perror("Unexpected event");
        }
    }
} 

static algorithm_t stat_test = {
    .name="stat_test",
    .handler=stat_test_handler
};

ALGORITHM_REGISTER(stat_test);
