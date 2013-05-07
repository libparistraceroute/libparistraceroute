#include <errno.h>       // errno, EINVAL
#include <stdlib.h>      // malloc
#include <stdio.h>       // TODO debug 
#include <stdbool.h>     // bool
#include <string.h>      // memset()

#include "../probe.h"
#include "../event.h"
#include "../pt_loop.h"
#include "../algorithm.h"

#include "traceroute.h"

// traceroute options

struct opt_spec traceroute_options[] = {
    // action       short       long      metavar    help           data
    {opt_store_int, OPT_NO_SF, "min-ttl", "MIN_TTL", "minimum TTL", NULL},
    {opt_store_int, OPT_NO_SF, "max-ttl", "MAX_TTL", "maximum TTL", NULL},
    {OPT_NO_ACTION}
};

// TODO to remove, see opt_spec 
inline traceroute_options_t traceroute_get_default_options() {
    traceroute_options_t traceroute_options = {
        .min_ttl    = 1,
        .max_ttl    = 30,
        .num_probes = 3,
        .dst_ip     = NULL
    };
    return traceroute_options;
};


/**
 * \brief Complete the options passed as parameter with the
 *   traceroute-specific options
 * \param options A dynarray containing zero or more opt_spec
 *   instances
 */
void traceroute_update_options(dynarray_t * options) {
    // TODO push_element pour chaque opt initialisé avec l'options
}

//-----------------------------------------------------------------
// Traceroute algorithm
//-----------------------------------------------------------------

/**
 * \brief Check whether the destination is reached
 * \return true iif the destination is reached
 */

static inline bool destination_reached(const char * dst_ip, const probe_t * reply) {
    return !strcmp(probe_get_field(reply, "src_ip")->value.string, dst_ip);
}

/**
 * \brief Send a traceroute probe
 * \param pt_loop The paris traceroute loop
 * \param probe_skel The probe skeleton which indicates how to forge a probe
 * \param ttl Time To Live related to our probe
 * \return true if successful
 */

bool send_traceroute_probe(pt_loop_t * loop, probe_t * probe_skel, uint8_t ttl) {
    if (probe_set_fields(probe_skel, I8("ttl", ttl), NULL)) {
        pt_send_probe(loop, probe_skel);
        return true;
    }
    return false; 
}

/**
 * \brief Handle events to a traceroute algorithm instance
 * \param loop The main loop
 * \param event The raised event
 * \param pdata Points to a (void *) address that may be altered by traceroute_handler in order
 *   to manage data related to this instance.
 * \param probe_skel The probe skeleton
 * \param opts Points to the option related to this instance (== loop->cur_instance->options)
 */

// TODO remove opts parameter and define pt_loop_get_cur_options()
int traceroute_handler(pt_loop_t * loop, event_t * event, void ** pdata, probe_t * probe_skel, void * opts)
{
    traceroute_data_t    * data = NULL;     // Current state of the algorithm instance
    const probe_t        * reply;           // Reply
    probe_reply_t        * probe_reply;     // (Probe, Reply) pair
    traceroute_options_t * options = opts;  // Options passed to this instance

    switch (event->type) {

        case ALGORITHM_INIT:
            // Check options 
            if (!options || options->min_ttl >= options->max_ttl) {
                errno = EINVAL;
                goto FAILURE;
            }

            // Allocate structure storing current state information and update *pdata
            if (!(data = malloc(sizeof(traceroute_data_t)))) {
                goto FAILURE;
            } else *pdata = data;

            // Initialize traceroute_data_t structure
            memset(data, 0, sizeof(traceroute_data_t));
            data->ttl = options->min_ttl;

            // Send first probe
            if (!send_traceroute_probe(loop, probe_skel, data->ttl)) {
                goto FAILURE;
            }
            (data->num_sent_probes)++;
            break;

        case PROBE_REPLY:

            data  = *pdata;
            probe_reply = (probe_reply_t *) event->data;
            reply = probe_reply->reply;

            // Reinitialize star counters, we've discovered an IP address
            data->num_stars = 0;
            data->num_undiscovered = 0;

            // Move to traceroute_user_handler
            // Print reply (i-th reply corresponding to the current hop)
            data->destination_reached |= destination_reached(options->dst_ip, reply); 

            pt_raise_event(loop, event_create(TRACEROUTE_PROBE_REPLY, probe_reply, NULL, (ELEMENT_FREE) probe_reply_free));

            // We've sent num_probes packets for the current hop
            if ((data->num_sent_probes % options->num_probes) == 0) {
                // We've reached the destination
                if (data->destination_reached) {
                    pt_raise_event(loop, event_create(TRACEROUTE_DESTINATION_REACHED, NULL, NULL, NULL));
                    break;
                }

                // Increase TTL in order to discover the next hop 
                (data->ttl)++;
            }

            // Send next probe packet (DUPLICATE code)
            if (data->ttl > options->max_ttl) {
                pt_raise_event(loop, event_create(TRACEROUTE_MAX_TTL_REACHED, NULL, NULL, NULL));
            } else {
                if (!send_traceroute_probe(loop, probe_skel, data->ttl)) {
                    goto FAILURE;
                }
                (data->num_sent_probes)++;
            }

            break;

        case PROBE_TIMEOUT:

            data  = *pdata;
            ++(data->num_stars);

            // We've sent num_probes packets for the current hop
            if (data->num_sent_probes % options->num_probes == 0) {
                // Check whether we've got at least one reply for this hop 
                if (data->num_stars == options->num_probes) {
                    ++(data->num_undiscovered);
                }

                // The 3 last hops have not been discovered, give up!
                if (data->num_undiscovered == 3) break;

                // Increase TTL in order to discover the next hop 
                (data->ttl)++;
            }

            // Send next probe packet (DUPLICATED code)
            if (data->ttl > options->max_ttl) {
                pt_raise_event(loop, event_create(TRACEROUTE_MAX_TTL_REACHED, NULL, NULL, NULL));
            } else {
                if (!send_traceroute_probe(loop, probe_skel, data->ttl)) {
                    goto FAILURE;
                }
                (data->num_sent_probes)++;
            }

            break;
        case ALGORITHM_TERMINATED:
            if (*pdata) free(data);
            *pdata = NULL;
            break;
        case ALGORITHM_ERROR:
            goto FAILURE;
        default:
            // Unhandled events
            break;
    }

    // Forward event to the caller
    pt_algorithm_throw(loop, loop->cur_instance->caller, event);
    return 0;
FAILURE:
    // This event is not forwarded to the caller, so we have to release it
    // from the memory
    event_free(event);

    // Sent to the current instance a ALGORITHM_FAILURE notification.
    // The caller has to free the data allocated by the algorithm.
    pt_raise_error(loop);
    return EINVAL;
}

static algorithm_t traceroute = {
    .name    = "traceroute",
    .handler = traceroute_handler,
    .options = traceroute_options
};

ALGORITHM_REGISTER(traceroute);
