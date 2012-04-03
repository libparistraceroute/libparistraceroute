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

#include "traceroute.h"

//-----------------------------------------------------------------
// Internal data management
//-----------------------------------------------------------------

/**
 * \brief This structure stores internal data of the algorithm
 */

typedef struct traceroute_data_s {
    unsigned    num_sent_probes; /**< Amount of probes sent to the current hop */
    unsigned    ttl;             /**< The current TTL */
    probe_t  ** probes;          /**< Probes related to the current hop */
} traceroute_data_t;   

/**
 * \brief Reinitialize fields of a traceroute_data_t instance
 * \param instance Instance of the algorithm
 * \param traceroute_data The instance we reinitialize
 */

void traceroute_data_reset(algorithm_instance_t * instance, traceroute_data_t * traceroute_data)
{
    traceroute_options_t * traceroute_options = NULL;
    traceroute_options = algorithm_instance_get_options(instance);
    if (!traceroute_options) return; 

    if (traceroute_data) {
        memset(traceroute_data->probes, 0, traceroute_options->num_probes);
        traceroute_data->num_sent_probes = 0;
    }
}

/**
 * \brief Free internal data 
 * \param traceroute_data A pointer to internal data
 */

void traceroute_data_free(traceroute_data_t * traceroute_data)
{
    if (traceroute_data) {
        if (traceroute_data->probes) free(traceroute_data->probes);
        free(traceroute_data);
    }
}


/**
 * \brief Allocate internal data needed by traceroute
 * \param instance Instance of the algorithm
 * \return The corresponding structure
 */

traceroute_data_t *traceroute_data_create(algorithm_instance_t * instance)
{
    traceroute_options_t * options         = NULL;
    traceroute_data_t    * traceroute_data = NULL;

    traceroute_data = malloc(sizeof(traceroute_data_t));
    if (!traceroute_data) goto FAILURE;
    
    options = algorithm_instance_get_options(instance);
    if (!options) goto FAILURE; 

    traceroute_data->probes = malloc(sizeof(probe_t) * options->num_probes);
    if (!traceroute_data->probes) goto FAILURE;

    traceroute_data_reset(instance, traceroute_data);
    return traceroute_data;

FAILURE:
    printf("FAILURE\n");
    if(traceroute_data) traceroute_data_free(traceroute_data);
    return NULL;
}

//-----------------------------------------------------------------
// Traceroute algorithm
//-----------------------------------------------------------------

/**
 * \brief Check whether a reply is a star 
 * \return true iif this is a star 
 */

bool is_star(const probe_t * probe) {
    return probe == NULL;
}

/**
 * \brief Check whether the destination is reached
 * \return true iif the destination is reached
 */

bool destination_reached(const probe_t * probe) {
    // Not yet implemented
    return false;
}

/**
 * \brief Check whether an ICMP error has occured 
 * \return true iif an error has occured 
 */

bool stopping_icmp_error(const probe_t * probe) {
    // Not yet implemented
    return false;
}

/**
 * \brief How the algorithm handles the different events
 * \param loop Paris Traceroute's loop
 * \param instance The instance carry every relevant information from
 *  the upper scopes.
 */

void traceroute_handler(pt_loop_t * loop, algorithm_instance_t * instance)
{ 
    traceroute_data_t    * data;
    unsigned int           i;
  //probe_t              * probe_skel = algorithm_instance_get_probe_skel(instance);
  //void                ** data     
    probe_t              * probe;
    unsigned int           num_probes;
    unsigned int           num_sent_probes;
    bool                   stop;
    bool                   all_stars;

    traceroute_options_t * options    = algorithm_instance_get_options(instance);
    event_t             ** events     = algorithm_instance_get_events(instance);
    unsigned int           num_events = algorithm_instance_get_num_events(instance);

    if (!options) goto FAILURE; 
    if (!events)  goto FAILURE;
    //if (!data)    goto FAILURE; // data is null before init

    // For each events, execute the appriopriate code
    for (i = 0; i < num_events; i++) {
        switch (events[i]->type) {
            case ALGORITHM_INIT:

                // Algorithm initialization
                data = traceroute_data_create(instance);
                if(!data) goto FAILURE;
                algorithm_instance_set_data(instance, data);

                // Create a probe with ttl = 1 and send it
                // TODO Use probe_skel 
                probe = probe_create();

                // We should be able not to specify the protocol stack
                probe_set_protocols(probe, "ipv4", "udp", NULL);
                probe_set_fields(probe, I8("ttl", options->min_ttl), NULL);
                probe_set_fields(probe, STR("dst_ip", "1.1.1.1"), NULL);
                pt_probe_send(loop, probe);
                // probe_free(probe); // TODO we don't want to make copies
                break;

            case PROBE_REPLY_RECEIVED:
                data = algorithm_instance_get_data(instance);
                printf("traceroute::INIT probe reply received\n");

                num_probes      = options->num_probes;
                num_sent_probes = data->num_sent_probes;

                data->probes[data->num_sent_probes] = ((reply_received_params_t *) events[i]->params)->probe;
                data->num_sent_probes++;
                if (num_sent_probes < num_probes) {
                    // We've not yet collected the all probes for this hop
                    break; 
                }

                // We've collected every probes for this hop 
                stop = false;
                all_stars = true;
                for(i = 0; i < num_probes; ++i) {
                    all_stars &= is_star(data->probes[i]);
                    if(destination_reached(data->probes[i])
                    || stopping_icmp_error(data->probes[i])
                    ) {
                        stop = true;
                        break;
                    }
                }
                stop |= all_stars;

                if (stop) goto SUCCESS;

                data->ttl++;
                if (data->ttl == options->max_ttl) goto SUCCESS;
                break;

            case ALGORITHM_TERMINATED:
                // An algorithm called by traceroute has finished
                break;
        }
        pt_algorithm_terminate(loop, instance);
    }
    return;
SUCCESS: // This algorithm has successfully ended
    return;
FAILURE: // This algorithm has crashed
    // We let the user release the memory (traceroute_data_free())
    return;
}

static algorithm_t traceroute = {
    .name    = "traceroute",
    .handler = traceroute_handler
};

ALGORITHM_REGISTER(traceroute);
