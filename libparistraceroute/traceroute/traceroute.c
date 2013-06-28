#include <stdlib.h> // malloc
#include <stdio.h>  // perror
#include <string.h> // memcpy

#include "address.h"
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
    traceroute_event_t         * traceroute_event,
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

            // Print TTL (if this is the first probe related to this TTL)
            if (num_probes_printed % traceroute_options->num_probes == 0) {
                uint8_t ttl;
                if (probe_extract(probe, "ttl", &ttl)) {
                    printf("%-2d", ttl);
                }
            }

            // Print discovered IP
            {
                char * discovered_ip;
                if (probe_extract(reply, "src_ip", &discovered_ip)) {
                    printf(" %-16s ", discovered_ip);
                    free(discovered_ip);
                }
            }

            // Print delay
            {
                double send_time = probe_get_sending_time(probe),
                       recv_time = probe_get_recv_time(reply);
                printf(" (%-5.2lfms) ", 1000 * (recv_time - send_time));
            }
            num_probes_printed++;
            break;
        case TRACEROUTE_STAR:
            printf(" *");
            num_probes_printed++;
            break;
        case TRACEROUTE_ICMP_ERROR:
            printf(" !");
            num_probes_printed++;
            break;
        case TRACEROUTE_TOO_MANY_STARS:
            printf("Too many stars\n");
            break;
        case TRACEROUTE_MAX_TTL_REACHED:
            printf("Max ttl reached\n");
            break;
        case TRACEROUTE_DESTINATION_REACHED:
            // The traceroute algorithm has terminated.
            // We could print additional results.
            // Interrupt the main loop.
            printf("Destination reached\n");
            break;
        default:
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

void algorithm_handler(pt_loop_t * loop, event_t * event, void * user_data)
{
    traceroute_event_t         * traceroute_event;
    const traceroute_options_t * traceroute_options;
    const traceroute_data_t    * traceroute_data;
    const char                 * algorithm_name;

    switch (event->type) {
        case ALGORITHM_TERMINATED:
            printf("> ALGORITHM_TERMINATED\n");
            pt_instance_stop(loop, event->issuer); // release traceroute's data from the memory
            pt_loop_terminate(loop);               // we've only run one 'traceroute' algorithm, so we can break the main loop
            break;
        case ALGORITHM_EVENT: // a traceroute-specific event has been raised
            algorithm_name = event->issuer->algorithm->name;
            if (strcmp(algorithm_name, "traceroute") == 0) {
                traceroute_event   = event->data;
                traceroute_options = event->issuer->options;
                traceroute_data    = event->issuer->data;
                my_traceroute_handler(loop, traceroute_event, traceroute_options, traceroute_data);
            }
            break;
        default:
            break;
    }

    // Release the event, its nested traceroute_event (if any), its attached probe and reply (if any)
    //event_free(event); // TODO this may provoke seg fault in case of stars
}

/**
 * \brief Main program
 * \param argc Number of arguments
 * \param argv Array of arguments
 * \return Execution code
 */

int main(int argc, char ** argv)
{
    buffer_t             * payload;
    algorithm_instance_t * instance;
    traceroute_options_t   options = traceroute_get_default_options();
    probe_t              * probe;
    pt_loop_t            * loop;
    int                    family;
    int                    ret = EXIT_FAILURE;
    const char           * ip_protocol_name;
    //    const char           * message = "@ABCDEFGHIJKLMNOPQRSTUVWXYZ[ \\]^_";

    // Harcoded command line parsing here
    //char dst_ip[] = "173.194.78.104";
   // char dst_ip[] = "8.8.8.8";
    char dst_ip[] = "1.1.1.2";
    //char dst_ip[] = "2001:db8:85a3::8a2e:370:7338";

    if (!address_guess_family(dst_ip, &family)) {
        fprintf(stderr, "E: Cannot guess family of destination address (%s)", dst_ip);
        goto ERR_ADDRESS_GUESS_FAMILY;
    }

    switch (family) {
        case AF_INET:
            ip_protocol_name = "ipv4";
            break;
        case AF_INET6:
            ip_protocol_name = "ipv6";
            break;
        default:
            fprintf(stderr, "Internet family not supported (%d)\n", family);
            goto ERR_FAMILY;
    }

    if (!(payload = buffer_create())) {
        fprintf(stderr, "E: Cannot allocate payload buffer");
        goto ERR_BUFFER_CREATE;
    }

//    buffer_set_data(payload, message, strlen(message) - 1);
    buffer_write_bytes(payload, "\0\0", 2);

    // Prepare options related to the 'traceroute' algorithm
    options.dst_ip = dst_ip;
    options.num_probes = 1;
    //options.max_ttl = 1;
    printf("num_probes = %lu max_ttl = %u\n", options.num_probes, options.max_ttl);

    // Create libparistraceroute loop
    // No information shared by traceroute algorithm instances, so we pass NULL
    if (!(loop = pt_loop_create(algorithm_handler, NULL))) {
        fprintf(stderr, "E: Cannot create libparistraceroute loop");
        goto ERR_LOOP_CREATE;
    }

    // Probe skeleton definition: IPv4/UDP probe targetting 'dst_ip'
    if (!(probe = probe_create())) {
        fprintf(stderr, "E: Cannot create probe skeleton");
        goto ERR_PROBE_CREATE;
    }

    probe_set_protocols(probe, ip_protocol_name, "udp", NULL);
    probe_write_payload(probe, payload);
    probe_set_fields(probe,
        STR("dst_ip",   dst_ip),
        I16("dst_port", 30000),
        NULL
    );

    probe_set_delay(probe, DOUBLE("delay", 0));
    probe_dump(probe);

    // Instanciate a 'traceroute' algorithm
    if (!(instance = pt_algorithm_add(loop, "traceroute", &options, probe))) {
        fprintf(stderr, "E: Cannot add 'traceroute' algorithm");
        goto ERR_INSTANCE;
    }

    // Wait for events. They will be catched by handler_user()
    if (pt_loop(loop, 0) < 0) {
        fprintf(stderr, "E: Main loop interrupted");
        goto ERR_IN_PT_LOOP;
    }
    ret = EXIT_SUCCESS;

    // Free data and quit properly
ERR_IN_PT_LOOP:
    // instance is freed by pt_loop_free
ERR_INSTANCE:
    probe_free(probe);
ERR_PROBE_CREATE:
    pt_loop_free(loop);
ERR_LOOP_CREATE:
    buffer_free(payload);
ERR_BUFFER_CREATE:
ERR_FAMILY:
ERR_ADDRESS_GUESS_FAMILY:
    exit(ret);
}
