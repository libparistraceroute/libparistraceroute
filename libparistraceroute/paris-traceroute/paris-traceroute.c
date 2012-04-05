#include <stdlib.h>
#include <stdio.h>
#include <search.h>
#include <string.h> // memcpy

#include "pt_loop.h"
#include "probe.h"
#include "algorithms/traceroute.h"

/******************************************************************************
 * Command line stuff                                                         *
 ******************************************************************************/

//void mda_done(pt_loop_t *pt_loop, mda_options_t *options) { 
//    if (!options) {
//        printf("E: No options\n");
//        exit(EXIT_FAILURE);
//    }
//    pt_loop_stop(pt_loop);
//    //graph_print(options->graph); 
//}

int main(int argc, char ** argv)
{
    algorithm_instance_t * instance;
    probe_t              * probe_skel;
    pt_loop_t            * loop;
    
    loop = pt_loop_create();
    if (!loop) {
        perror("E: Cannot create libparistraceroute instance");
        exit(EXIT_FAILURE);
    }

    // Harcode command line parsing here
    char dst_ip[] = "1.1.1.2";
    // We can get algorithm options from the commandline also

    // Set basic probe constraints
    
    probe_skel = probe_create();
    probe_set_protocols(probe_skel, "ipv4", "udp", NULL);
    probe_set_fields(probe_skel, STR("dst_ip", dst_ip), NULL);
    
    /* We might imagine that this is done by default if given NULL */
    //ip_graph_t *graph = ip_graph_new();
    //mda_options_t opt;
    //= {
        //.graph = graph,
        //.on_done = NULL
    //}
    
    // Prepare options related to the traceroute algorithm
    traceroute_options_t options = {
        .min_ttl = 1,
        .max_ttl = 30,
        .num_probes = 3,
        .dst_ip = dst_ip
    };
/*
    memcpy(&options, &traceroute_default, sizeof(traceroute_default));
    options.dst_ip = dst_ip,
    */

    // Instanciate a traceroute algorithm
    printf("Instanciate 'traceroute' algorithm\n");
    instance = pt_algorithm_add(loop, "traceroute", &options, probe_skel);
    if (!instance) {
        perror("E: Cannot add 'traceroute' algorithm");
        exit(EXIT_FAILURE);
    }

    /* Subscribe to a set of events for display: is that the output of pt_loop ? */
    /* Outputs may be dependent on the algorithm */
    /* We need access to 
     *  - full set of results
     *  - diff
     */
    
    printf("Entering main loop...\n");
    for(;;) {
        /* Now wait for events to arrive */
        if (pt_loop(loop, 0) < 0)
            break;
    }

    /* Display results */

    // Free data and quit properly
    pt_algorithm_terminate(loop, instance);
    exit(EXIT_SUCCESS);
}
