#ifndef ALGORITHMS_MDA_JSON
#define ALGORITHMS_MDA_JSON

#include <stdio.h>                      // FILE *

#include "../../probe.h"                // probe_t
#include "../../containers/map.h"       // map_t
#include "traceroute_enriched_data.h"   // enriched_reply_t

//---------------------------------------------------------------------------
// JSON output
//---------------------------------------------------------------------------

// TODO: Kevin: comments

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

// TODO static
void replies_to_json_dump(const map_t * replies_by_ttl);

/**
 * @brief Print the output of MDA stars to JSON on the standard output
 * @param stars_by_ttl A map which associates for each TTL the corresponding replies.
 */

// TODO static
void stars_to_json_dump(const map_t * stars_by_ttl);

/**
 * @brief Print some metadata of the MDA to JSON.
 * @param user_data Data used to transport some relevant metadata.
 */

//void mda_infos_dump(const user_data_t * user_data);


// TODO: Kévin: comments
void json_print_header(FILE * f_json, const char * source, const char * destination, const char * protocol);
void json_print_footer(FILE * f_json);
#endif
