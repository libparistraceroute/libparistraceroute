#include <stdlib.h>
#include <stdio.h>
#include <search.h>
#include <string.h> // memcpy
#include <libgen.h> // basename

#include <sys/types.h>  // getaddrinfo etc.
#include <sys/socket.h>
#include <netdb.h>

#include "paris-traceroute.h"

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

/******************************************************************************
 * Helper functions
 ******************************************************************************/

static int af = 0; // ?
static char *dst_name = NULL;


// CPPFLAGS += -D_GNU_SOURCE

static int getaddr (const char *name, sockaddr_any *addr) {
    int ret;
    struct addrinfo hints, *ai, *res = NULL;

    memset (&hints, 0, sizeof (hints));
    hints.ai_family = af;
    hints.ai_flags = AI_IDN;

    ret = getaddrinfo (name, NULL, &hints, &res);
    if (ret) {
        fprintf (stderr, "%s: %s\n", name, gai_strerror (ret));
        return -1;
    }

    for (ai = res; ai; ai = ai->ai_next) {
        if (ai->ai_family == af)  break;
        /*  when af not specified, choose DEF_AF if present   */
        if (!af && ai->ai_family == DEF_AF)
            break;
    }
    if (!ai)  ai = res; /*  anything...  */

    if (ai->ai_addrlen > sizeof (*addr))
        return -1;  /*  paranoia   */
    memcpy (addr, ai->ai_addr, ai->ai_addrlen);

    freeaddrinfo (res);

    return 0;
}

static int set_host (char *hostname, sockaddr_any *dst_addr)
{

    if (getaddr (hostname, dst_addr) < 0)
        return -1;

    dst_name = hostname;

    /*  i.e., guess it by the addr in cmdline...  */
    if (!af)  af = dst_addr->sa.sa_family;

    return 0;
}

static char addr2str_buf[INET6_ADDRSTRLEN];

static const char *addr2str (const sockaddr_any *addr) {
    getnameinfo (&addr->sa, sizeof (*addr),
        addr2str_buf, sizeof (addr2str_buf), 0, 0, NI_NUMERICHOST);

    return addr2str_buf;
}

/******************************************************************************
 * Main
 ******************************************************************************/

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
    const char           * algorithm;
    int ret;

    static sockaddr_any dst_addr = {{ 0, }, };
    
    if (opt_parse("usage: %s [options] host", options, argv) != 1) {
        fprintf(stderr, "%s: destination required\n", basename(argv[0]));
        exit(EXIT_FAILURE);
    }
    set_host(argv[1], &dst_addr);
    dst_ip = addr2str(&dst_addr);
    algorithm = algorithms[0];
    printf("Traceroute to %s using algorithm %s\n\n", dst_ip, algorithm);
    
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
    probe_set_payload_size(probe_skel, 32); // probe_set_size XXX
    probe_set_fields(probe_skel, STR("dst_ip", dst_ip), I16("dst_port", 30000), NULL);

    // Prepare options related to the 'mda' algorithm
    traceroute_options_t options = {
        .min_ttl    = 1,        // fields from command line
        .max_ttl    = 30,
        .num_probes = 3,
        .dst_ip     = dst_ip    // probe skeleton
    };
    
    instance = pt_algorithm_add(loop, algorithm, &options, probe_skel);
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
