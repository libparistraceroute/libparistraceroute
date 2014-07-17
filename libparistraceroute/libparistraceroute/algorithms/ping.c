#include "ping.h"

#include <errno.h>            // errno, EINVAL
#include <stdlib.h>           // malloc
#include <stdio.h>            // fprintf
#include <string.h>           // memset()
#include <netinet/ip_icmp.h>  // icmpv4 constants
#include <netinet/icmp6.h>    // icmpv6 constantsak

#include "../probe.h"
#include "../event.h"
#include "../algorithm.h"
#include "../address.h"       // address_resolv
#include "../common.h"        // get_timestamp
#include "../network.h"       // options_network_get_timeout

//-----------------------------------------------------------------
// Ping options
//-----------------------------------------------------------------

// Bounded integer parameters
static bool              do_resolv           = OPTIONS_PING_DO_RESOLV_DEFAULT;
static bool              show_timestamp      = OPTIONS_PING_SHOW_TIMESTAMP_DEFAULT;
static bool              is_quiet            = OPTIONS_PING_IS_QUIET_DEFAULT;
static unsigned int      count[3]            = OPTIONS_PING_COUNT;

static option_t ping_options[] = {
    // action              short       long                 metavar             help          data
    {opt_store_int,        "c",        OPT_NO_LF,           " COUNT",           HELP_c,       &count},
    {opt_store_1,          "D",        OPT_NO_LF,           OPT_NO_METAVAR,     HELP_D,       &show_timestamp},
    {opt_store_0,          "n",        OPT_NO_LF,           OPT_NO_METAVAR,     HELP_n,       &do_resolv},
    {opt_store_1,          "q",        OPT_NO_LF,           OPT_NO_METAVAR,     HELP_q_ping,  &is_quiet},
    {opt_help,             "v",        OPT_NO_LF,           OPT_NO_METAVAR,     OPT_NO_HELP,  OPT_NO_DATA},
    END_OPT_SPECS
};

unsigned int options_ping_get_count() {
    return count[0];
}

bool options_ping_get_show_timestamp() {
    return show_timestamp;
}

bool options_ping_get_is_quiet() {
    return is_quiet;
}

bool options_ping_get_do_resolv() {
    return do_resolv;
}

const option_t * ping_get_options() {
    return ping_options;
}

void options_ping_init(ping_options_t * ping_options, address_t * address, double interval)
{
    ping_options->count            = options_ping_get_count();
    ping_options->dst_addr         = address;
    ping_options->interval         = interval;
    ping_options->show_timestamp   = options_ping_get_show_timestamp();
    ping_options->is_quiet         = options_ping_get_is_quiet();
    ping_options->do_resolv        = options_ping_get_do_resolv();
}

inline ping_options_t ping_get_default_options() {
    ping_options_t ping_options = {
        .dst_addr         = NULL,
        .do_resolv        = OPTIONS_PING_DO_RESOLV_DEFAULT,
        .interval         = OPTIONS_PING_INTERVAL_DEFAULT,
        .count            = OPTIONS_PING_COUNT_DEFAULT,
        .show_timestamp   = OPTIONS_PING_SHOW_TIMESTAMP_DEFAULT,
        .is_quiet         = OPTIONS_PING_IS_QUIET_DEFAULT,
    };
    return ping_options;
};

//-------------------------------------------------------------
// ICMP error analysing NOTE: these functions probably have to be put in another file
//-------------------------------------------------------------

static bool destination_network_unreachable(const probe_t *reply) {
    uint8_t version = 0, code = 0, type = 0;
    probe_extract(reply, "version", &version);
    probe_extract(reply, "code", &code);
    probe_extract(reply, "type", &type);

    if (version == 4) {
        return (type == ICMP_UNREACH) && (code == ICMP_UNREACH_HOST);
    }
    else {
        return (type == ICMP6_DST_UNREACH) && (code == ICMP6_DST_UNREACH_ADDR);
    }
}

static bool destination_host_unreachable(const probe_t *reply) {
    uint8_t version = 0, code = 0, type = 0;
    probe_extract(reply, "version", &version);
    probe_extract(reply, "code", &code);
    probe_extract(reply, "type", &type);

    if (version == 4) {
        return (type == ICMP_UNREACH) && (code == ICMP_UNREACH_NET);
    }
    else {
        return (type == ICMP6_DST_UNREACH) && (code == ICMP6_DST_UNREACH_NOROUTE);
    }
}

