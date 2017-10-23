#include "config.h"

#include <stdlib.h>                  // malloc...
#include <stdio.h>                   // perror, printf
#include <stdbool.h>                 // bool
#include <stddef.h>                  // size_t
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
#include "algorithm.h"               // algorithm_instance_t
#include "algorithms/mda.h"          // mda_*_t
#include "algorithms/traceroute.h"   // traceroute_options_t
#include "address.h"                 // address_to_string
#include "options.h"                 // options_*
#include "containers/map.h"          // map_t
#include "containers/vector.h"       // vector_t
#include "int.h"

//---------------------------------------------------------------------------
// Command line stuff
//---------------------------------------------------------------------------

#define TRACEROUTE_HELP_4  "Use IPv4."
#define TRACEROUTE_HELP_6  "Use IPv6."
#define TRACEROUTE_HELP_a  "Set the traceroute algorithm (default: 'paris-traceroute'). Valid values are 'paris-traceroute' and 'mda'."
#define TRACEROUTE_HELP_F  "Set the output format of paris-traceroute (default: 'default')"
#define TRACEROUTE_HELP_d  "Print libparistraceroute debug information."
#define TRACEROUTE_HELP_p  "Set PORT as destination port (default: 33457)."
#define TRACEROUTE_HELP_s  "Set PORT as source port (default: 33456)."
#define TRACEROUTE_HELP_I  "Use ICMPv4/ICMPv6 for tracerouting."
#define TRACEROUTE_HELP_P  "Use raw packet of protocol PROTOCOL for tracerouting (default: 'udp'). Valid values are 'udp' and 'icmp'."
#define TRACEROUTE_HELP_T  "Use TCP for tracerouting."
#define TRACEROUTE_HELP_U  "Use UDP for tracerouting. The destination port is set by default to 53."
#define TRACEROUTE_HELP_z  "Minimal time interval between probes (default 0).  If the value is more than 10, then it specifies a number in milliseconds, else it is a number of seconds (float point values allowed  too)"
#define TEXT               "paris-traceroute - print the IP-level path toward a given IP host."
#define TEXT_OPTIONS       "Options:"

// Default values (based on modern traceroute for linux)

#define UDP_DEFAULT_SRC_PORT  33457
#define UDP_DEFAULT_DST_PORT  33456
#define UDP_DST_PORT_USING_U  53

#define TCP_DEFAULT_SRC_PORT  16449
#define TCP_DEFAULT_DST_PORT  16963
#define TCP_DST_PORT_USING_T  80

const char * algorithm_names[] = {
    "paris-traceroute", // default value
    "mda",
    NULL
};

typedef enum format_t {
    FORMAT_DEFAULT,
    FORMAT_JSON,
    FORMAT_XML
} format_t;

const char * format_names[] = {
    "default", // default value
    "json",
    "xml"
};

static bool is_ipv4  = false;
static bool is_ipv6  = false;
static bool is_tcp   = false;
static bool is_udp   = false;
static bool is_icmp  = false;
static bool is_debug = false;

const char * protocol_names[] = {
    "udp", // default value
    "icmp",
    "tcp",
    NULL
};

// Bounded integer parameters
//                              def     min  max         option_enabled
static int    dst_port[4]    = {33457,  0,   UINT16_MAX, 0};
static int    src_port[4]    = {33456,  0,   UINT16_MAX, 0};
static double send_time[4]   = {1,      1,   DBL_MAX,    0};

