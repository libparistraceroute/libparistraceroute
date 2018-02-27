#ifndef ALGORITHMS_OUTPUTS_TRACEROUTE_ENRICHED_DATA_H
#define ALGORITHMS_OUTPUTS_TRACEROUTE_ENRICHED_DATA_H

#include "../../containers/map.h"       // map_t
#include "../../containers/vector.h"    // map_t
#include "../../probe.h"                // probe_t
#include "../../pt_loop.h"              // pt_loop_t
#include "../mda.h"                     // mda_event_t
#include "traceroute_output_format.h"   // traceroute_output_format_t

/**
 * The following data structure is passed to json_handler() and
 * xml_handler() to produced extended MDA ouputs.
 */

typedef struct {
    traceroute_output_format_t   format;           /**< FORMAT_XML|FORMAT_JSON|FORMAT_DEFAULT */
    map_t                      * replies_by_ttl;   /**< Maps each TTL with the corresponding vector of replies. */
    map_t                      * stars_by_ttl;     /**< Maps each TTL with the corresponding vector of stars. */
    bool                         is_first_result;  /**< Akward variable for first probe or star recieved for json compliance. */
    char                       * source;           /**< Source address. */
    const char                 * destination;      /**< MDA target address. */
    char                       * destination_ip;   /**< MDA IP target address */
    const char                 * protocol;         /**< Protocol. */
} traceroute_enriched_user_data_t;

// TODO: Kévin: comments

traceroute_enriched_user_data_t * traceroute_enriched_user_data_create(int family, const char * protocol_name, const char * dst_ip, const char * format_name);
void traceroute_enriched_user_data_free(traceroute_enriched_user_data_t * user_data);

typedef struct {
    const probe_t * reply;
    uint8_t         hop;
    double          delay;
} enriched_reply_t;



// TODO: Kevin: these function become static once paris-traceroute.c is clean.
void map_probe_free(map_t * map);
enriched_reply_t * enriched_reply_shallow_copy(const enriched_reply_t * reply);
void vector_enriched_reply_free(vector_t * vector);

/**
 * @brief Handler for enriched output (json, xml, ... ).
 * @param loop Loop of events.
 * @param mda_event The MDA event raised.
 * @param traceroute_options Options passed to traceroute algorithm.
 * @param user_data Custom metadata/structure to produce JSON / XML outputs.
 */

void traceroute_enriched_handler(
    pt_loop_t                       * loop,
    mda_event_t                     * mda_event,
    struct algorithm_instance_s     * issuer,
    const traceroute_options_t      * traceroute_options,
    traceroute_enriched_user_data_t * user_data
);

// TODO: Kevin: add options, like for algorithms

#endif