static bool destination_port_unreachable(const probe_t *reply) {
    uint8_t version = 0, code = 0, type = 0;
    probe_extract(reply, "version", &version);
    probe_extract(reply, "code", &code);
    probe_extract(reply, "type", &type);

    if (version == 4) {
        return (type == ICMP_UNREACH) && (code == ICMP_UNREACH_PORT);
    }
    else {
        return (type == ICMP6_DST_UNREACH) && (code == ICMP6_DST_UNREACH_NOPORT);
    }
}

static bool destination_protocol_unreachable(const probe_t *reply) {
    uint8_t version = 0, code = 0, type = 0;
    probe_extract(reply, "version", &version);
    probe_extract(reply, "code", &code);
    probe_extract(reply, "type", &type);

    if (version == 4) {
        return (type == ICMP_UNREACH) && (code == ICMP_UNREACH_PROTOCOL);
    }
    else {
        return (type == ICMP6_PARAM_PROB) && (code == ICMP6_PARAMPROB_NEXTHEADER);
    }
}

static bool ttl_exceeded(const probe_t *reply) {
    uint8_t version = 0, code = 0, type = 0;
    probe_extract(reply, "version", &version);
    probe_extract(reply, "code", &code);
    probe_extract(reply, "type", &type);

    if (version == 4) {
        return (type == ICMP_TIMXCEED) && (code == ICMP_TIMXCEED_INTRANS);
    }
    else {
        return (type == ICMP6_TIME_EXCEEDED) && (code == ICMP6_TIME_EXCEED_TRANSIT);
    }
}

static bool fragment_reassembly_time_exceeded(const probe_t *reply) {
    uint8_t version = 0, code = 0, type = 0;
    probe_extract(reply, "version", &version);
    probe_extract(reply, "code", &code);
    probe_extract(reply, "type", &type);

    if (version == 4) {
        return (type == ICMP_TIMXCEED) && (code == ICMP_TIMXCEED_REASS);
    }
    else {
        return (type == ICMP6_TIME_EXCEEDED) && (code == ICMP6_TIME_EXCEED_REASSEMBLY);
    }
}

static bool redirect(const probe_t *reply) {
    uint8_t version = 0, code = 0, type = 0;
    probe_extract(reply, "version", &version);
    probe_extract(reply, "code", &code);
    probe_extract(reply, "type", &type);

    if (version == 4) {
        return (type == ICMP_REDIRECT) && (code == ICMP_REDIRECT_NET);
    }
    else {
        return (type == ND_REDIRECT);
    }
}

static bool parameter_problem(const probe_t *reply) {
    uint8_t version = 0, code = 0, type = 0;
    probe_extract(reply, "version", &version);
    probe_extract(reply, "code", &code);
    probe_extract(reply, "type", &type);

    if (version == 4) {
        return (type == ICMP_PARAMPROB);
    }
    else {
        return (type == ICMP6_PARAM_PROB ) && ((code == ICMP6_PARAMPROB_HEADER) 
                                                || (code == ICMP6_PARAMPROB_OPTION));
    }
}

/**
 * \brief Check whether the destination is reached.
 * \param dst_addr The destination address of this ping instance.
 * \return true iif the destination is reached.
 */

static inline bool destination_reached(const address_t * dst_addr, const probe_t * reply) {
    bool        ret = false;
    address_t   discovered_addr;

    if (probe_extract(reply, "src_ip", &discovered_addr)) {
        ret = (address_compare(dst_addr, &discovered_addr) == 0);
    }
    return ret;
}

//-----------------------------------------------------------------
// Ping algorithm's data
//-----------------------------------------------------------------

/**
 * \brief Allocate a ping_data_t instance
 * \return The newly allocated ping_data_t instance,
 *    NULL in case of failure
 */

static ping_data_t * ping_data_create() { // RENAME
    ping_data_t * ping_data;

    if (!(ping_data = calloc(1, sizeof(ping_data_t))))    goto ERR_MALLOC;
    if (!(ping_data->probes = dynarray_create()))         goto ERR_PROBES;
    // if (!ping_data->rtt_results = dynarray_create())   goto ERR_RTT_RESULTS;
    return ping_data;

// ERR_RTT_RESULTS:
ERR_PROBES:
    free(ping_data);
ERR_MALLOC:
    return NULL;
}

/**
 * \brief Release a ping_data_t instance from the memory
 * \param ping_data The ping_data_t instance we want to release.
 */

static void ping_data_free(ping_data_t * ping_data) {
    if (ping_data) {
        if (ping_data->probes) {
            dynarray_free(ping_data->probes, (ELEMENT_FREE) probe_free);
        // if (ping_data->rtt:results) {
            // dynarray_free(ping_data->rtt_results, NULL);
        }
        free(ping_data);
    }
}