struct opt_spec runnable_options[] = {
    // action                 sf          lf                   metavar             help          data
    {opt_text,                OPT_NO_SF,  OPT_NO_LF,           OPT_NO_METAVAR,     TEXT,         OPT_NO_DATA},
    {opt_text,                OPT_NO_SF,  OPT_NO_LF,           OPT_NO_METAVAR,     TEXT_OPTIONS, OPT_NO_DATA},
    {opt_store_1,             "4",        OPT_NO_LF,           OPT_NO_METAVAR,     TRACEROUTE_HELP_4,       &is_ipv4},
    {opt_store_1,             "6",        OPT_NO_LF,           OPT_NO_METAVAR,     TRACEROUTE_HELP_6,       &is_ipv6},
    {opt_store_choice,        "a",        "--algorithm",       "ALGORITHM",        TRACEROUTE_HELP_a,       algorithm_names},
    {opt_store_1,             "d",        "--debug",           OPT_NO_METAVAR,     TRACEROUTE_HELP_d,       &is_debug},
    {opt_store_choice,        "F",        "--format",          "FORMAT",           TRACEROUTE_HELP_F,       format_names},
    {opt_store_int_lim_en,    "p",        "--dst-port",        "PORT",             TRACEROUTE_HELP_p,       dst_port},
    {opt_store_int_lim_en,    "s",        "--src-port",        "PORT",             TRACEROUTE_HELP_s,       src_port},
    {opt_store_double_lim_en, "z",        OPT_NO_LF,           "WAIT",             TRACEROUTE_HELP_z,       send_time},
    {opt_store_1,             "I",        "--icmp",            OPT_NO_METAVAR,     TRACEROUTE_HELP_I,       &is_icmp},
    {opt_store_choice,        "P",        "--protocol",        "PROTOCOL",         TRACEROUTE_HELP_P,       protocol_names},
    {opt_store_1,             "T",        "--tcp",             OPT_NO_METAVAR,     TRACEROUTE_HELP_T,       &is_tcp},
    {opt_store_1,             "U",        "--udp",             OPT_NO_METAVAR,     TRACEROUTE_HELP_U,       &is_udp},
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
    options_add_optspecs(options, traceroute_get_options());
    options_add_optspecs(options, mda_get_options());
    options_add_optspecs(options, network_get_options());
    options_add_common  (options, version);
    return options;

ERR_OPTIONS_CREATE:
    return NULL;
}

//---------------------------------------------------------------------------
// Options checking
//---------------------------------------------------------------------------

static bool check_ip_version(bool is_ipv4, bool is_ipv6) {
    // The user may omit -4 and -6 but cannot set the both
    // options simultaneously.
    if (is_ipv4 && is_ipv6) {
        fprintf(stderr, "Cannot set both ip versions\n");
        return false;
    }

    return true;
}

static bool check_protocol(bool is_icmp, bool is_tcp, bool is_udp, const char * protocol_name) {
    unsigned check = 0;

    if (is_icmp) check += 1;
    if (is_udp)  check += 1;
    if (is_tcp)  check += 1;

    if (check > 1) {
        fprintf(stderr, "E: Cannot use simultaneously icmp tcp and udp tracerouting\n");
        return false;
    }

    return true;
}

static bool check_ports(bool is_icmp, int dst_port_enabled, int src_port_enabled) {
    if (is_icmp && (dst_port_enabled || src_port_enabled)) {
        fprintf(stderr, "E: Cannot use --src-port or --dst-port when using icmp tracerouting\n");
        return false;
    }

    return true;
}

static bool check_algorithm(const char * algorithm_name) {
    if (options_mda_get_is_set()) {
        if (strcmp(algorithm_name, "mda") != 0) {
            fprintf(stderr, "You cannot pass options related to mda when using another algorithm\n");
            return false;
        }
    }
    return true;
}

static bool check_options(
    bool         is_icmp,
    bool         is_tcp,
    bool         is_udp,
    bool         is_ipv4,
    bool         is_ipv6,
    int          dst_port_enabled,
    int          src_port_enabled,
    const char * protocol_name,
    const char * algorithm_name
) {
    return check_ip_version(is_ipv4, is_ipv6)
        && check_protocol(is_icmp, is_tcp, is_udp, protocol_name)
        && check_ports(is_icmp, dst_port_enabled, src_port_enabled)
        && check_algorithm(algorithm_name);
}

static inline double delay_probe_reply(const probe_t *probe, const probe_t * reply) {
    double send_time = probe_get_sending_time(probe),
           recv_time = probe_get_recv_time(reply);
    return 1000 * (recv_time - send_time);
}

typedef struct {
    probe_t * reply;
    double    delay;
} enriched_reply_t;

enriched_reply_t * enriched_reply_shallow_copy(const enriched_reply_t * reply) {
    enriched_reply_t * reply_dup = malloc(sizeof(enriched_reply_t));
    reply_dup->reply = reply->reply;
    reply_dup->delay = reply->delay;
    return reply_dup;
}

