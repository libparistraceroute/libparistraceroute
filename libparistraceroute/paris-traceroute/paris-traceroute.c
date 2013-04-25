#include <stdlib.h>
#include <stdio.h>
#include <search.h>
#include <string.h> // memcpy
#include <libgen.h> // basename
#include <limits.h>
#include <sys/types.h>  // getaddrinfo etc.
#include <sys/socket.h>
#include <netdb.h>

//#include "paris-traceroute.h"
#include "optparse.h"
#include "pt_loop.h"
#include "probe.h"
#include "lattice.h"
#include "algorithm.h"
#include "algorithms/mda.h"
#include "algorithms/traceroute.h"
#include "address.h"

/******************************************************************************
 * Command line stuff                                                         *
 ******************************************************************************/

const char * algorithms[] = {
    "mda", "traceroute", "paris-traceroute", NULL
};
static unsigned    is_ipv4 = 1;
static unsigned    is_udp = 0;
static unsigned    do_res = 1;

const char *protocols[] = {
    "udp", NULL
};

//                              def    min  max
static unsigned first_ttl[3] = {1,     1,   255};
static unsigned max_ttl[3]   = {30,    1,   255};
static double   wait[3]      = {5,     0,   INT_MAX};
static unsigned dport[3]     = {30000, 0,   65535};
static unsigned sport[3]     = {3083,  0,   65535};

//                              def1 min1 max1 def2 min2 max2      mda_enabled
static unsigned mda[7]       = {95,  0,   100, 5,   1,   INT_MAX , 0};

#define HELP_4 "Use IPv4"
#define HELP_P "Use raw packet of protocol prot for tracerouting: one of 'udp' [default]"
#define HELP_U "Use UDP to particular port for tracerouting (instead of increasing the port per each probe),default port is 53"
#define HELP_f "Start from the first_ttl hop (instead from 1), first_ttl must be between 1 and 255"
#define HELP_m "Set the max number of hops (max TTL to be reached). Default is 30, max_ttl must must be between 1 and 255"
#define HELP_n "Do not resolve IP addresses to their domain names"
#define HELP_w "Set the number of seconds to wait for response to a probe (default is 5.0)"
#define HELP_M "Multipath tracing  bound: an upper bound on the probability that multipath tracing will fail to find all of the paths (default 0.05) max_branch: the maximum number of branching points that can be encountered for the bound still to hold (default 5)"
#define HELP_a "Traceroute algorithm: one of  'mda' [default],'traceroute', 'paris-traceroute'"
#define HELP_d "set PORT as destination port (default: 30000)"
#define HELP_s "set PORT as source port (default: 3083)"
struct opt_spec cl_options[] = {
    // action               sf   lf                   metavar             help         data
    {opt_help,              "h", "--help"   ,         OPT_NO_METAVAR,     OPT_NO_HELP, OPT_NO_DATA},
    {opt_version,           "V", "--version",         OPT_NO_METAVAR,     OPT_NO_HELP, "version 1.0"},
    {opt_store_choice,      "a", "--algo",            "ALGORITHM",        HELP_a,      algorithms},
    {opt_store_1,           "4", OPT_NO_LF,           OPT_NO_METAVAR,     HELP_4,      &is_ipv4},
    {opt_store_choice,      "P", "--protocol",        "protocol",         HELP_P,      protocols},
    {opt_store_1,           "U", "--UDP",             OPT_NO_METAVAR,     HELP_U,      &is_udp},
    {opt_store_int_lim,     "f", "--first",           "first_ttl",        HELP_f,      first_ttl},
    {opt_store_int_lim,     "m", "--max-hops",        "max_ttl",          HELP_m,      max_ttl},
    {opt_store_0,           "n", OPT_NO_LF,           OPT_NO_METAVAR,     HELP_n,      &do_res},
    {opt_store_double_lim,  "w", "--wait",            "waittime",         HELP_w,      wait},
    {opt_store_int_2,       "M", "--mda",             "bound,max_branch", HELP_M,      mda},
    {opt_store_int_lim,     "s", "--source_port",     "PORT",             HELP_s,      sport},
    {opt_store_int_lim,     "d", "--dest_port",       "PORT",             HELP_d,      dport},
    {OPT_NO_ACTION},
};

/******************************************************************************
 * Program data
 ******************************************************************************/

typedef struct {
    const char * algorithm;
    const char * dst_ip;
    void       * options;
} paris_traceroute_data_t;


/******************************************************************************
 * Main
 ******************************************************************************/


void result_dump(lattice_elt_t * elt)
{
    unsigned int      i, num_next;
    mda_interface_t * link[2];
    
    link[0] = lattice_elt_get_data(elt);

    num_next = dynarray_get_size(elt->next);
    if (num_next == 0) {
        link[1] = NULL;
        mda_link_dump(link, do_res);
    }
    for (i = 0; i < num_next; i++) {
        lattice_elt_t *iter_elt;

        iter_elt = dynarray_get_ith_element(elt->next, i);
        link[1] = lattice_elt_get_data(iter_elt);

        mda_link_dump(link,do_res);

    }
}

// TODO manage properly event allocation and desallocation
void paris_traceroute_handler(pt_loop_t * loop, event_t * event, void * user_data)
{
    mda_event_t             * mda_event;
    paris_traceroute_data_t * data = user_data;

    switch (event->type) {
        case ALGORITHM_TERMINATED:
            // Dump full lattice, only when MDA_NEW_LINK is not handled
            if (strcmp(data->algorithm, "mda") != 0) {
                lattice_dump(event->data, (ELEMENT_DUMP) result_dump);
            }
            pt_loop_terminate(loop);
            break;
        case ALGORITHM_EVENT:
            if (strcmp(data->algorithm, "mda") == 0) {
                mda_event = event->data;
                switch (mda_event->type) {
                    case MDA_NEW_LINK:
                        mda_link_dump(mda_event->data, do_res);
                        break;
                    default:
                        break;
                }
            }

            break;
        default:
            break;
    }
}

