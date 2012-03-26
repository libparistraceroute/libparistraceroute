/* TODO
 * The algorithm will have to expose a set of options to the commandline if it wants 
 * to be called from it
 */

#include <stdlib.h>
//#include <stdbool.h>
//#include "stat_test.h"
#include "../probe.h"
#include "../event.h"
#include "../pt_loop.h"
#include "traceroute.h"

typedef struct traceroute_data_s {
    // Store internal data (FSM, ...)
} traceroute_data_t;   

/**
 * \brief Allocate internal data needed by traceroute
 * \return The corresponding structure
 */

traceroute_data_t *traceroute_data_create(void) {
    return malloc(sizeof(traceroute_data_t));
}

/**
 * \brief Free internal data 
 * \param traceroute_data A pointer to internal data
 */

void traceroute_data_free(traceroute_data_t *traceroute_data)
{
    if(traceroute_data) free(traceroute_data);
}

/**
 * \brief How the algorithm handles the different events
 * \param loop Paris Traceroute's loop
 * \param instance The instance carry every relevant information from
 *  the upper scopes.
 */

void traceroute_handler(pt_loop_t * loop, algorithm_instance_t * instance)
{ 
    traceroute_options_t * traceroute_options;
    traceroute_data_t    * traceroute_data;
    unsigned int           i;
    void                 * options;
    probe_t              * probe_skel;
    void                ** data;
    event_t             ** events;
    unsigned int           num_events;
    probe_t              * probe;
    field_t              * field_ttl;

    options    = algorithm_instance_get_options(instance);
    probe_skel = algorithm_instance_get_probe_skel(instance);
    data       = algorithm_instance_get_data(instance);
    events     = algorithm_instance_get_events(instance);
    num_events = algorithm_instance_get_num_events(instance);

    // Check options (return if not correct) 

    // For each events, execute the appriopriate code
    for (i = 0; i < num_events; i++) {
        switch (events[i]->type) {
            case ALGORITHM_INIT:

                // Algorithm initialization
                traceroute_data = traceroute_data_create();
                algorithm_instance_set_data(instance, traceroute_data);

                // Create a probe with ttl = 1 and send it
                probe = probe_create();
                field_ttl = field_create_int8("ttl", 1);
                probe_set_fields(probe, field_ttl);
                pt_probe_send(loop, probe);
                field_free(field_ttl); 
                probe_free(probe);
                break;

            case REPLY_RECEIVED: break;

            case ALGORITHM_TERMINATED:

                traceroute_data_free(traceroute_data);
                break;
        }
        pt_algorithm_terminate(loop, instance);
    }
}

static algorithm_t traceroute = {
    .name    = "traceroute",
    .handler = traceroute_handler
};

ALGORITHM_REGISTER(traceroute);
