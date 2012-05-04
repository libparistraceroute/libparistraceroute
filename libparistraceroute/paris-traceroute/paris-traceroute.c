#include <stdlib.h>
#include <stdio.h>
#include <search.h>
#include <string.h> // memcpy
#include <libgen.h> // basename
#include "optparse.h"
#include "pt_loop.h"
#include "probe.h"
#include "lattice.h"
#include "algorithm.h"
#include "algorithms/mda.h"

/******************************************************************************
 * Command line stuff                                                         *
 ******************************************************************************/
const char *algorithms[] = {
    "mda", "traceroute", NULL
};
struct opt_spec options[] = {
    {opt_help, "h", "--help", OPT_NO_METAVAR, OPT_NO_HELP, OPT_NO_DATA},
    {opt_store_choice, "a", "--algorithm", "ALGORITHM", "traceroute algorithm: one of "
     "'traceroute', 'mda' [default]", algorithms},
    {OPT_NO_ACTION}
};


// TODO manage properly event allocation and desallocation
void paris_traceroute_handler(pt_loop_t * loop, event_t * event, void *data)
{
    lattice_t * lattice;

    switch (event->type) {
        case ALGORITHM_TERMINATED:
            lattice = event->data;
            lattice_dump(lattice, (ELEMENT_DUMP) mda_interface_dump);
            pt_loop_terminate(loop);
            break;
        default:
            break;
    }
}

int main(int argc, char ** argv)
{
    algorithm_instance_t * instance;
    probe_t              * probe_skel;
    pt_loop_t            * loop;
    char                 * dst_ip;

    int ret;
    
    if (opt_parse("usage: %s [options] host", options, argv) != 1) {
        fprintf(stderr, "%s: destination required\n", basename(argv[0]));
        exit(EXIT_FAILURE);
    }
    dst_ip = argv[1];
    
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

    // Wait for events. They will be catched by handler_user() 
    ret = pt_loop(loop, 0);
    if (ret < 0) goto error;

    // Free data and quit properly
    pt_algorithm_free(instance);

    exit(EXIT_SUCCESS);

ERR_INSTANCE:
ERR_PROBE_SKEL:
    //pt_loop_free(loop);
ERR_LOOP:
error:
    exit(EXIT_FAILURE);
}