void vector_enriched_reply_free(vector_t * vector) {
    for (int i = 0; i < vector->num_cells; ++i) {
        free(vector_get_ith_element(vector, i));
    }
    free(vector);
}

void map_probe_free(map_t * map){
    
    for (size_t i = 1; i < options_traceroute_get_max_ttl(); ++i) {
        vector_t * replies_by_hop_i = NULL;
        if (map_find(map, &i, &replies_by_hop_i)) {
            //Only free the vector and not the probes
            free(replies_by_hop_i);
        }
    }
}

/**
 * @brief Structure used to pass some user data to loop handler.
 */

typedef struct {
    format_t   format;
    map_t    * replies_by_hop;
    map_t    * stars_by_hop;
} user_data_t;

/**
 * Json handler to ouput the traceroute output in a json format according to RIPE format.
 */

void traceroute_json_handler(
    pt_loop_t                  * loop,
    mda_event_t                * mda_event,
    const traceroute_options_t * traceroute_options,
    user_data_t                * user_data
) {
    probe_t * probe;
    probe_t * reply;

    map_t   * current_replies_by_hop = user_data->replies_by_hop;
    map_t   * current_stars_by_hop   = user_data->stars_by_hop;
    
    uint8_t * ttl_probe = malloc(sizeof(uint8_t));
            * ttl_probe = 0;
    switch (mda_event->type) {
        case MDA_PROBE_REPLY:

            // Retrieve the probe and its corresponding reply
            probe = ((const probe_reply_t *) mda_event->data)->probe;
            reply = ((const probe_reply_t *) mda_event->data)->reply;

            //Managed by the container that uses it.
            enriched_reply_t * enriched_reply = malloc(sizeof(enriched_reply_t));
            enriched_reply->reply = reply;
            enriched_reply->delay = delay_probe_reply(probe, reply);

            if (probe_extract(probe, "ttl", ttl_probe)) {
                vector_t * replies_ttl;

                // Check if we already have this ttl in the map.
                if (map_find(current_replies_by_hop, ttl_probe, &replies_ttl)) {
                    //We already have the element, just push this new reply related to this probe.
                    vector_push_element(replies_ttl, enriched_reply);
                } else {
                    replies_ttl = vector_create(sizeof(enriched_reply_t), enriched_reply_shallow_copy, free, NULL);
                    vector_push_element(replies_ttl, enriched_reply);
                    //map_update duplicate the value and the key passed in arg, so free the vector after the call to map_update
                    map_update(current_replies_by_hop, ttl_probe, replies_ttl);
                    free(replies_ttl);
                }
                free(enriched_reply);
            }
            break;
        case MDA_PROBE_TIMEOUT:
            probe = (probe_t *) mda_event->data;
            if(probe_extract(probe,"ttl", ttl_probe)){
                vector_t * stars_ttl;
                // Check if we already have this ttl in the map.
                if (map_find(current_stars_by_hop, ttl_probe, &stars_ttl)) {
                    //We already have the element, just push this new reply related to this probe.
                    vector_push_element(stars_ttl, probe);
                } else {
                    stars_ttl = vector_create(sizeof(probe_t), probe_dup, probe_free, NULL);
                    vector_push_element(stars_ttl, probe);
                    //map_update duplicate the value and the key passed in arg, so free the vector after the call to map_update
                    map_update(current_stars_by_hop, ttl_probe, stars_ttl);
                    free(stars_ttl);
                }
            }
            break;
        default:
            break;
    }
    free(ttl_probe);
}



/**
 * Print the output of the mda to json.
 * @param out The output file descriptor (e.g. stdout)
 * @param replies_by_hop A map which associates for each TTL the corresponding replies.
 */

