#include <stdlib.h>
#include <stdio.h>
#include <search.h>
#include <string.h> // memcpy

#include "pt_loop.h"
#include "probe.h"
#include "algorithm.h"
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

// TODO manage properly event allocation and desallocation
void paris_traceroute_handler(void *data)
{
    const traceroute_probe_reply_t * reply;
    traceroute_caller_data_t *_data;

    _data = data;

    switch (_data->type) {
        case TRACEROUTE_PROBE_REPLY:
            reply = &_data->value.probe_reply;
            printf(
                "ttl = %2hu (%d/%d) dst_ip = %s disc_ip = %s\n",
                reply->current_ttl,
                reply->num_sent_probes,
                reply->options->num_probes,
                reply->options->dst_ip,
                reply->discovered_ip
            );
            break;
        case TRACEROUTE_DESTINATION_REACHED:
            break;
        case TRACEROUTE_ALL_STARS:
        case TRACEROUTE_ICMP_ERROR:
            printf("not yet implemented");
            break;
    }
}

int main(int argc, char ** argv)
{
    algorithm_instance_t * instance;
    probe_t              * probe_skel;
    pt_loop_t            * loop;

    int status;
    int ret = EXIT_FAILURE;
    
    // Harcode command line parsing here
    char dst_ip[] = "1.1.1.2";
    // We can get algorithm options from the commandline also

    
    // Create libparistraceroute loop
    loop = pt_loop_create(paris_traceroute_handler);
    if (!loop) {
        perror("E: Cannot create libparistraceroute loop");
        goto ERR_LOOP;
    }
    
    // Probe skeleton definition: IPv4/UDP probe targetting 'dst_ip'
    probe_skel = probe_create();
    if (!probe_skel) {
        perror("E: Cannot create probe skeleton");
        goto ERR_PROBE_SKEL;
    }

    probe_set_protocols(probe_skel, "ipv4", "udp", NULL);
    probe_set_fields(probe_skel, STR("dst_ip", dst_ip), NULL);

    // Prepare options related to the 'mda' algorithm
    traceroute_options_t options = {
        .min_ttl    = 1,        // fields from command line
        .max_ttl    = 30,
        .num_probes = 3,
        .dst_ip     = dst_ip    // probe skeleton
    };
    
    // Instanciate a 'mda' algorithm
    instance = pt_algorithm_add(loop, "mda", &options, probe_skel);
    if (!instance) {
        perror("E: Cannot add 'mda' algorithm");
        goto ERR_INSTANCE;
    }

    do {
        // Wait for events. They will be catched by handler_user() 
        status = pt_loop(loop, 0);
    } while (status > 0);

    // Display results
    printf("******************* END *********************\n");
    ret = EXIT_SUCCESS;

    // Free data and quit properly
    pt_algorithm_free(instance);

ERR_INSTANCE:
ERR_PROBE_SKEL:
    //pt_loop_free(loop);
ERR_LOOP:
    exit(ret);
}