//-----------------------------------------------------------------
// Ping default handler
//-----------------------------------------------------------------

static inline void ttl_dump(const probe_t * probe) {
    uint8_t ttl;
    if (probe_extract(probe, "ttl", &ttl)) printf("%2d ", ttl);
}

static inline void discovered_ip_dump(const probe_t * reply, bool do_resolv) {
    address_t   discovered_addr;
    char      * discovered_hostname;

    if (probe_extract(reply, "src_ip", &discovered_addr)) {
        printf(" ");
        if (do_resolv) {
            if (address_resolv(&discovered_addr, &discovered_hostname, CACHE_ENABLED)) {
                printf("%s", discovered_hostname);
                free(discovered_hostname);
            } else {
                address_dump(&discovered_addr);
            }
            printf(" (");
        }

        address_dump(&discovered_addr);

        if (do_resolv) {
            printf(")");
        }
    }
}

static inline void delay_dump(const probe_t * probe, const probe_t * reply) {
    double send_time = probe_get_sending_time(probe),
           recv_time = probe_get_recv_time(reply);
    printf("  %-5.3lfms  ", 1000 * (recv_time - send_time));
}

static inline double delay_get(const probe_t * probe, const probe_t * reply) {
    return probe_get_recv_time(probe) - probe_get_sending_time(probe);     
}

void ping_handler(
    pt_loop_t            * loop,
    ping_event_t         * ping_event,
    const ping_options_t * ping_options,
    ping_data_t          * ping_data
) {
    const probe_t * probe;
    const probe_t * reply;    

    switch (ping_event->type) {
        case PING_PROBE_REPLY:
            // Retrieve the probe and its corresponding reply
            probe = ((const probe_reply_t *) ping_event->data)->probe;
            reply = ((const probe_reply_t *) ping_event->data)->reply;
            
            if (ping_options->show_timestamp) {    // option -D enabled
                printf("[%lf] ",get_timestamp());
            }

            printf("%d bytes from ", (int)probe_get_size(probe));
            discovered_ip_dump(reply, ping_options->do_resolv);
            printf(" : seq=%d ttl=%d time= ", (int)(ping_data->num_replies), (int)(ping_options->max_ttl));
            // Print delay
            delay_dump(probe, reply);
            //double delay = delay_get(probe, reply); // we need to store the rtts to compute statistics
            //dynarray_push_element(ping_data->rtt_results, &delay);
            break;

        case PING_DEST_NET_UNREACHABLE:
            reply = ((const probe_reply_t *) ping_event->data)->reply;

            printf("From ");
            discovered_ip_dump(reply, ping_options->do_resolv);
            printf(" : seq=%d ", (int)(ping_data->num_replies));
            printf("network unreachable\n");
            break;

        case PING_DEST_HOST_UNREACHABLE:
            reply = ((const probe_reply_t *) ping_event->data)->reply;

            printf("From ");
            discovered_ip_dump(reply, ping_options->do_resolv);
            printf(" : seq=%d ", (int)(ping_data->num_replies));
            printf("host unreachable\n");
            break;

        case PING_DEST_PROT_UNREACHABLE:
            reply = ((const probe_reply_t *) ping_event->data)->reply;

            printf("From ");
            discovered_ip_dump(reply, ping_options->do_resolv);
            printf(" : seq=%d ", (int)(ping_data->num_replies));
            printf("protocol unreachable\n");
            break;

        case PING_DEST_PORT_UNREACHABLE:
            reply = ((const probe_reply_t *) ping_event->data)->reply;

            printf("From ");
            discovered_ip_dump(reply, ping_options->do_resolv);
            printf(" : seq=%d ", (int)(ping_data->num_replies));
            printf("port unreachable\n");
            break;

        case PING_TTL_EXCEEDED_TRANSIT:
            reply = ((const probe_reply_t *) ping_event->data)->reply;

            printf("From ");
            discovered_ip_dump(reply, ping_options->do_resolv);
            printf(" : seq=%d ", (int)(ping_data->num_replies));
            printf("ttl exceeded in transit\n");
            break;

        case PING_TIME_EXCEEDED_REASSEMBLY:
            reply = ((const probe_reply_t *) ping_event->data)->reply;

            printf("From ");
            discovered_ip_dump(reply, ping_options->do_resolv);
            printf(" : seq=%d ", (int)(ping_data->num_replies));
            printf("fragment reassembly time exeeded\n");
            break;

        case PING_REDIRECT:
            reply = ((const probe_reply_t *) ping_event->data)->reply;

            printf("From ");
            discovered_ip_dump(reply, ping_options->do_resolv);
            printf(" : seq=%d ", (int)(ping_data->num_replies));
            printf("redirect\n");
            break;

        case PING_PARAMETER_PROBLEM:
            reply = ((const probe_reply_t *) ping_event->data)->reply;

            printf("From ");
            discovered_ip_dump(reply, ping_options->do_resolv);
            printf(" : seq=%d ", (int)(ping_data->num_replies));
            printf("parameter problem\n");
            break;

        case PING_GEN_ERROR:
            reply = ((const probe_reply_t *) ping_event->data)->reply;

            printf("From ");
            discovered_ip_dump(reply, ping_options->do_resolv);
            printf(" : seq=%d ", (int)(ping_data->num_replies));
            printf("packet has not reached its destination\n");
            break;

        case PING_ALL_PROBES_SENT:
            printf("\n");
            break;

        case PING_TIMEOUT:
            printf("Timeout\n");
            break;

        case PING_WAIT:
            break;

        default:
            break;
        }
        fflush(stdout);
    }

