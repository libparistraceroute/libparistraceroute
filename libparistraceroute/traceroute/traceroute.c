#include <stdlib.h>
#include <stdio.h>
#include <search.h>
#include <string.h> // memcpy

#include "pt_loop.h"
#include "probe.h"
#include "algorithms/traceroute.h"

/**
 *
 */

// TODO manage properly event allocation and desallocation
void handler_traceroute_event(const traceroute_caller_data_t * data) {
    const traceroute_probe_reply_t * reply;
    switch (data->type) {
        case TRACEROUTE_PROBE_REPLY:
            reply = &data->value.probe_reply;
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

/**
 * \brief User-defined handler called by libparistraceroute 
 * \param loop The libparistraceroute loop
 * \param algorithm_outputs Information updated by the algorithm
 * \return An integer used by pt_loop()
 *   - <0: there is failure, stop pt_loop
 *   - =0: algorithm has successfully ended, stop the loop
 *   - >0: the algorithm has not yet ended, continue
 */

int handler_user(pt_loop_t * loop) {
    unsigned        i;
    event_t      ** events     = pt_loop_get_user_events(loop); 
    unsigned int    num_events = pt_loop_get_num_user_events(loop);

    for (i = 0; i < num_events; i++) {
        switch (events[i]->type) {
            case ALGORITHM_TERMINATED:
                printf("> Received ALGORITHM_TERMINATED\n");
                return 0;
            case ALGORITHM_FAILURE:
                printf("> Received ALGORITHM_FAILURE\n");
                return -1;
            case ALGORITHM_ANSWER:
                // TODO carry (algorithm_id, instance_id) in user data ?
                handler_traceroute_event(events[i]->params);
                break;
            default:
                break;
        }
    }
    return 1;
}

/**
 * \brief Main program
 * \param argc Number of arguments
 * \param argv Array of arguments
 * \return Execution code
 */

int main(int argc, char ** argv)
{
    algorithm_instance_t * instance;
    probe_t              * probe_skel;
    pt_loop_t            * loop;

    int status;
    int ret = EXIT_FAILURE;
    
    // Harcode command line parsing here
    char dst_ip[] = "1.1.1.2";

    // Create libparistraceroute loop
    loop = pt_loop_create(handler_user);
    if (!loop) {
        perror("E: Cannot create libparistraceroute loop ");
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
   
    // Prepare options related to the 'traceroute' algorithm
    traceroute_options_t options = {
        .min_ttl    = 1,
        .max_ttl    = 30,
        .num_probes = 3,
        .dst_ip     = dst_ip
    };

    // Instanciate a 'traceroute' algorithm
    if (!(instance = pt_algorithm_add(loop, "traceroute", &options, probe_skel))) {
        perror("E: Cannot add 'traceroute' algorithm");
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
