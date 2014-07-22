#include <stdlib.h> // malloc
#include <stdio.h>  // perror
#include <string.h> // memcpy

#include "address.h"
#include "pt_loop.h"
#include "probe.h"
#include "algorithm.h"
#include "algorithms/traceroute.h"

/**
 * \brief Handle events raised in the main loop
 * \param loop The main loop.
 * \param event The handled event.
 * \param user_data Data shared in pt_loop_create()
 */

void loop_handler(pt_loop_t * loop, event_t * event, void * user_data)
{
    traceroute_event_t         * traceroute_event;
    const traceroute_options_t * traceroute_options;
    const traceroute_data_t    * traceroute_data;
    const char                 * algorithm_name;

    switch (event->type) {
        case ALGORITHM_HAS_TERMINATED:
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
                traceroute_handler(loop, traceroute_event, traceroute_options, traceroute_data);
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
    algorithm_instance_t * instance;
    traceroute_options_t   options = traceroute_get_default_options();
    probe_t              * probe;
    pt_loop_t            * loop;
    int                    family;
    int                    ret = EXIT_FAILURE;
    const char           * ip_protocol_name;
    const char           * protocol_name;
    address_t              dst_addr;

    // Harcoded command line parsing here
    char dst_ip[] = "8.8.8.8";
    //char dst_ip[] = "1.1.1.2";
    //char dst_ip[] = "2001:db8:85a3::8a2e:370:7338";

    if (!address_guess_family(dst_ip, &family)) {
        fprintf(stderr, "Cannot guess family of destination address (%s)", dst_ip);
        goto ERR_ADDRESS_GUESS_FAMILY;
    }

    if (address_from_string(family, dst_ip, &dst_addr) != 0) {
        fprintf(stderr, "Cannot guess family of destination address (%s)", dst_ip);
        goto ERR_ADDRESS_FROM_STRING;
    }

    // Prepare options related to the 'traceroute' algorithm
    options.do_resolv = false;
    options.dst_addr = &dst_addr;
    options.num_probes = 1;
//    options.max_ttl = 1;
    printf("num_probes = %lu max_ttl = %u\n", options.num_probes, options.max_ttl);

    // Create libparistraceroute loop
    // No information shared by traceroute algorithm instances, so we pass NULL
    if (!(loop = pt_loop_create(loop_handler, NULL))) {
        fprintf(stderr, "Cannot create libparistraceroute loop");
        goto ERR_LOOP_CREATE;
    }

    // Probe skeleton definition: IPv4/UDP probe targetting 'dst_ip'
    if (!(probe = probe_create())) {
        fprintf(stderr, "Cannot create probe skeleton");
        goto ERR_PROBE_CREATE;
    }

    switch (family) {
        case AF_INET:
            ip_protocol_name = "ipv4";
            protocol_name    = "icmpv4";
            break;
        case AF_INET6:
            ip_protocol_name = "ipv6";
            protocol_name    = "icmpv6";
            break;
        default:
            fprintf(stderr, "Internet family not supported (%d)\n", family);
            goto ERR_FAMILY;
    }
//    protocol_name = "udp";
    protocol_name = "tcp";
    printf("protocol_name = %s\n", protocol_name);

    if (!probe_set_protocols(probe, ip_protocol_name, protocol_name, NULL)) {
        fprintf(stderr, "Can't set protocols %s/%s\n", ip_protocol_name, protocol_name);
        goto ERR_PROBE_SET_PROTOCOLS;
    }

    if (strncmp("icmp", protocol_name, 4) != 0) {
        probe_write_payload(probe, "\0\0\0\0", 4);
    } else {
        probe_set_field(probe, I32("body", 1));
    }

    probe_set_fields(probe,
        ADDRESS("dst_ip", &dst_addr),
        I16("dst_port", 3000),
        NULL
    );

    /*
    if (strcmp("tcp", protocol_name) == 0) {
        printf("setting tcp fields\n");
        uint8_t one = 1;
        probe_set_fields(probe,
            BITS("reserved", 1, &one),
            BITS("ns",  1, &one),
            BITS("cwr", 1, &one),
            BITS("ece", 1, &one),
            BITS("urg", 1, &one),
            BITS("ack", 1, &one),
            BITS("psh", 1, &one),
            BITS("rst", 1, &one),
            BITS("syn", 1, &one),
            BITS("fin", 1, &one),
            NULL
        );
    }
    */
    probe_dump(probe);

    // Instanciate a 'traceroute' algorithm
    if (!(instance = pt_algorithm_add(loop, "traceroute", &options, probe))) {
        fprintf(stderr, "Cannot add 'traceroute' algorithm");
        goto ERR_INSTANCE;
    }

    // Wait for events. They will be catched by handler_user()
    if (pt_loop(loop, 0) < 0) {
        fprintf(stderr, "Main loop interrupted");
        goto ERR_IN_PT_LOOP;
    }
    ret = EXIT_SUCCESS;

    // Free data and quit properly
ERR_IN_PT_LOOP:
    // instance is freed by pt_loop_free
ERR_INSTANCE:
ERR_PROBE_SET_PROTOCOLS:
ERR_FAMILY:
    probe_free(probe);
ERR_PROBE_CREATE:
    pt_loop_free(loop);
ERR_LOOP_CREATE:
ERR_ADDRESS_FROM_STRING:
ERR_ADDRESS_GUESS_FAMILY:
    exit(ret);
}
