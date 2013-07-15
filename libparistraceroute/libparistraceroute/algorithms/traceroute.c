#include "traceroute.h"

#include <errno.h>       // errno, EINVAL
#include <stdlib.h>      // malloc
#include <stdio.h>       // fprintf
#include <string.h>      // memset()

#include "../probe.h"
#include "../event.h"
#include "../algorithm.h"
#include "../address.h"  // address_resolv

//-----------------------------------------------------------------
// Traceroute options
//-----------------------------------------------------------------

// Bounded integer parameters
static unsigned min_ttl[3]          = OPTIONS_TRACEROUTE_MIN_TTL;
static unsigned max_ttl[3]          = OPTIONS_TRACEROUTE_MAX_TTL;
static unsigned max_undiscovered[3] = OPTIONS_TRACEROUTE_MAX_UNDISCOVERED;
static unsigned num_queries[3]      = OPTIONS_TRACEROUTE_NUM_QUERIES;
static bool     do_resolv           = OPTIONS_TRACEROUTE_DO_RESOLV_DEFAULT;

static option_t traceroute_options[] = {
    // action           short long                  metavar             help    data
    {opt_store_int_lim, "f",  "--first",            "FIRST_TTL",        HELP_f, min_ttl},
    {opt_store_int_lim, "m",  "--max-hops",         "MAX_TTL",          HELP_m, max_ttl},
    {opt_store_0,       "n",  OPT_NO_LF,            OPT_NO_METAVAR,     HELP_n, &do_resolv},
    {opt_store_int_lim, "q",  "--num-queries",      "NUM_QUERIES",      HELP_q, num_queries},
    {opt_store_int_lim, "M",  "--max-undiscovered", "MAX_UNDISCOVERED", HELP_M, max_undiscovered},
    END_OPT_SPECS
};

uint8_t options_traceroute_get_min_ttl() {
    return min_ttl[0];
}

uint8_t options_traceroute_get_max_ttl() {
    return max_ttl[0];
}

uint8_t options_traceroute_get_max_undiscovered() {
    return max_undiscovered[0];
}

uint8_t options_traceroute_get_num_queries() {
    return num_queries[0];
}

bool options_traceroute_get_do_resolv() {
    return do_resolv;
}

const option_t * traceroute_get_options() {
    return traceroute_options;
}

void options_traceroute_init(traceroute_options_t * traceroute_options, address_t * address)
{
    traceroute_options->min_ttl          = options_traceroute_get_min_ttl();
    traceroute_options->max_ttl          = options_traceroute_get_max_ttl();
    traceroute_options->num_probes       = options_traceroute_get_num_queries();
    traceroute_options->max_undiscovered = options_traceroute_get_max_undiscovered();
    traceroute_options->dst_addr         = address;
    traceroute_options->do_resolv        = options_traceroute_get_do_resolv();

}

inline traceroute_options_t traceroute_get_default_options() {
    traceroute_options_t traceroute_options = {
        .min_ttl          = OPTIONS_TRACEROUTE_MIN_TTL_DEFAULT,
        .max_ttl          = OPTIONS_TRACEROUTE_MAX_TTL_DEFAULT,
        .num_probes       = OPTIONS_TRACEROUTE_NUM_QUERIES_DEFAULT,
        .max_undiscovered = OPTIONS_TRACEROUTE_MAX_UNDISCOVERED_DEFAULT,
        .dst_addr         = NULL,
        .do_resolv        = OPTIONS_TRACEROUTE_DO_RESOLV_DEFAULT,
    };
    return traceroute_options;
};


//-----------------------------------------------------------------
// Traceroute algorithm's data
//-----------------------------------------------------------------

/**
 * \brief Allocate a traceroute_data_t instance
 * \return The newly allocated traceroute_data_t instance,
 *    NULL in case of failure
 */

static traceroute_data_t * traceroute_data_create() {
    traceroute_data_t * traceroute_data;

    if (!(traceroute_data = calloc(1, sizeof(traceroute_data_t)))) goto ERR_MALLOC;
    if (!(traceroute_data->probes = dynarray_create()))            goto ERR_PROBES;
    return traceroute_data;

ERR_PROBES:
    free(traceroute_data);
ERR_MALLOC:
    return NULL;
}

/**
 * \brief Release a traceroute_data_t instance from the memory
 * \param traceroute_data The traceroute_data_t instance we want to release.
 */

static void traceroute_data_free(traceroute_data_t * traceroute_data) {
    if (traceroute_data) {
        if (traceroute_data->probes) {
            dynarray_free(traceroute_data->probes, (ELEMENT_FREE) probe_free);
        }
        free(traceroute_data);
    }
}

