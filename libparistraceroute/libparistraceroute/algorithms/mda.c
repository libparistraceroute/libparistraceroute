/* TODO
 * The algorithm will have to expose a set of options to the commandline if it wants 
 * to be called from it
 */

#include <stdlib.h>
#include <stdbool.h>
#include "stat_test.h"
#include "../probe.h"
#include "../event.h"
#include "../pt_loop.h"
#include "mda.h"

typedef enum { MDA_1, MDA_2 } mda_state_t;

/**
 * internal structure of the MDA algorithm
 *  - implements a FSM
 *  - store MDA internal data
 */
typedef struct {
    mda_state_t state;
} mda_data_t;   

mda_data_t *mda_data_create(void) {
    return malloc(sizeof(mda_data_t));
}

void mda_data_free(mda_data_t *mda_data)
{
    free(mda_data);
    mda_data = NULL;
}

/**
 * \brief CLASSIFY state of the MDA FSM
 */
void mda_classify(pt_loop_t *loop, mda_options_t *options, mda_data_t *data, probe_t *probe_skel, event_t events[])
{
    //algorithm_instance_t *instance;
    /* Process events... */

    /* To classify, we will suppose that the router is a PPLB with at least two
        * interfaces. We will then send enough probes to reject this hypothesis.
        * Also we need to fix the flow_id
        */
    //stat_test_options_t opt;
    //unsigned int constant_flow_id;
    //list_of_interfaces_t ifaces;

    //data->reject = false;

    /* Get an existing flow_id */
    //constant_flow_id = 28; // XXX
    //probe_set_constraint(probe_skel, "flow_id", constant_flow_id);

    /* Prepare input for statistical test */
    //ifaces = list_of_interfaces_new();  /* bof */
    //opt = { .result = ifaces, .confidence = 95 };

    /* We need to define an hypothesis. Maybe stat_test should not be an algorithm */
    //instance = pt_algorithm_add(loop, "stat_test", (void*)&opt, probe_skel);
    //algorithm_start(a);

    /* We return, and wait for the statistical test to complete */

    /* How to get informed ? Do we need to store this state into our FSM ? */

    /* 
        * when we terminate, either we have to wait or not
        * also we might change state or not 
        */
}

/* When an algo received a probe_skel, it has been appended one layer so that we can pop it afterwards */

/**
 * \brief how the algorithm handles the different events
 * \param cache a pointer to a cache instance needed to store the results
 * \param probe_skel a skeleton of a probe that will be used as a model
 * \param data a personal structure that can be used by the algorithm
 * \param events an array of events that have occured since last invocation
 */
void mda_handler(pt_loop_t *loop, algorithm_instance_t *instance)
{ 
    mda_options_t *mda_options;
    mda_data_t *mda_data;
    unsigned int i;
    void *options;
    //probe_t *probe_skel;
    void **data;
    event_t** events;
    unsigned int num_events;

    options = algorithm_instance_get_options(instance);
    //probe_skel = algorithm_instance_get_probe_skel(instance);
    data = algorithm_instance_get_data(instance);
    events = algorithm_instance_get_events(instance);
    num_events = algorithm_instance_get_num_events(instance);

    /* inspect the options = what results the user is interested in */
    mda_options = options;
    //if (!mda_options->graph)
    //    return NULL;    /* terminate algorithm */
    

    for (i = 0; i < num_events; i++) {
        switch (events[i]->type) {
            case ALGORITHM_INIT:
                /* algorithm initialization */
                mda_data = mda_data_create();
                algorithm_instance_set_data(instance, mda_data);
                
//                nq_options_t nq_options = { 
//                    .caller = {
//                        .type = CALLER_ALGORITHM,
//                        .caller_algorithm = algorithm
//                    },
//                    .graph = options->graph,
//                    .exhaustive = true,
//                    .ttl = 1
//                };
//                pt_algorithm_add(loop, "node_query", &nq_options, probe_skel)
                
                break;
            default:
                /* Get the algorithm data structure */
                mda_data = *data;

                /* Previous node query has terminated... continue */
                break;
        }
        pt_algorithm_terminate(loop, instance);
    }
}

static algorithm_t mda = {
    .name="mda",
    .handler=mda_handler
};

ALGORITHM_REGISTER(mda);