int main(int argc, char ** argv)
{
    traceroute_options_t      traceroute_options;
    mda_options_t             mda_options;
    paris_traceroute_data_t * data;
    algorithm_instance_t    * instance;
    probe_t                 * probe_skel;
    pt_loop_t               * loop;
    int                       ret,i;
    char                    * prot;
    static sockaddr_any       dst_addr = {{ 0, }, };

    opt_options1st();
    if (opt_parse("usage: %s [options] host", cl_options, argv) != 1) {
        fprintf(stderr, "%s: destination required\n", basename(argv[0]));
        exit(EXIT_FAILURE);
    }
    
    // Retrieve the target IP address
    for(i = 0; argv[i] != NULL && i < sizeof(argv); ++i);
    address_set_host(argv[i - 1], &dst_addr);
    data = malloc(sizeof(paris_traceroute_data_t));
    if (!data) goto ERROR;
    data->dst_ip = address_2_str(&dst_addr);
    data->algorithm = algorithms[0];
    prot = protocols[0];   
    printf("Traceroute to %s using algorithm %s\n\n", data->dst_ip, data->algorithm);
    network_set_timeout(wait[0]);

    // Create libparistraceroute loop
    loop = pt_loop_create(paris_traceroute_handler, data);
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
    
    probe_set_protocols(
        probe_skel,
        is_ipv4 ? "ipv4" : "ipv6",
        is_udp  ? "udp" : prot,
        NULL
    );
    probe_set_payload_size(probe_skel, 32); // probe_set_size XXX

    // Set default values
    probe_set_fields(
        probe_skel,
        STR("dst_ip", data->dst_ip),
        I16("dst_port", dport[0]),
        I16("src_port", sport[0]),

        NULL
    );

    if (is_udp) {           
        probe_set_fields(probe_skel, I16("dst_port", 53), NULL);
    } 
 
    // Verify that the user pass option related to mda iif
    // this is the chosen algorithm .
    if (mda[6]) {
        if (data->algorithm) {
            if (strcmp(data->algorithm, "mda") != 0) {
                perror("E: You cannot pass options related to mda when using another algorithm ");
            }
        } else data->algorithm = "mda";
    }

    // Prepare options related to the chosen algorithm
    if (strcmp(data->algorithm, "traceroute") == 0 || strcmp(data->algorithm, "paris-traceroute") == 0) {
        traceroute_options = traceroute_get_default_options();

        // Overrides default parameter with user's ones 
        if (first_ttl[0] != traceroute_options.min_ttl) { 
            traceroute_options.min_ttl = first_ttl[0];
        }
         if (max_ttl[0] != traceroute_options.max_ttl) { 
            traceroute_options.max_ttl = max_ttl[0];
        }
        traceroute_options.dst_ip = address_2_str(&dst_addr);
        data->options = &traceroute_options;

    } else if (strcmp(data->algorithm, "mda") == 0 || mda[6]) {
        mda_options = mda_get_default_options(); 
        printf("min_ttl = %d max_ttl = %d num_probes = %d dst_ip = %s bound = %d max_branch = %d\n",
            mda_options.traceroute_options.min_ttl,
            mda_options.traceroute_options.max_ttl,
            mda_options.traceroute_options.num_probes,
            mda_options.traceroute_options.dst_ip ? mda_options.traceroute_options.dst_ip : "",
            mda_options.bound,
            mda_options.max_branch
        );

        if (first_ttl[0] != mda_options.traceroute_options.min_ttl) { 
            mda_options.traceroute_options.min_ttl = first_ttl[0];
        }
        if (max_ttl[0] != mda_options.traceroute_options.max_ttl) { 
            mda_options.traceroute_options.max_ttl = max_ttl[0];
        }
        traceroute_options.dst_ip = address_2_str(&dst_addr);
        if (mda[0] != mda_options.bound) { 
            mda_options.bound = mda[0];
        }
        if (mda[3] != mda_options.max_branch) { 
            mda_options.max_branch = mda[3];
        }
            /*printf("min_ttl = %d max_ttl = %d num_probes = %d dst_ip = %s bound = %d max_branch = %d\n",
            mda_options.traceroute_options.min_ttl,
            mda_options.traceroute_options.max_ttl,
            mda_options.traceroute_options.num_probes,
            mda_options.traceroute_options.dst_ip ? mda_options.traceroute_options.dst_ip : "",
            mda_options.bound,
            mda_options.max_branch
        );*/

        data->options = &mda_options;
    } else {
        perror("E: Unknown algorithm ");
        goto ERROR;   
    }
    

    instance = pt_algorithm_add(loop, data->algorithm, data->options, probe_skel);
    if (!instance) {
        perror("E: Cannot add the chosen algorithm");
        goto ERR_INSTANCE;
    }

    // Wait for events. They will be catched by handler_user() 
    ret = pt_loop(loop, 0);
    if (ret < 0) goto ERROR;

    // Free data and quit properly
    pt_algorithm_free(instance);

    exit(EXIT_SUCCESS);

ERR_INSTANCE:
ERR_PROBE_SKEL:
    //pt_loop_free(loop);
ERR_LOOP:
ERROR:
    exit(EXIT_FAILURE);
}
