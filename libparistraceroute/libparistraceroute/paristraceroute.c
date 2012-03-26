#include <stdlib.h>
#include <stdio.h>
#include <search.h>

/******************************************************************************
 * Command line stuff                                                         *
 ******************************************************************************/
#include "pt_loop.h"
#include "probe.h" /* needed ? */
#include "algorithm.h"
#include "algorithms/mda.h"
#include "algorithms/traceroute.h"

void mda_done(pt_loop_t *pt_loop, mda_options_t *options) { 
    if (!options) {
        printf("E: No options\n");
        exit(EXIT_FAILURE);
    }
    pt_loop_stop(pt_loop);
    //graph_print(options->graph); 
}

typedef enum algo_e {
    MDA, TRACEROUTE
} algo_t;


int main(int argc, char **argv)
{
    algorithm_instance_t * instance;
    probe_t              * probe_skel;
    pt_loop_t            * loop;
    mda_options_t          mda_opt;
    traceroute_options_t   traceroute_opt;
    
    loop = pt_loop_create();
    if (!loop) {
        perror("Cannot create libparistraceroute instance");
        exit(EXIT_FAILURE);
    }

    // Harcode command line parsing here
    char dst_ip[] = "1.1.1.2";
    // We can get algorithm options from the commandline also

    // Set basic probe constraints
    probe_skel = probe_create();
    probe_set_fields(probe_skel, STR("dst_ip", dst_ip));

    // Instanciate the algorithm
    algo_t algo = TRACEROUTE;
    switch(algo){
        case MDA:
            /* We might imagine that this is done by default if given NULL */
            //ip_graph_t *graph = ip_graph_new();
            //= {
                //.graph = graph,
                //.on_done = NULL
            //}

            instance = pt_algorithm_add(loop, "mda", &mda_opt, probe_skel);
            break;

        case TRACEROUTE:
            instance = pt_algorithm_add(loop, "traceroute", &traceroute_opt, probe_skel);
            break;
    }

    if (!instance) {
        printf("E: cannot add algorithm");
        exit(EXIT_FAILURE);
    }

    /* Subscribe to a set of events for display: is that the output of pt_loop ? */
    /* Outputs may be dependent on the algorithm */
    /* We need access to 
     *  - full set of results
     *  - diff
     */
    
    for(;;) {
        /* Now wait for events to arrive */
        if (pt_loop(loop, 0) < 0)
            break;
    }

    /* Display results */

    exit(EXIT_SUCCESS);
}
