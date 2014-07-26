#include "config.h"

#include <stdlib.h>                  // malloc...
#include <stdio.h>                   // perror, printf
#include <stdbool.h>                 // bool
#include <errno.h>                   // errno
#include <libgen.h>                  // basename
#include <string.h>                  // strcmp
#include <stdint.h>                  // UINT16_MAX
#include <float.h>                   // DBL_MAX
#include <sys/types.h>               // gai_strerror
#include <sys/socket.h>              // gai_strerror, AF_INET, AF_INET6
#include <netdb.h>                   // gai_strerror

#include "common.h"                  // ELEMENT_DUMP
#include "optparse.h"                // opt_*()
#include "pt_loop.h"                 // pt_loop_t
#include "probe.h"                   // probe_t
#include "lattice.h"                 // lattice_t
#include "algorithms/mda.h"          // mda_*_t
#include "algorithm.h"               // algorithm_instance_t
#include "algorithms/ping.h"         // ping_options_t
#include "address.h"                 // address_to_string
#include "options.h"                 // options_*

//---------------------------------------------------------------------------
// Command line stuff
//---------------------------------------------------------------------------

#define PING_HELP_4        "Use IPv4."
#define PING_HELP_6        "Use IPv6."
#define PING_HELP_F        "Allocate and set 20 bit flow label on echo request packets. (Only IPv6)."
#define PING_HELP_i        "Wait 'interval' seconds between sending each packet."
#define PING_HELP_I        "Set source address to specified interface address."
#define PING_HELP_s        "Specifies the number of data bytes to be sent."
#define PING_HELP_W        "Time to wait for a response, in seconds."
#define PING_HELP_P        "Set PORT as destination port (default: 33457)."
#define PING_HELP_S        "Set PORT as source port (default: 33456)."
#define PING_HELP_T        "Use TCP. The destination port is set by default to 80."
#define PING_HELP_U        "Use UDP. The destination port is set by default to 53."
#define PING_HELP_k        "Send a TCP ACK packet. (Works only with TCP)"


#define TEXT          "ping - verify the connection between two hosts."
#define TEXT_OPTIONS  "Options:"

// Default values (based on modern traceroute for linux)

#define UDP_DEFAULT_SRC_PORT  33457
#define UDP_DEFAULT_DST_PORT  33456
#define UDP_DST_PORT_USING_U  53

#define TCP_DEFAULT_SRC_PORT  16449
#define TCP_DEFAULT_DST_PORT  16963
#define TCP_DST_PORT_USING_T  80

const char * algorithm_names[] = {
    "ping", // default value
    NULL
};

static bool is_ipv4    = false;
static bool is_ipv6    = false;

static bool is_tcp     = false;
static bool is_udp     = false;
static bool is_icmp    = false;
static bool is_tcp_ack = false;

// points to the source address (if indicated; option -I)
struct opt_str src_ip = {NULL, 0};

const char * protocol_names[] = {
    "icmp", // default value
    "tcp",
    "udp",
    NULL
};

// Bounded integer parameters
//                                  def     min  max                     option_enabled
static int      dst_port[4]      = {33457,  0,   UINT16_MAX,             0};
static int      src_port[4]      = {33456,  0,   UINT16_MAX,             0};
static int      flow_label[4]    = {0,      0,   PING_FLOW_LABEL_MAX,    0};
static double   send_time[3]     = {1,      1,   DBL_MAX};
static int      packet_size[3]   = OPTIONS_PING_PACKET_SIZE;
static unsigned max_ttl[3]       = OPTIONS_PING_MAX_TTL;