//-----------------------------------------------------------------
// Ping algorithm
//-----------------------------------------------------------------

/**
 * \brief Send a ping probe packet
 * \param loop The main loop
 * \param ping_data Data attached to this instance of ping algorithm
 * \param probe_skel The probe skeleton used to craft the probe packet
 * \param ttl The TTL that we set for this packet
 */

static bool send_ping_probe(
    pt_loop_t         * loop,
    ping_data_t       * ping_data,
    const probe_t     * probe_skel,
    size_t              i
) {
    probe_t * probe;
    double    delay;
    // a probe must never be altered, otherwise the network layer may
    // manage corrupted probes.
    if (!(probe = probe_dup(probe_skel)))                       goto ERR_PROBE_DUP;
    if (probe_get_delay(probe) != DELAY_BEST_EFFORT) {
        delay = i * probe_get_delay(probe_skel);
        probe_set_delay(probe, DOUBLE("delay", delay));
    }
    if (!dynarray_push_element(ping_data->probes, probe))       goto ERR_PROBE_PUSH_ELEMENT;

    return pt_send_probe(loop, probe);

ERR_PROBE_PUSH_ELEMENT:
ERR_PROBE_DUP:
    fprintf(stderr, "Error in send_ping_probe\n");
    return false;
}

/**
 * \brief Send n ping probes toward a destination with a given TTL
 * \param pt_loop The paris traceroute loop
 * \param probe_skel The probe skeleton used to craft the probe packet
 * \param num_probes The amount of probe to send
 * \param ttl Time To Live related to our probe
 * \return true if successful
 */

bool send_ping_probes(
    pt_loop_t         * loop,
    ping_data_t       * ping_data,
    probe_t           * probe_skel,
    size_t              num_probes
) {
    size_t i;
    for (i = 0; i < num_probes; ++i) {
        if (!(send_ping_probe(loop, ping_data, probe_skel, i + 1))) {
            return false;
        }
    }
    return true;
}

/**
 * \brief Handle events to a ping algorithm instance
 * \param loop The main loop
 * \param event The raised event
 * \param pdata Points to a (void *) address that may be altered by ping_handler in order
 *   to manage data related to this instance.
 * \param probe_skel The probe skeleton used to craft the probe packet
 * \param opts Points to the option related to this instance (== loop->cur_instance->options)
 */

