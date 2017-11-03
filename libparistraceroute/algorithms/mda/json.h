#ifndef ALGORITHMS_MDA_JSON
#define ALGORITHMS_MDA_JSON
#include <stdio.h>

#include "../../probe.h"
#include "../../containers/vector.h"
#include "../../containers/map.h"
#include "../mda.h"
#include "mda_enriched_data.h"

//---------------------------------------------------------------------------
// JSON output
//---------------------------------------------------------------------------

// TODO: Kevin: comments

typedef struct {
    probe_t * reply;
    double    delay;
} enriched_reply_t;

enriched_reply_t * enriched_reply_shallow_copy(const enriched_reply_t * reply);

void vector_enriched_reply_free(vector_t * vector);

void map_probe_free(map_t * map);

void json_handler(
    mda_event_type_t   mda_event_type,
    probe_t          * probe,
    enriched_reply_t * enriched_reply,
    user_data_t      * user_data,
    bool               sorted_print
);

/**
 * @brief Handler for enriched output (json, xml, ... ).
 * @param loop Loop of events.
 * @param mda_event The MDA event raised.
 * @param traceroute_options Options passed to traceroute algorithm.
 * @param user_data Custom metadata/structure to produce JSON / XML outputs.
 */

void traceroute_enriched_handler(
    pt_loop_t                  * loop,
    mda_event_t                * mda_event,
    const traceroute_options_t * traceroute_options,
    user_data_t                * user_data,
    bool                         sorted_print
);

/**
 * @brief Print a reply to JSON format.
 * @param enriched_reply The MDA reply with the information for the JSON output.
 * @param f_json A valid file descriptor used to write the reply.
 */

void reply_to_json(const enriched_reply_t * enriched_reply, FILE * f_json);

/**
 * @brief Print the output of MDA replies to JSON.
 * @param replies_by_ttl A map which associates for each TTL the corresponding replies.
 * @param f_json The output file descriptor (e.g. stdout).
 */

void replies_to_json(const map_t * replies_by_ttl, FILE * f_json);

/**
 * @brief Print a star to JSON format.
 * @param star MDA probe that did not get a reply and for what we got a star.
 * @param f_json The output file descriptor (e.g. stdout).
 */

void star_to_json(const probe_t * star, FILE * f_json);

/**
 * @brief Print the output of MDA replies to JSON.
 * @param stars_by_ttl A map which associates for each TTL the corresponding stars.
 * @param f_json The output file descriptor (e.g. stdout).
 */

void stars_to_json(const map_t * stars_by_ttl, FILE * f_json);

/**
 * @brief Print the output of MDA replies to JSON on the standard output.
 * @param replies_by_ttl A map which associates for each TTL the corresponding replies.
 */

void replies_to_json_dump(const map_t * replies_by_ttl);

/**
 * @brief Print the output of MDA stars to JSON on the standard output
 * @param stars_by_ttl A map which associates for each TTL the corresponding replies.
 */

void stars_to_json_dump(const map_t * stars_by_ttl);

/**
 * @brief Print some metadata of the MDA to JSON.
 * @param user_data Data used to transport some relevant metadata.
 */

//void mda_infos_dump(const user_data_t * user_data);

#endif