struct opt_spec runnable_options[] = {
    // action                 sf          lf                   metavar               help               data
    {opt_text,                OPT_NO_SF,  OPT_NO_LF,           OPT_NO_METAVAR,       TEXT,              OPT_NO_DATA},
    {opt_text,                OPT_NO_SF,  OPT_NO_LF,           OPT_NO_METAVAR,       TEXT_OPTIONS,      OPT_NO_DATA},
    {opt_store_1,             "4",        OPT_NO_LF,           OPT_NO_METAVAR,       PING_HELP_4,       &is_ipv4},
    {opt_store_1,             "6",        OPT_NO_LF,           OPT_NO_METAVAR,       PING_HELP_6,       &is_ipv6},
    {opt_store_int_lim_en,    "F",        OPT_NO_LF,           " FLOW_LABEL",        PING_HELP_F,       flow_label},
    {opt_store_str,           "I",        OPT_NO_LF,           " INTERFACE_ADDRESS", PING_HELP_I,       &src_ip},
    {opt_store_double_lim_en, "i",        OPT_NO_LF,           " INTERVAL",          PING_HELP_i,       send_time},
    {opt_store_int_lim,       "s",        OPT_NO_LF,           " PACKET_SIZE",       PING_HELP_s,       packet_size},
    {opt_store_int_lim_en,    "P",        "--dst-port",        " PORT",              PING_HELP_P,       dst_port},
    {opt_store_int_lim_en,    "S",        "--src-port",        " PORT",              PING_HELP_S,       src_port},
    {opt_store_1,             "T",        "--tcp",             OPT_NO_METAVAR,       PING_HELP_T,       &is_tcp},
    {opt_store_1,             "U",        "--udp",             OPT_NO_METAVAR,       PING_HELP_U,       &is_udp},
    {opt_store_1,             "k",        OPT_NO_LF,           OPT_NO_METAVAR,       PING_HELP_k,       &is_tcp_ack},
    {opt_store_int,           "t",        OPT_NO_LF,           " TIME TO LIVE",      PING_HELP_t,       max_ttl},

    END_OPT_SPECS
};

/**
 * \brief Prepare options supported by paris-traceroute
 * \return A pointer to the corresponding options_t instance if successfull, NULL otherwise
 */

static options_t * init_options(char * version) {
    options_t * options;

    // Building the command line options
    if (!(options = options_create(NULL))) {
        goto ERR_OPTIONS_CREATE;
    }

    options_add_optspecs(options, runnable_options);
    options_add_optspecs(options, ping_get_options());
    options_add_optspecs(options, network_get_options());
    options_add_common  (options, version);
    return options;

ERR_OPTIONS_CREATE:
    return NULL;
}

//---------------------------------------------------------------------------
// Options checking
//---------------------------------------------------------------------------

static bool check_ip_version(bool is_ipv4, bool is_ipv6)
{
    // The user may omit -4 and -6 but cannot set the both
    // options simultaneously.
    if (is_ipv4 && is_ipv6) {
        fprintf(stderr, "Cannot set both ip versions\n");
        return false;
    }

    return true;
}

static bool check_protocol(bool is_icmp, bool is_tcp, bool is_udp, const char * protocol_name)
{
    unsigned check = 0;

    if (is_icmp) check += 1;
    if (is_udp)  check += 1;
    if (is_tcp)  check += 1;

    if (check > 1) {
        fprintf(stderr, "E: Cannot use simultaneously icmp tcp and udp\n");
        return false;
    }

    return true;
}

static bool check_ports(bool is_icmp, int dst_port_enabled, int src_port_enabled)
{
    if (is_icmp && (dst_port_enabled || src_port_enabled)) {
        fprintf(stderr, "E: Cannot use --src-port or --dst-port when using icmp\n");
        return false;
    }

    return true;
}

static bool check_valid_flow_option(bool is_ipv6, bool set_flow_label)
{
    if (!is_ipv6 && set_flow_label) {
        fprintf(stderr, "E: Cannot set a flow label when using ipv4\n");
        return !set_flow_label;
    }
    return true;
}

static bool check_valid_ack_option(bool is_tcp, bool is_ack) {
    if (is_ack && !is_tcp) {
        fprintf(stderr, "E: Cannot send a TCP ACK packet when a protocol other than TCP is chosen\n");
        return false;
    }
    return true;
}

static bool check_options(
    bool         is_icmp,
    bool         is_tcp,
    bool         is_udp,
    bool         is_tcp_ack,
    bool         is_ipv4,
    bool         is_ipv6,
    bool         set_flow_label,
    int          dst_port_enabled,
    int          src_port_enabled,
    const char * protocol_name,
    const char * algorithm_name
) {
    return check_ip_version(is_ipv4, is_ipv6)
        && check_protocol(is_icmp, is_tcp, is_udp, protocol_name)
        && check_ports(is_icmp, dst_port_enabled, src_port_enabled)
        && check_valid_flow_option(is_ipv6, set_flow_label)
        && check_valid_ack_option(is_tcp, is_tcp_ack);
}

//---------------------------------------------------------------------------
// Command-line 

// libparistraceroute translation
//---------------------------------------------------------------------------

/**
 * \brief Handle events raised by libparistraceroute.
 * \param loop The main loop.
 * \param event The event raised by libparistraceroute.
 * \param user_data Points to user data, shared by
 *   all the algorithms instances running in this loop.
 */

