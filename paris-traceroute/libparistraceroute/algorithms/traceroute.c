/* TODO
 * The algorithm will have to expose a set of options to the commandline if it wants 
 * to be called from it
 */

#include <stdlib.h>
#include <stdio.h> // XXX
#include <stdbool.h>
#include <string.h>      // memcpy()

#include "../probe.h"
#include "../event.h"
#include "../pt_loop.h"
#include "../algorithm.h"

#include "traceroute.h"

//-----------------------------------------------------------------
// Internal data management
//-----------------------------------------------------------------

/**
 * \brief This structure stores internal data of the algorithm
 */

typedef struct {
    unsigned    num_sent_probes; /**< Amount of probes sent to the current hop */
    unsigned    ttl;             /**< The current TTL */
    probe_t  ** probes;          /**< Probes related to the current hop */
    probe_t  ** replies;         /**< Replies related to each of these probes */
    traceroute_caller_data_t * caller_data;
} traceroute_data_t;   

/**
 * \brief Reinitialize every fields of a traceroute_data_t instance but ttl.
 * \param instance Instance of the algorithm
 * \param traceroute_data The instance we reinitialize
 */

static void traceroute_data_reset(
    traceroute_data_t    * traceroute_data
) {
    traceroute_options_t * traceroute_options = NULL;
    traceroute_options = NULL;
    //if (!traceroute_options) return; 

    if (traceroute_data) {
        memset(traceroute_data->probes,  0, traceroute_options->num_probes);
        memset(traceroute_data->replies, 0, traceroute_options->num_probes);
        traceroute_data->num_sent_probes = 0;
    }
}

/**
 * \brief Free internal data 
 * \param traceroute_data A pointer to internal data
 */

static void traceroute_data_free(traceroute_data_t * traceroute_data)
{
    if (traceroute_data) {
        if (traceroute_data->probes)      free(traceroute_data->probes);
        if (traceroute_data->replies)     free(traceroute_data->replies);
        if (traceroute_data->caller_data) free(traceroute_data->caller_data);
        free(traceroute_data);
    }
}

/**
 * \brief Allocate internal data needed by traceroute
 * \param instance Instance of the algorithm
 * \return The corresponding structure
 */

traceroute_data_t * traceroute_data_create(void)
{
    traceroute_options_t * options;
    traceroute_data_t    * traceroute_data = NULL;

    // Get options
    options = NULL;
    // XXX if (!options) goto FAILURE; 

    // Alloc traceroute_data
    traceroute_data = calloc(1, sizeof(traceroute_data_t));
    if (!traceroute_data) goto FAILURE;
    
    // Alloc probes 
    traceroute_data->probes = malloc(sizeof(probe_t) * options->num_probes);
    if (!traceroute_data->probes) goto FAILURE;

    // Alloc replies 
    traceroute_data->replies = malloc(sizeof(probe_t) * options->num_probes);
    if (!traceroute_data->replies) goto FAILURE;

    // Alloc user data 
    traceroute_data->caller_data = malloc(sizeof(traceroute_caller_data_t));
    if (!traceroute_data->caller_data) goto FAILURE;

    // Initialization 
    traceroute_data_reset(traceroute_data);
    return traceroute_data;

FAILURE:
    traceroute_data_free(traceroute_data);
    return NULL;
}

//-----------------------------------------------------------------
// Traceroute algorithm
//-----------------------------------------------------------------

/**
 * \brief Check whether a reply is a star 
 * \return true iif this is a star 
 */

static inline bool is_star(const probe_t * probe) {
    return probe == NULL;
}

/**
 * \brief Check whether the destination is reached
 * \return true iif the destination is reached
 */

static inline bool destination_reached(
    const char * dst_ip,
    probe_t    * reply
) {
    return !strcmp(
        probe_get_field(reply, "src_ip")->value.string,
        dst_ip
    );
}

/**
 * \brief Check whether an ICMP error has occured 
 * \return true iif an error has occured 
 */

static inline bool stopping_icmp_error(const probe_t * reply) {
    // Not implemented
    return false;
}

/**
 * \brief Send a traceroute probe
 * \param pt_loop The paris traceroute loop
 * \param probe_skel The probe skeleton which indicates how to forge a probe
 * \param ttl Time To Live related to our probe
 * \return true if successful
 */