//-----------------------------------------------------------------
// Traceroute default handler
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
            if (address_resolv(&discovered_addr, &discovered_hostname)) {
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

void traceroute_handler(
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

            // Print TTL and discovered IP if this is the first probe related to this TTL
            if (num_probes_printed % traceroute_options->num_probes == 0) {
                ttl_dump(probe);
                discovered_ip_dump(reply, traceroute_options->do_resolv);
            }

            // Print delay
            delay_dump(probe, reply);
            fflush(stdout);
            num_probes_printed++;
            break;

        case TRACEROUTE_STAR:
            probe = (const probe_t *) traceroute_event->data;
            if (num_probes_printed % traceroute_options->num_probes == 0) {
                ttl_dump(probe);
            }
            printf(" *");
            fflush(stdout);
            num_probes_printed++;
            break;

        case TRACEROUTE_ICMP_ERROR:
            printf(" !");
            num_probes_printed++;
            break;

        case TRACEROUTE_DESTINATION_REACHED:
        case TRACEROUTE_TOO_MANY_STARS:
        case TRACEROUTE_MAX_TTL_REACHED:
        default:
            break;
    }

    if (num_probes_printed % traceroute_options->num_probes == 0) {
        printf("\n");
    }
}


//-----------------------------------------------------------------
// Traceroute algorithm
//-----------------------------------------------------------------

/**
 * \brief Check whether the destination is reached.
 * \param dst_addr The destination address of this traceroute instance.
 * \return true iif the destination is reached.
 */

static inline bool destination_reached(const address_t * dst_addr, const probe_t * reply) {
    bool        ret = false;
    address_t   discovered_addr;

    if (probe_extract(reply, "src_ip", &discovered_addr)) {
        ret = (address_cmp(dst_addr, &discovered_addr) == 0);
    }
    return ret;
}

/**
 * \brief Send a traceroute probe packet
 * \param loop The main loop
 * \param traceroute_data Data attached to this instance of traceroute algorithm
 * \param probe_skel The probe skeleton used to craft the probe packet
 * \param ttl The TTL that we set for this packet
 */

static bool send_traceroute_probe(
    pt_loop_t         * loop,
    traceroute_data_t * traceroute_data,
    const probe_t     * probe_skel,
    uint8_t             ttl,
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
    if (!probe_set_fields(probe, I8("ttl", ttl), NULL))         goto ERR_PROBE_SET_FIELDS;
    if (!dynarray_push_element(traceroute_data->probes, probe)) goto ERR_PROBE_PUSH_ELEMENT;

    return pt_send_probe(loop, probe);

ERR_PROBE_PUSH_ELEMENT:
ERR_PROBE_SET_FIELDS:
    probe_free(probe);
ERR_PROBE_DUP:
    fprintf(stderr, "Error in send_traceroute_probe\n");
    return false;
}
/*
static bool send_traceroute_probe_scheduled(
    pt_loop_t         * loop,
    traceroute_data_t * traceroute_data,
    const probe_t     * probe_skel,
    uint8_t             ttl
) {
    probe_t * probe;

    // a probe must never be altered, otherwise the network layer may
    // manage corrupted probes.
    if (!(probe = probe_dup(probe_skel)))                       goto ERR_PROBE_DUP;
    if (!probe_set_field(probe, I8("ttl", ttl)))                goto ERR_PROBE_SET_FIELD;
    if (!dynarray_push_element(traceroute_data->probes, probe)) goto ERR_PROBE_PUSH_ELEMENT;

    return pt_send_probe(loop, probe);

ERR_PROBE_PUSH_ELEMENT:
ERR_PROBE_SET_FIELD:
    probe_free(probe);
ERR_PROBE_DUP:
    fprintf(stderr, "Error in send_traceroute_probe\n");
    return false;
}
*/

/**
 * \brief Send n traceroute probes toward a destination with a given TTL
 * \param pt_loop The paris traceroute loop
 * \param probe_skel The probe skeleton used to craft the probe packet
 * \param num_probes The amount of probe to send
 * \param ttl Time To Live related to our probe
 * \return true if successful
 */

bool send_traceroute_probes(
    pt_loop_t         * loop,
    traceroute_data_t * traceroute_data,
    probe_t           * probe_skel,
    size_t              num_probes,
    uint8_t             ttl
) {
    size_t i;

    for (i = 0; i < num_probes; ++i) {
        if (!(send_traceroute_probe(loop, traceroute_data, probe_skel, ttl, i + 1))) {
            return false;
        }
    }
    return true;
}