void reply_to_json(const map_t * replies_by_hop, FILE * f_json) {

    fprintf(f_json, "\"results\":[");
    for (size_t i = 1; i < options_traceroute_get_max_ttl(); ++i) {
        vector_t * replies_by_hop_i = NULL;

        if (map_find(replies_by_hop, &i, &replies_by_hop_i)) {
            fprintf(f_json, "{\"hop\":");
            fprintf(f_json, "%zu,", i);
            fprintf(f_json, "\"result\":[");
            for (size_t j = 0; j < replies_by_hop_i->num_cells; ++j) {

                fprintf(f_json, "{");
                enriched_reply_t *enriched_reply = vector_get_ith_element(replies_by_hop_i, j);
                probe_t *reply = enriched_reply->reply;

                uint16_t src_port = 0;
                uint16_t dst_port = 0;
                address_t reply_from_address;
                uint16_t flow_id = 0;
                uint8_t ttl_reply = 0;

                probe_extract(reply, "src_ip", &reply_from_address);
                probe_extract(reply, "src_port", &src_port);
                probe_extract(reply, "dst_port", &dst_port);
                probe_extract(reply, "flow_id", &flow_id);
                probe_extract(reply, "ttl", &ttl_reply);
                char *saddress[16];
                address_to_string(&reply_from_address, saddress);

                fprintf(f_json, "\"from\":\"%s\",", saddress[0]);
                fprintf(f_json, "\"src_port\":%-10hu,", src_port);
                fprintf(f_json, "\"dst_port\":%-10hu,", dst_port);
                fprintf(f_json, "\"flow_id\":%-10hu,", flow_id);
                fprintf(f_json, "\"ttl\":%-10hhu,", ttl_reply);
                fprintf(f_json, "\"rtt\":%5.3lf", enriched_reply->delay);
                fprintf(f_json, "}");

                // Check if its the last response for this hop.
                if (j != replies_by_hop_i->num_cells - 1) {
                    fprintf(f_json, ",");
                }
            }
            fprintf(f_json, "]}");
            if (i < map_size(replies_by_hop)) {
                fprintf(f_json, ",");
            } else {
                //Ensure that the result is flushed, even in debug mode.
                fprintf(f_json,"]\n");      
                break;  
            } 
        }
    }
}

void stars_to_json(const map_t * stars_by_hop, FILE * f_json) {

    fprintf(f_json, "\"stars\":[");
    for (size_t i = 1; i < options_traceroute_get_max_ttl(); ++i) {
        vector_t * starts_by_hop_i = NULL;

        if (map_find(stars_by_hop, &i, &starts_by_hop_i)) {
            fprintf(f_json, "{\"hop\":");
            fprintf(f_json, "%zu,", i);
            fprintf(f_json, "\"result\":[");
            for (size_t j = 0; j < starts_by_hop_i->num_cells; ++j) {

                fprintf(f_json, "{");
                probe_t * star = vector_get_ith_element(starts_by_hop_i, j);

                uint16_t src_port = 0;
                uint16_t dst_port = 0;
                address_t reply_from_address;
                uint16_t flow_id = 0;

                probe_extract(star, "src_ip", &reply_from_address);
                probe_extract(star, "src_port", &src_port);
                probe_extract(star, "dst_port", &dst_port);
                probe_extract(star, "flow_id", &flow_id);
                


                fprintf(f_json, "\"src_port\":%-10hu,", src_port);
                fprintf(f_json, "\"dst_port\":%-10hu,", dst_port);
                fprintf(f_json, "\"flow_id\":%-10hu,", flow_id);
                fprintf(f_json, "}");

                // Check if its the last response for this hop.
                if (j != starts_by_hop_i->num_cells - 1) {
                    fprintf(f_json, ",");
                }
            }
            fprintf(f_json, "]}");
            if (i < map_size(stars_by_hop)) {
                fprintf(f_json, ",");
            } else {
                //Ensure that the result is flushed, even in debug mode.
                fprintf(f_json,"]\n");      
                break;  
            } 
        }
    }
}

void reply_to_json_dump(const map_t * replies_by_hop) {
    reply_to_json(replies_by_hop, stdout);
    fflush(stdout);
}

void starts_to_json_dump(const map_t * stars_by_hop){
    stars_to_json(stars_by_hop, stdout);
    fflush(stdout);
}

//---------------------------------------------------------------------------
// Command-line / libparistraceroute translation
//---------------------------------------------------------------------------