bool send_traceroute_probe(
    pt_loop_t * loop,
    probe_t   * probe_skel,
    uint8_t     ttl
) {
    if (probe_set_fields(probe_skel, I8("ttl", ttl), NULL) == 0) {
        probe_dump(probe_skel);
        pt_send_probe(loop, probe_skel);
        return true;
    }
    return false; 
}

/**
 * \brief How the algorithm handles the different events
 * \param loop Paris Traceroute's loop
 * \param instance The instance carry every relevant information from
 *  the upper scopes.
 */

int traceroute_handler(pt_loop_t *loop, event_t *event, void **pdata, probe_t *skel)
{ 
    unsigned int           j, num_probes, num_sent_probes;
    bool                   all_stars;
    traceroute_data_t    * data; // is NULL before ALGORITHM_INIT
    probe_t              * reply;
    probe_t              * probe;
    probe_reply_t        * probe_reply;

    traceroute_options_t * options = NULL; // XXX TODO

    //if (!options)         goto FAILURE; 
    //if (!options->dst_ip) goto FAILURE; 

    switch (event->type) {
        case ALGORITHM_INIT: // Algorithm initialization

            *pdata = traceroute_data_create();
            if (!*pdata)
                goto FAILURE;
            data = *pdata;

            // Let's start
            data->ttl = options->min_ttl;
            if (send_traceroute_probe(loop, skel, data->ttl)) {
                data->num_sent_probes++;
            } else goto FAILURE;
            break;

        case PROBE_REPLY:

            data = *pdata;
            if (!data->caller_data) goto FAILURE;

            num_probes      = options->num_probes;
            num_sent_probes = data->num_sent_probes;

            probe_reply = event->data;
            probe = probe_reply->probe;
            reply = probe_reply->reply;
            data->probes [num_sent_probes - 1] = probe;
            data->replies[num_sent_probes - 1] = reply;

            // Send event to the caller
            data->caller_data->type = TRACEROUTE_PROBE_REPLY;
            data->caller_data->value.probe_reply.options         = options;
            data->caller_data->value.probe_reply.discovered_ip   = probe_get_field(reply, "src_ip")->value.string;
            data->caller_data->value.probe_reply.current_ttl     = data->ttl;
            data->caller_data->value.probe_reply.num_sent_probes = num_sent_probes;
            // XXX pt_algorithm_throw(loop, instance->caller, event_create(ALGORITHM_ANSWER, data->caller_data));

            // If we've collected every probes related to this hop 
            if (num_sent_probes >= num_probes) {
                all_stars = true;
                for (j = 0; j < num_probes; ++j) {
                    all_stars &= is_star(data->probes[j]);
                    if (destination_reached(options->dst_ip, data->replies[j])) {
                        // TODO send_message "destination reached"
                        goto SUCCESS;
                    } else if (stopping_icmp_error(data->replies[j])) {
                        // TODO send_message "icmp error"
                        goto SUCCESS;
                    }
                }

                if (all_stars) {
                    // TODO send_message "all stars"
                    goto SUCCESS;
                }

                // Increment ttl
                (data->ttl)++;
                if (data->ttl == options->max_ttl) {
                    // TODO send_message "ttl max reached"
                    goto SUCCESS;
                }

                // Reset data related to the current ttl
                traceroute_data_reset(data);
            }

            // Send probe
            if (send_traceroute_probe(loop, skel, data->ttl)) {
                data->num_sent_probes++;
            } else goto FAILURE;

            break;

        case ALGORITHM_TERMINATED:
            // The caller does not need anymore 'traceroute' datas.
            traceroute_data_free(*pdata);
            break;

        default: break; // Events not handled
    }
    return 0; // Traceroute algorithm is not yet finished

SUCCESS: // This traceroute has successfully ended
    // The caller has to release the memory in the handler in ALGORITHM_TERMINATED
    // by calling traceroute_data_free()
    // XXX pt_algorithm_terminate(loop, instance);
    return 0;
FAILURE: // This algorithm has crashed
    // The caller has to release the memory in the handler in ALGORITHM_FAILURE
    // by calling traceroute_data_free()
    // XXX pt_algorithm_throw(loop, instance, event_create(ALGORITHM_FAILURE, NULL));
    return -1;
}

static algorithm_t traceroute = {
    .name    = "traceroute",
    .handler = traceroute_handler
};

ALGORITHM_REGISTER(traceroute);