/**
 * \brief Handle events to a traceroute algorithm instance
 * \param loop The main loop
 * \param event The raised event
 * \param pdata Points to a (void *) address that may be altered by traceroute_handler in order
 *   to manage data related to this instance.
 * \param probe_skel The probe skeleton used to craft the probe packet
 * \param opts Points to the option related to this instance (== loop->cur_instance->options)
 */

// TODO remove opts parameter and define pt_loop_get_cur_options()
int traceroute_loop_handler(pt_loop_t * loop, event_t * event, void ** pdata, probe_t * probe_skel, void * opts)
{
    traceroute_data_t    * data = NULL;     // Current state of the algorithm instance
    probe_t              * probe;           // Probe
    const probe_t        * reply;           // Reply
    probe_reply_t        * probe_reply;     // (Probe, Reply) pair
    traceroute_options_t * options = opts;  // Options passed to this instance
    bool                   discover_next_hop = false;
    bool                   has_terminated = false;

    switch (event->type) {

        case ALGORITHM_INIT:
            // Check options
            if (!options || options->min_ttl > options->max_ttl) {
                fprintf(stderr, "Invalid traceroute options\n");
                errno = EINVAL;
                goto FAILURE;
            }

            // Allocate structure storing current state information and update *pdata
            if (!(data = traceroute_data_create())) {
                goto FAILURE;
            }
            *pdata = data;
            data->ttl = options->min_ttl;
            break;

        case PROBE_REPLY:
            data        = *pdata;
            probe_reply = (probe_reply_t *) event->data;
            reply       = probe_reply->reply;

            // Reinitialize star counters, check wether we've discovered an IP address
            data->num_stars = 0;
            data->num_undiscovered = 0;
            ++(data->num_replies);
            data->destination_reached |= destination_reached(options->dst_addr, reply);

            // Notify the caller we've discovered an IP address
            pt_raise_event(loop, event_create(TRACEROUTE_PROBE_REPLY, probe_reply, NULL, (ELEMENT_FREE) probe_reply_free));
            break;

        case PROBE_TIMEOUT:
            data  = *pdata;
            probe = (probe_t *) event->data;

            // Update counters
            ++(data->num_stars);
            ++(data->num_replies);

            // Notify the caller we've got a probe timeout
            pt_raise_event(loop, event_create(TRACEROUTE_STAR, probe, NULL, (ELEMENT_FREE) probe_free));
            break;

        case ALGORITHM_TERMINATED:

            // The caller allows us to free traceroute's data
            traceroute_data_free(*pdata);
            *pdata = NULL;
            break;

        case ALGORITHM_ERROR:
            goto FAILURE;

        default:
            break;
    }

    // Forward event to the caller
    pt_algorithm_throw(loop, loop->cur_instance->caller, event);

    // Explore next hop
    if ((data->num_replies % options->num_probes) == 0) {
        if (data->destination_reached) {
            // We've reached the destination
            pt_raise_event(loop, event_create(TRACEROUTE_DESTINATION_REACHED, NULL, NULL, NULL));
            pt_raise_terminated(loop);
        } else if (data->ttl > options->max_ttl) {
            // We've reached the maximum TTL
            pt_raise_event(loop, event_create(TRACEROUTE_MAX_TTL_REACHED, NULL, NULL, NULL));
            pt_raise_terminated(loop);
        } else if (data->num_stars == options->num_probes) {
            // We've only discovered stars for the current hop
            ++(data->num_undiscovered);
            if (data->num_undiscovered == options->max_undiscovered) {
                // We've only discovered stars for the last "max_undiscovered" hops, so give up
                pt_raise_event(loop, event_create(TRACEROUTE_TOO_MANY_STARS, NULL, NULL, NULL));
                pt_raise_terminated(loop);
            } else {
                // Skip this hop and explore the next one
                discover_next_hop = true;
            }
        } else discover_next_hop = true;

        if (discover_next_hop) {
            data->num_stars = 0;

            // Discover the next hop
            if (!send_traceroute_probes(loop, data, probe_skel, options->num_probes, data->ttl)) {
                goto FAILURE;
            }
            (data->ttl)++;
        }

    }
    // Notify the caller the algorithm has terminated. The caller can still
    // use traceroute's data. It has to run pt_instance_free once this
    // data if no more needed.
    if (has_terminated) {
        pt_raise_terminated(loop);
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

static algorithm_t traceroute = {
    .name    = "traceroute",
    .handler = traceroute_loop_handler,
    .options = (const option_t *) &traceroute_options // TODO akram pass pointer to traceroute_get_options
};

ALGORITHM_REGISTER(traceroute);