void loop_handler(pt_loop_t * loop, event_t * event, void * user_data)
{
    ping_event_t         * ping_event;
    const ping_options_t * ping_options;
    ping_data_t          * ping_data;

    switch (event->type) {
        case ALGORITHM_HAS_TERMINATED:
            ping_data = event->issuer->data;

            if (ping_data != NULL) { // to prevent to print statistics twice
            ping_dump_statistics(ping_data);
            }

            pt_instance_stop(loop, event->issuer);
            pt_loop_terminate(loop);
            break;

        case ALGORITHM_EVENT:
            ping_event   = event->data;
            ping_options = event->issuer->options;
            ping_data    = event->issuer->data;

            // Forward this event to the default ping handler
            // See libparistraceroute/algorithms/ping.c
            ping_handler(loop, ping_event, ping_options, ping_data);
            break;

        default:
            break;
    }
    event_free(event);
}

const char * get_ip_protocol_name(int family) {
    switch (family) {
        case AF_INET:
            return "ipv4";
        case AF_INET6:
            return "ipv6";
        default:
            fprintf(stderr, "get_ip_protocol_name: Internet family not supported (%d)\n", family);
            break;
    }

    return NULL;
}

const char * get_protocol_name(int family, bool use_icmp, bool use_tcp, bool use_udp) {
    if (use_tcp) {
        return "tcp";
    } 
    else if (use_udp) {
        return "udp";
    }
    else if (use_icmp) {
        switch (family) {
            case AF_INET:
                return "icmpv4";
            case AF_INET6:
                return "icmpv6";
            default:
                fprintf(stderr, "Internet family not supported (%d)\n", family);
                break;
        }
    }

    return NULL;
}

//---------------------------------------------------------------------------
// Main program
//---------------------------------------------------------------------------