// TODO remove opts parameter and define pt_loop_get_cur_options()
int ping_loop_handler(pt_loop_t * loop, event_t * event, void ** pdata, probe_t * probe_skel, void * opts)
{
    ping_data_t          * data = NULL;             // Current state of the algorithm instance
    probe_t              * probe;                   // Probe
    const probe_t        * reply;                   // Reply
    probe_reply_t        * probe_reply;             // (Probe, Reply) pair
    ping_options_t       * options = opts;          // Options passed to this instance
    size_t                 num_probes_to_send  = 0; // the number of probes to send

    switch (event->type) {
        case ALGORITHM_INIT:
            // Check options
            if (!options) {
                fprintf(stderr, "Invalid ping options\n");
                errno = EINVAL;
                goto FAILURE;
            }
            // Allocate structure storing current state information and update *pdata
            if (!(data = ping_data_create())) {
                goto FAILURE;
            }
            *pdata = data;
            // We have to make sure not to send too many probes
            num_probes_to_send = MIN((int)(options_network_get_timeout() / options->interval), options->count);
            break;

        case PROBE_REPLY:
            data        = *pdata;
            probe_reply = (probe_reply_t *) event->data;
            reply       = probe_reply->reply;

            ++(data->num_replies);
            --(data->num_probes_in_flight);
           
            // Notify the caller we've got a response
            if (destination_reached(options->dst_addr, reply)) {
                pt_raise_event(loop, event_create(PING_PROBE_REPLY, probe_reply, NULL, (ELEMENT_FREE) probe_reply_free));
            }
            else if (destination_network_unreachable(reply)) {
                pt_raise_event(loop, event_create(PING_DEST_NET_UNREACHABLE, probe_reply, NULL, (ELEMENT_FREE) probe_reply_free));
            }
            else if (destination_host_unreachable(reply)) {
                pt_raise_event(loop, event_create(PING_DEST_HOST_UNREACHABLE, probe_reply, NULL, (ELEMENT_FREE) probe_reply_free));
            }
            else if (destination_protocol_unreachable(reply)) {
                pt_raise_event(loop, event_create(PING_DEST_PROT_UNREACHABLE, probe_reply, NULL, (ELEMENT_FREE) probe_reply_free));
            }
            else if (destination_port_unreachable(reply)) {
                pt_raise_event(loop, event_create(PING_DEST_PORT_UNREACHABLE, probe_reply, NULL, (ELEMENT_FREE) probe_reply_free));
            }
            else if (ttl_exceeded(reply)) {
                pt_raise_event(loop, event_create(PING_TTL_EXCEEDED_TRANSIT, probe_reply, NULL, (ELEMENT_FREE) probe_reply_free));
            }
            else if (fragment_reassembly_time_exceeded(reply)) {
                pt_raise_event(loop, event_create(PING_TIME_EXCEEDED_REASSEMBLY, probe_reply, NULL, (ELEMENT_FREE) probe_reply_free));
            }
            else if (redirect(reply)) {
                pt_raise_event(loop, event_create(PING_REDIRECT, probe_reply, NULL, (ELEMENT_FREE) probe_reply_free));
            }
            else if (parameter_problem(reply)) {
                pt_raise_event(loop, event_create(PING_PARAMETER_PROBLEM, probe_reply, NULL, (ELEMENT_FREE) probe_reply_free));
            }
            else {
                pt_raise_event(loop, event_create(PING_GEN_ERROR, probe_reply, NULL, (ELEMENT_FREE) probe_reply_free));
            }

            num_probes_to_send = (int)(options->count - data->num_replies) > 0;
            break;

        case PROBE_TIMEOUT:
            data  = *pdata;
            probe = (probe_t *) event->data;
            
            ++(data->num_replies);
            ++(data->num_losses);
            --(data->num_probes_in_flight);

            // Notify the caller we've got a probe timeout
            pt_raise_event(loop, event_create(PING_TIMEOUT, probe, NULL, (ELEMENT_FREE) probe_free));

            num_probes_to_send = (int)(options->count - data->num_replies) > 0;
            break;

        case ALGORITHM_TERMINATED:
            // The caller allows us to free ping's data
            ping_data_free(*pdata);
            *pdata = NULL;
            break;

        case ALGORITHM_ERROR:
            goto FAILURE;

        default:
            break;
    }
    // Forward event to the caller
    pt_algorithm_throw(loop, loop->cur_instance->caller, event);

    // check if we can send another probe or if we have already sent the maximum number of probes
    if (num_probes_to_send > 0 && (data->num_replies + data->num_probes_in_flight != options->count)) {
        send_ping_probes(loop, data, probe_skel, num_probes_to_send);
        data->num_probes_in_flight += (size_t)num_probes_to_send;
    }   
    else {
       
        if (data->num_probes_in_flight == 0) { // we've recieved a response from all the probes we sent
            pt_raise_event(loop, event_create(PING_ALL_PROBES_SENT, NULL, NULL, NULL));
            pt_raise_terminated(loop);
        }
        else { // there are still probes in flight
            pt_raise_event(loop, event_create(PING_WAIT, NULL, NULL, NULL));
        }
    }

    // Handled event must always been free when leaving the handler
    event_free(event);
    return 0;

FAILURE:
    // Handled event must always been free when leaving the handler
    event_free(event);

    // Sent to the current instance a ALGORITHM_FAILURE notification.
    // The caller has to free the data allocated by the algorithm.
    pt_raise_error(loop);
    return EINVAL;
}

static algorithm_t ping = {
    .name    = "ping",
    .handler = ping_loop_handler,
    .options = (const option_t *) &ping_options   
};

ALGORITHM_REGISTER(ping);
