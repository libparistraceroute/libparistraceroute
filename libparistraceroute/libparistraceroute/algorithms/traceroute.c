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
    bool   ret = false;
    char * discovered_ip;
    
    if (probe_extract(reply, "src_ip", &discovered_ip)) {
        ret = !strcmp(discovered_ip, dst_ip);
        free(discovered_ip);
    }
    return ret;
}

/**
 * \brief Send n traceroute probes toward a destination with a given TTL
 * \param pt_loop The paris traceroute loop
 * \param probe_skel The probe skeleton which indicates how to forge a probe
 * \param num_probes The amount of probe to send
 * \param ttl Time To Live related to our probe
 * \return true if successful
 */

bool send_traceroute_probe(
    pt_loop_t * loop,
    probe_t   * probe_skel,
    size_t num_probes,
    uint8_t ttl
) {
    size_t i;
    bool ret = true;

    if (probe_set_fields(probe_skel, I8("ttl", ttl), NULL)) {
        for (i = 0; i < num_probes; ++i) {
            ret &= pt_send_probe(loop, probe_skel);
        }
    } else ret = false;
    return ret; 
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
    const probe_t        * probe;           // Probe 
    const probe_t        * reply;           // Reply
    probe_reply_t        * probe_reply;     // (Probe, Reply) pair
    traceroute_options_t * options = opts;  // Options passed to this instance
    bool                   has_terminated = false;

    switch (event->type) {

        case ALGORITHM_INIT:
            // Check options 
            if (!options || options->min_ttl > options->max_ttl) {
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
            break;

        case PROBE_REPLY:
            data        = *pdata;
            probe_reply = (probe_reply_t *) event->data;
            reply       = probe_reply->reply;

            // Reinitialize star counters, check wether we've discovered an IP address 
            data->num_stars = 0;
            data->num_undiscovered = 0;
            ++(data->num_replies);
            data->destination_reached |= destination_reached(options->dst_ip, reply); 

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
            if (*pdata) free(data);
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
            if (data->num_undiscovered == 3) {
                // We've only discovered stars for the last 3 hops, so give up
                pt_raise_event(loop, event_create(TRACEROUTE_TOO_MANY_STARS, NULL, NULL, NULL));
                pt_raise_terminated(loop);
            }
        } else {
            // Discover the next hop
            if (!send_traceroute_probe(loop, probe_skel, options->num_probes, data->ttl)) {
                goto FAILURE;
            }
            (data->ttl)++;
        }
    }

    // Notify the caller the algorithm has terminated. The caller can still
    // use traceroute's data. It has to run pt_instance_free once this
    // data is no more needed.
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
    .handler = traceroute_handler,
    .options = traceroute_options
};

ALGORITHM_REGISTER(traceroute);