/**
 * \brief Handle events raised by libparistraceroute.
 * \param loop The main loop.
 * \param event The event raised by libparistraceroute.
 * \param _user_data Points to a user_data_y, shared by
 *   all the algorithms instances running in this loop.
 */

void loop_handler(pt_loop_t * loop, event_t * event, void * _user_data) {
    traceroute_event_t          * traceroute_event;
    const traceroute_options_t  * traceroute_options;
    const traceroute_data_t     * traceroute_data;
    mda_event_t                 * mda_event;
    mda_data_t                  * mda_data;
    const char                  * algorithm_name;

    user_data_t                 * user_data = (user_data_t *) _user_data;
    switch (event->type) {
        case ALGORITHM_HAS_TERMINATED:
            algorithm_name = event->issuer->algorithm->name;
            if (strcmp(algorithm_name, "mda") == 0) {
                mda_data = event->issuer->data;

                switch (user_data->format) {
                    case FORMAT_DEFAULT:
                        printf("Lattice:\n");
                        lattice_dump(mda_data->lattice, (ELEMENT_DUMP) mda_lattice_elt_dump);
                        printf("\n");
                        break;
                    case FORMAT_XML:
                        fprintf(stderr, "loop_handler: Format XML not yet implemented\n");
                        break;
                    case FORMAT_JSON:
                        printf("{");
                        reply_to_json_dump(user_data->replies_by_hop);
                        
                        if(map_size(user_data->stars_by_hop) > 0){
                            printf(",");
                            starts_to_json_dump(user_data->stars_by_hop);
                        }
                        printf("}\n");
                        break;
                }

                mda_data_free(mda_data);
            }

            // Tell to the algorithm it can free its data
            pt_stop_instance(loop, event->issuer);

            // Remove the application from the loop.
            pt_del_instance(loop, event->issuer);

            // Kill the loop
            pt_loop_terminate(loop);
            break;
        case ALGORITHM_EVENT:
            algorithm_name = event->issuer->algorithm->name;
            if (strcmp(algorithm_name, "mda") == 0) {
                mda_event = event->data;
                traceroute_options = event->issuer->options; // mda_options inherits traceroute_options
                switch (mda_event->type) {
                    case MDA_NEW_LINK:
                        if (user_data->format == FORMAT_DEFAULT) {
                            mda_link_dump(mda_event->data, traceroute_options->do_resolv);
                        }
                        break;
                    case MDA_PROBE_REPLY:
                        if (user_data->format == FORMAT_JSON) {
                            traceroute_json_handler(loop, mda_event, traceroute_options, user_data);
                        }
                        break;
                    case MDA_PROBE_TIMEOUT:
                        if (user_data->format == FORMAT_JSON) {
                            traceroute_json_handler(loop, mda_event, traceroute_options, user_data);
                        }
                        break;
                    default:
                        break;
                }
            } else if (strcmp(algorithm_name, "traceroute") == 0) {
                traceroute_event   = event->data;
                traceroute_options = event->issuer->options;
                traceroute_data    = event->issuer->data;
                traceroute_handler(loop, traceroute_event, traceroute_options, traceroute_data);
            }
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
    if (use_icmp) {
        switch (family) {
            case AF_INET:
                return "icmpv4";
            case AF_INET6:
                return "icmpv6";
            default:
                fprintf(stderr, "Internet family not supported (%d)\n", family);
                break;
        }
    } else if (use_tcp) {
        return "tcp";
    } else if (use_udp) {
        return "udp";
    }

    return NULL;
}


//---------------------------------------------------------------------------
// Main program
//---------------------------------------------------------------------------

int main(int argc, char **argv) {
    int                       exit_code = EXIT_FAILURE;
    char                    * version = strdup("version 1.0");
    const char              * usage = "usage: %s [options] host\n";
    void                    * algorithm_options;
    traceroute_options_t      traceroute_options;
    traceroute_options_t    * ptraceroute_options;
    mda_options_t             mda_options;
    probe_t                 * probe;
    pt_loop_t               * loop;
    int                       family;
    address_t                 dst_addr;
    options_t               * options;
    char                    * dst_ip;
    const char              * algorithm_name;
    const char              * protocol_name;
    const char              * format_name;
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
    format_name    = format_names[0];

    // Checking if there is any conflicts between options passed in the commandline
    if (!check_options(is_icmp, is_tcp, is_udp, is_ipv4, is_ipv6, dst_port[3], src_port[3], protocol_name, algorithm_name)) {
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
        fprintf(stderr, "E: Cannot create probe skeleton");
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

    if (send_time[3]) {
        if (send_time[0] <= 10) { // seconds
            probe_set_delay(probe, DOUBLE("delay", send_time[0]));
        } else { // milli-seconds
            probe_set_delay(probe, DOUBLE("delay", 0.001 * send_time[0]));
        }
    }

    // ICMPv* do not support src_port and dst_port fields nor payload.
    if (!use_icmp) {
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

        // Resize payload (it will be use to set our customized checksum in the {TCP, UDP} layer)
        probe_payload_resize(probe, 2);
    }

    // Algorithm options (dedicated options)
    if (strcmp(algorithm_name, "paris-traceroute") == 0) {
        traceroute_options  = traceroute_get_default_options();
        ptraceroute_options = &traceroute_options;
        algorithm_options   = &traceroute_options;
        algorithm_name      = "traceroute";
    } else if ((strcmp(algorithm_name, "mda") == 0) || options_mda_get_is_set()) {
        mda_options         = mda_get_default_options();
        ptraceroute_options = &mda_options.traceroute_options;
        algorithm_options   = &mda_options;
        options_mda_init(&mda_options);
    } else {
        fprintf(stderr, "E: Unknown algorithm");
        goto ERR_UNKNOWN_ALGORITHM;
    }

    // Algorithm options (common options)
    options_traceroute_init(ptraceroute_options, &dst_addr);


    // Create user_data struct
    user_data_t user_data;
    user_data.format = !strcmp("json", format_name) ? FORMAT_JSON :
                       !strcmp("xml", format_name)  ? FORMAT_XML :
                       FORMAT_DEFAULT;
    user_data.replies_by_hop = NULL;

    //Create a dummy object with no elements, just put the callbacks.
    if (user_data.format == FORMAT_JSON) {
        user_data.replies_by_hop = map_create(uint8_dup, free, NULL, uint8_compare, vector_dup, vector_enriched_reply_free, NULL);
        user_data.stars_by_hop = map_create(uint8_dup, free, NULL, uint8_compare, vector_dup, free, NULL);
    }

    // Create libparistraceroute loop
    if (!(loop = pt_loop_create(loop_handler, &user_data))) {
        fprintf(stderr, "E: Cannot create libparistraceroute loop");
        goto ERR_LOOP_CREATE;
    }

    // Set network options (network and verbose)
    options_network_init(loop->network, is_debug);

    printf("%s to %s (", algorithm_name, dst_ip);
    address_dump(&dst_addr);
    printf("), %u hops max, %u bytes packets\n",
           ptraceroute_options->max_ttl,
           (unsigned int)packet_get_size(probe->packet)
          );

    // Add an algorithm instance in the main loop
    if (!pt_add_instance(loop, algorithm_name, algorithm_options, probe)) {
        fprintf(stderr, "E: Cannot add the chosen algorithm");
        goto ERR_INSTANCE;
    }

    // Wait for events. They will be catched by handler_user()
    if (pt_loop(loop, 0) < 0) {
        fprintf(stderr, "E: Main loop interrupted");
        goto ERR_PT_LOOP;
    }
    exit_code = EXIT_SUCCESS;

    // Free the allocated memory used for output
    if (user_data.format == FORMAT_JSON) {
        //Leak here because of double free on probes...
        //map_free(user_data.replies_by_hop);
        map_probe_free(user_data.replies_by_hop);
        map_probe_free(user_data.stars_by_hop);
    }

    // Leave the program
ERR_PT_LOOP:
ERR_INSTANCE:
    // pt_loop_free() automatically removes algorithms instances,
    // probe_replies and events from the memory.
    // Options and probe must be manually removed.
    pt_loop_free(loop);
ERR_LOOP_CREATE:
ERR_UNKNOWN_ALGORITHM:
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

