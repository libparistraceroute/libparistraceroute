#include <stdlib.h> // malloc
#include <stdio.h>  // perror
#include <string.h> // memcpy

#include "pt_loop.h"
#include "probe.h"
#include "algorithm.h"
#include "algorithms/traceroute.h"

/**
 * \brief Handle raised traceroute_event_t events.
 * \param loop The main loop.
 * \param traceroute_event The handled event.
 * \param traceroute_options Options related to this instance of traceroute .
 * \param traceroute_data Data related to this instance of traceroute.
 */

void my_traceroute_handler(
    pt_loop_t                  * loop,
    const traceroute_event_t   * traceroute_event,
    const traceroute_options_t * traceroute_options,
    const traceroute_data_t    * traceroute_data
) {
    const probe_t * probe;
    const probe_t * reply;
    static size_t   num_probes_printed = 0;

    switch (traceroute_event->type) {
        case TRACEROUTE_PROBE_REPLY:
            // Retrieve the probe and its corresponding reply
            probe = ((const probe_reply_t *) traceroute_event->data)->probe;
            reply = ((const probe_reply_t *) traceroute_event->data)->reply;

            // Print reply (i-th reply corresponding to the current hop)
            if (num_probes_printed % traceroute_options->num_probes == 0) {
                printf("%d ", probe_get_field(probe, "ttl")->value.int8);
            }
            printf("%10s ", probe_get_field(reply, "src_ip")->value.string);
            num_probes_printed++;
            break;
        case TRACEROUTE_STAR:
            printf("*");
            num_probes_printed++;
            break;
        case TRACEROUTE_ICMP_ERROR:
            printf("!");
            num_probes_printed++;
            break;
        case TRACEROUTE_DESTINATION_REACHED:
            // The traceroute algorithm has terminated.
            // We could print additional results.
            // Interrupt the main loop.
            // TODO: should we notify the main loop and provoke pt_loop_terminate in main_handler?
            pt_loop_terminate(loop);
            break;
        default:
            // Unhandled event
            break;
    }
    if (num_probes_printed % traceroute_options->num_probes == 0) {
        printf("\n");
    }
}

/**
 * \brief Handle events raised in the main loop 
 * \param loop The main loop.
 * \param event The handled event.
 * \param user_data Data shared in pt_loop_create() 
 */

void main_handler(pt_loop_t * loop, event_t * event, void * user_data)
{
    const traceroute_options_t * traceroute_options;
    const traceroute_data_t    * traceroute_data;
    const traceroute_event_t   * traceroute_event;
    const char                 * algorithm_name;

    switch (event->type) {
        case ALGORITHM_TERMINATED:
            printf("> ALGORITHM_TERMINATED\n");
            pt_loop_terminate(loop);
            break;
        case ALGORITHM_EVENT: // an traceroute-specific event has been raised
            algorithm_name = event->issuer->algorithm->name;
            if (strcmp(algorithm_name, "traceroute") == 0) {
                traceroute_event   = event->data;
                traceroute_options = event->issuer->options;
                traceroute_data    = event->issuer->data;
                my_traceroute_handler(loop, traceroute_event, traceroute_options, traceroute_data);
            }
            break;
        default:
            perror("main_handler: Unhandled event\n");
            break;
    }
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
    int ret = EXIT_FAILURE;
    
    // Harcoded command line parsing here
    char dst_ip[] = "1.1.1.2";

    // Prepare options related to the 'traceroute' algorithm
    traceroute_options_t options = traceroute_get_default_options();
    options.dst_ip = dst_ip;

    // Create libparistraceroute loop
    // No information shared by traceroute algorithm instances, so we pass NULL
    if (!(loop = pt_loop_create(main_handler, NULL))) {
        perror("E: Cannot create libparistraceroute loop ");
        goto ERR_LOOP;
    }

    // Probe skeleton definition: IPv4/UDP probe targetting 'dst_ip'
    if (!(probe_skel = probe_create())) {
        perror("E: Cannot create probe skeleton");
        goto ERR_PROBE_SKEL;
    }

    probe_set_protocols(probe_skel, "ipv4", "udp", NULL);
    probe_set_fields(probe_skel, STR("dst_ip", dst_ip), NULL);
   
    // Instanciate a 'traceroute' algorithm
    if (!(instance = pt_algorithm_add(loop, "traceroute", &options, probe_skel))) {
        perror("E: Cannot add 'traceroute' algorithm");
        goto ERR_INSTANCE;
    }

    // Wait for events. They will be catched by handler_user()
    if (pt_loop(loop, 0) < 0) {
        perror("E: Main loop interrupted");
        goto ERR_PT_LOOP;
    }
    ret = EXIT_SUCCESS;

    // Free data and quit properly
ERR_INSTANCE:
ERR_PROBE_SKEL:
ERR_PT_LOOP:
    pt_loop_free(loop);
ERR_LOOP:
    exit(ret);
}