int main(int argc, char ** argv)
{
    int                       exit_code = EXIT_FAILURE;
    char                    * version = strdup("version 1.0");
    const char              * usage = "usage: %s [options] host\n";
    void                    * algorithm_options = NULL;
    ping_options_t            ping_options;
    probe_t                 * probe;
    pt_loop_t               * loop;
    int                       family;
    address_t                 dst_addr;
    address_t                 src_addr;
    options_t               * options;
    char                    * dst_ip;
    const char              * algorithm_name;
    const char              * protocol_name;
    bool                      use_icmp, use_udp, use_tcp;

    // Prepare the commande line options
    if (!(options = init_options(version))) {
        fprintf(stderr, "E: Can't initialize options\n");
        goto ERR_INIT_OPTIONS;
    }

    // Retrieve values passed in the command-line
    if (options_parse(options, usage, argv) != 1) {
        fprintf(stderr, "%s: destination required\n", basename(argv[0]));
        goto ERR_OPT_PARSE;
    }

    // We assume that the target IP address is always the last argument
    dst_ip         = argv[argc - 1];
    algorithm_name = algorithm_names[0];
    protocol_name  = protocol_names[0];

    // Checking if there is any conflicts between options passed in the commandline
    
    if (!check_options(is_icmp, is_tcp, is_udp, is_tcp_ack, is_ipv4, is_ipv6, flow_label[3], 
                       dst_port[3], src_port[3], protocol_name, algorithm_name)) {
        goto ERR_CHECK_OPTIONS;
    }

    use_icmp = is_icmp || strcmp(protocol_name, "icmp") == 0;
    use_tcp  = is_tcp  || strcmp(protocol_name, "tcp")  == 0;
    use_udp  = is_udp  || strcmp(protocol_name, "udp")  == 0;

    // If not any ip version is set, call address_guess_family.
    // If only one is set to true, set family to AF_INET or AF_INET6
    if (is_ipv4) {
        family = AF_INET;
    } else if (is_ipv6) {
        family = AF_INET6;
    } else {
        // Get address family if not defined by the user
        if (!address_guess_family(dst_ip, &family)) goto ERR_ADDRESS_GUESS_FAMILY;
    }
    
    // Translate the string IP / FQDN into an address_t * instance
    if (address_from_string(family, dst_ip, &dst_addr) != 0) {
        fprintf(stderr, "E: Invalid destination address %s\n", dst_ip);
        goto ERR_ADDRESS_IP_FROM_STRING;
    }

    // Probe skeleton definition: IPv4/UDP probe targetting 'dst_ip'
    if (!(probe = probe_create())) {
        fprintf(stderr,"E: Cannot create probe skeleton");
        goto ERR_PROBE_CREATE;
    }

    // Prepare the probe skeleton
    probe_set_protocols(
        probe,
        get_ip_protocol_name(family),                          // "ipv4"   | "ipv6"
        get_protocol_name(family, use_icmp, use_tcp, use_udp), // "icmpv4" | "icmpv6" | "tcp" | "udp"
        NULL
    );

    probe_set_field(probe, ADDRESS("dst_ip", &dst_addr));

    if (src_ip.s) {  // true if user has specified an interface address (-I)
        if (is_ipv4) {
           family = AF_INET;
        } else if (is_ipv6) {
           family = AF_INET6;
        } else {
        // Get address family if not defined by the user
           if (!address_guess_family(dst_ip, &family)) goto ERR_ADDRESS_GUESS_FAMILY;
        }
        if (address_from_string(family, src_ip.s, &src_addr) != 0) {
            fprintf(stderr, "E: Invalid source address %s\n", src_ip.s);
            goto ERR_ADDRESS_IP_FROM_STRING;
        } else {         
            probe_set_field(probe, ADDRESS("src_ip", &src_addr));
        }
    }

    probe_set_delay(probe, DOUBLE("delay", send_time[0]));

    probe_set_field(probe, I8("ttl", max_ttl[0]));

    // TODO fix BITS(x, y)
    /*
    if (flow_label[3]) {
        probe_set_field(probe, BITS("flow_label", 20, &(flow_label[0])));
    }
    */

     /*XXX UDP and TCP have to have a payload with size 2. However, this line of code shows that sth in the implementation
           has to change XXX*/
    probe_payload_resize(probe, packet_size[0] + 2 * (int)(is_tcp || is_udp));

    // ICMPv* do not support src_port and dst_port fields nor payload.
    if (use_tcp || use_udp) {
        uint16_t sport = 0,
                 dport = 0;

        if (use_udp) {
            // Option -U sets port to 53 (DNS) if dst_port is not explicitely set
            sport = src_port[3] ? src_port[0] : UDP_DEFAULT_SRC_PORT;
            dport = dst_port[3] ? dst_port[0] : (is_udp ? UDP_DST_PORT_USING_U : UDP_DEFAULT_DST_PORT);
        } else if (use_tcp) {
            // Option -T sets port to 80 (http) if dst_port is not explicitely set
            sport = src_port[3] ? src_port[0] : TCP_DEFAULT_SRC_PORT;
            dport = dst_port[3] ? dst_port[0] : (is_tcp ? TCP_DST_PORT_USING_T : TCP_DEFAULT_DST_PORT);
        }

        // Update ports
        probe_set_fields(
            probe,
            I16("src_port", sport),
            I16("dst_port", dport),
            NULL
        );
    }

    if (is_tcp) {
        int bit_value = 1;

        if (is_tcp_ack) {
            probe_set_field(probe, BITS("ack", 1, &bit_value));
        }
        else  {
            probe_set_field(probe, BITS("syn", 1, &bit_value));
        }
    }

    ping_options = ping_get_default_options();
    algorithm_options = &ping_options;

    // Algorithm options (common options)
    options_ping_init(&ping_options, &dst_addr, send_time[0], max_ttl[0]);

    // Create libparistraceroute loop
    if (!(loop = pt_loop_create(loop_handler, NULL))) {
        fprintf(stderr, "E: Cannot create libparistraceroute loop");
        goto ERR_LOOP_CREATE;
    }

    // Set network options (network and verbose)
    options_network_init(loop->network, false);

    printf("ping to %s (", dst_ip);
    address_dump(&dst_addr);
    printf(")\n");

    // Add an algorithm instance in the main loop
    if (!pt_algorithm_add(loop, algorithm_name, algorithm_options, probe)) {
        fprintf(stderr, "E: Cannot add the chosen algorithm");
        goto ERR_INSTANCE;
    }

    // Wait for events. They will be catched by handler_user()
    if (pt_loop(loop, 0) < 0) {
        fprintf(stderr, "E: Main loop interrupted");
        goto ERR_PT_LOOP;
    }

    exit_code = EXIT_SUCCESS;


    // Leave the program
ERR_PT_LOOP:
ERR_INSTANCE:
    // pt_loop_free() automatically removes algorithms instances,
    // probe_replies and events from the memory.
    // Options and probe must be manually removed.
    pt_loop_free(loop);
ERR_LOOP_CREATE:
    probe_free(probe);
ERR_PROBE_CREATE:
ERR_ADDRESS_IP_FROM_STRING:
ERR_ADDRESS_GUESS_FAMILY:
    if (errno) perror(gai_strerror(errno));
ERR_CHECK_OPTIONS:
ERR_OPT_PARSE:
ERR_INIT_OPTIONS:
    free(version);
    exit(exit_code);
}

