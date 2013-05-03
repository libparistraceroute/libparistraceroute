#include <stdlib.h>                  // malloc...
#include <stdio.h>                   // perror, printf
#include <errno.h>                   // EINVAL, ENOMEM, errno
#include <libgen.h>                  // basename
#include <limits.h>                  // INT_MAX
#include <string.h>                  // strcmp
#include <sys/types.h>               // gai_strerror
#include <sys/socket.h>              // gai_strerror
#include <netdb.h>                   // gai_strerror

#include "optparse.h"                // opt_*()
#include "pt_loop.h"                 // pt_loop_t
#include "probe.h"                   // probe_t
#include "lattice.h"                 // lattice_t
#include "algorithm.h"               // algorithm_instance_t
#include "algorithms/mda.h"          // mda_*_t
#include "algorithms/traceroute.h"   // traceroute_options_t
#include "address.h"                 // address_to_string, address_set_host

//---------------------------------------------------------------------------
// Command line stuff
//---------------------------------------------------------------------------

const char * algorithm_names[] = {
    "mda",
    "traceroute",
    "paris-traceroute",
    NULL
};

// static variables, needed for command-line parsing
static unsigned is_ipv4 = 1;
static unsigned is_udp  = 0;
static unsigned do_resolv  = 1;

const char * protocol_names[] = {
    "udp",
    NULL
};

// Bounded integer parameters | def    min  max
static unsigned first_ttl[3] = {1,     1,   255};
static unsigned max_ttl[3]   = {30,    1,   255};
static double   wait[3]      = {5,     0,   INT_MAX};

// Bounded integer parameters | def    min  max    option_enabled
static unsigned dst_port[4]  = {30000, 0,   65535, 0};
static unsigned src_port[4]  = {3083,  0,   65535, 1};

// Bounded pairs parameters  |  def1 min1 max1 def2 min2 max2      mda_enabled
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
#define HELP_V "version 1.0"

struct opt_spec cl_options[] = {
    // action                  sf   lf                   metavar             help         data
    {opt_help,                 "h", "--help"   ,         OPT_NO_METAVAR,     OPT_NO_HELP, OPT_NO_DATA},
    {opt_version,              "V", "--version",         OPT_NO_METAVAR,     OPT_NO_HELP, HELP_V},
    {opt_store_choice,         "a", "--algo",            "ALGORITHM",        HELP_a,      algorithm_names},
    {opt_store_1,              "4", OPT_NO_LF,           OPT_NO_METAVAR,     HELP_4,      &is_ipv4},
    {opt_store_choice,         "P", "--protocol",        "protocol",         HELP_P,      protocol_names},
    {opt_store_1,              "U", "--UDP",             OPT_NO_METAVAR,     HELP_U,      &is_udp},
    {opt_store_int_lim,        "f", "--first",           "first_ttl",        HELP_f,      first_ttl},
    {opt_store_int_lim,        "m", "--max-hops",        "max_ttl",          HELP_m,      max_ttl},
    {opt_store_0,              "n", OPT_NO_LF,           OPT_NO_METAVAR,     HELP_n,      &do_resolv},
    {opt_store_double_lim,     "w", "--wait",            "waittime",         HELP_w,      wait},
    {opt_store_int_2,          "M", "--mda",             "bound,max_branch", HELP_M,      mda},
    {opt_store_int_lim_en,     "s", "--source_port",     "PORT",             HELP_s,      src_port},
    {opt_store_int_lim_en,     "d", "--dest_port",       "PORT",             HELP_d,      dst_port},
    {OPT_NO_ACTION},
};

//---------------------------------------------------------------------------
// Main program data structure
//---------------------------------------------------------------------------

// TODO remove this structure which seems to be useless
typedef struct {
    char * algorithm;
    char * dst_ip;
    void * options;
} paris_traceroute_data_t;

paris_traceroute_data_t * paris_traceroute_data_create(
    const char      * algorithm_name,
    const address_t * dst_addr
) {
    paris_traceroute_data_t * data;

    if (!(data = calloc(1, sizeof(paris_traceroute_data_t)))) {
        goto ERROR_DATA;
    }

    // Convert target IP from address to string
    if (address_to_string(dst_addr, &data->dst_ip) != 0) {
        goto ERR_ADDRESS_TO_STRING;
    }

    // Assign parameters in the algorithm data structure
    if (!(data->algorithm = strdup(algorithm_name))) {
        goto ERR_DATA_ALGORITHM;
    }

    return data;
ERR_DATA_ALGORITHM:
    free(data->dst_ip);
ERR_ADDRESS_TO_STRING:
    free(data);
ERROR_DATA:
    return NULL;
}

void paris_traceroute_data_free(paris_traceroute_data_t * data) {
    if (data) {
        if (data->algorithm) free(data->algorithm);
        if (data->dst_ip)    free(data->dst_ip);
        free(data);
    }
}

//---------------------------------------------------------------------------
// Main program 
//---------------------------------------------------------------------------

/**
 * \brief Handle events raised by libparistraceroute
 * \param loop The main loop
 * \param event The event raised by libparistraceroute
 * \param user_data Points to user data, shared by
 *   all the algorithms instances running in this loop.
 */

void user_handler(pt_loop_t * loop, event_t * event, void * user_data)
{
    mda_event_t             * mda_event;
    paris_traceroute_data_t * data = user_data;

    switch (event->type) {
        case ALGORITHM_TERMINATED:
            // Dump full lattice, only when MDA_NEW_LINK is not handled
            if (strcmp(data->algorithm, "mda") != 0) {
                mda_interface_dump(event->data, do_resolv);
            }
            pt_loop_terminate(loop);
            break;
        case ALGORITHM_EVENT:
            if (strcmp(data->algorithm, "mda") == 0) {
                mda_event = event->data;
                switch (mda_event->type) {
                    case MDA_NEW_LINK:
                        mda_link_dump(mda_event->data, do_resolv);
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
    traceroute_options_t    * ptraceroute_options;
    mda_options_t             mda_options;
    paris_traceroute_data_t * data;
    probe_t                 * probe_skel;
    pt_loop_t               * loop      = NULL;
    int                       exit_code = EXIT_FAILURE, i;
    address_t                 dst_addr;

    // Retrieve values passed in the command-line
    opt_options1st();
    if (opt_parse("usage: %s [options] host", cl_options, argv) != 1) {
        fprintf(stderr, "%s: destination required\n", basename(argv[0]));
        exit(EXIT_FAILURE);
    }

    // Verify that the user pass option related to mda iif this is the chosen algorithm.
    if (mda[6]) {
        if (strcmp(algorithm_names[0], "mda") != 0) {
            perror("E: You cannot pass options related to mda when using another algorithm ");
            goto ERR_INVALID_ALGORITHM;
        }
    }

    // Iterate on argv to retrieve the target IP address
    for(i = 0; argv[i] && i < argc; ++i);
    if (address_from_string(argv[i - 1], &dst_addr) != 0) {
        goto ERR_ADDRESS_FROM_STRING;
    }

    // Prepare data
    if (!(data = paris_traceroute_data_create(algorithm_names[0], &dst_addr))) {
        goto ERR_DATA;
    }
    printf("Traceroute to %s using algorithm %s\n\n", data->dst_ip, data->algorithm);

    // Set network timeout
    network_set_timeout(wait[0]);

    // Probe skeleton definition: IPv4/UDP probe targetting 'dst_ip'
    if (!(probe_skel = probe_create())) {
        perror("E: Cannot create probe skeleton");
        goto ERR_PROBE_SKEL;
    }
    
    probe_set_protocols(
        probe_skel,
        is_ipv4 ? "ipv4" : "ipv6",
        is_udp  ? "udp"  : protocol_names[0],
        NULL
    );
    probe_set_payload_size(probe_skel, 32); // probe_set_size XXX

    // Set default values
    probe_set_fields(
        probe_skel,
        STR("dst_ip",   data->dst_ip),
        I16("dst_port", dst_port[0]),
        I16("src_port", src_port[0]),
        NULL
    );

    // Option -U sets port to 53 (DNS)
    if (is_udp && !dst_port[3]) {
        probe_set_fields(probe_skel, I16("dst_port", 53), NULL);
    }
 
    // Dedicated options 
    if (strcmp(data->algorithm, "traceroute") == 0
    ||  strcmp(data->algorithm, "paris-traceroute") == 0) {
        traceroute_options = traceroute_get_default_options();
        ptraceroute_options = &traceroute_options;
        data->options = &traceroute_options;
    } else if (strcmp(data->algorithm, "mda") == 0 || mda[6]) {
        mda_options = mda_get_default_options(); 
        ptraceroute_options = &mda_options.traceroute_options;
        mda_options.bound = mda[0];
        mda_options.max_branch = mda[3];
        data->options = &mda_options;
    } else {
        perror("E: Unknown algorithm ");
        goto ERR_UNKNOWN_ALGORITHM;
    }

    // Common options
    if (ptraceroute_options) {
        ptraceroute_options->min_ttl = first_ttl[0];
        ptraceroute_options->max_ttl = max_ttl[0];
        ptraceroute_options->dst_ip  = data->dst_ip;
    }

    // Create libparistraceroute loop
    if (!(loop = pt_loop_create(user_handler, data))) {
        perror("E: Cannot create libparistraceroute loop");
        goto ERR_LOOP_CREATE;
    }

    // Add an algorithm instance in the main loop
    if (!pt_algorithm_add(loop, data->algorithm, data->options, probe_skel)) {
        perror("E: Cannot add the chosen algorithm");
        goto ERR_INSTANCE;
    }

    // Wait for events. They will be catched by handler_user()
    if (pt_loop(loop, 0) < 0) {
        perror("E: Main loop interrupted");
        goto ERR_PT_LOOP;
    }
    exit_code = EXIT_SUCCESS;

    // Leave the program
ERR_PT_LOOP:
ERR_INSTANCE:
ERR_UNKNOWN_ALGORITHM:
ERR_INVALID_ALGORITHM:
    probe_free(probe_skel);
ERR_PROBE_SKEL:
    // pt_loop_free() automatically removes algorithms instances,
    // probe_replies and events from the memory.
    // Options and probe_skel must be manually removed.
    pt_loop_free(loop);
ERR_LOOP_CREATE:
    paris_traceroute_data_free(data);
ERR_DATA:
ERR_ADDRESS_FROM_STRING:
    if (errno) perror("Error in transforming a string to an address");
    exit(exit_code);
}

