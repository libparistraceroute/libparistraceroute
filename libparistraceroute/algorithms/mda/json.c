
#include <stdlib.h>
#include <netinet/in.h> // INET_ADDRSTRLEN, INET6_ADDRSTRLEN

#include "json.h"
#include "../traceroute.h"

#ifdef USE_FORMAT_JSON

#define FORMAT_JSON_TYPE     "\"type\""
#define FORMAT_JSON_HOP      "\"hop\""
#define FORMAT_JSON_RESULT   "\"result\""
#define FORMAT_JSON_FROM     "\"from\""
#define FORMAT_JSON_TO       "\"to\""
#define FORMAT_JSON_PROTOCOL "\"protocol\""
#define FORMAT_JSON_SRC_PORT "\"src_port\""
#define FORMAT_JSON_DST_PORT "\"dst_port\""
#define FORMAT_JSON_FLOW_ID  "\"flow_id\""
#define FORMAT_JSON_TTL      "\"ttl\""
#define FORMAT_JSON_RTT      "\"rtt\""

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
        vector_t * replies_by_ttl_i = NULL;
        if (map_find(map, &i, &replies_by_ttl_i)) {
            // Only free the vector, not the probes.
            free(replies_by_ttl_i);
        }
    }
}

// Move to enriched handler?
void json_handler(
    mda_event_type_t   mda_event_type,
    probe_t          * probe,
    enriched_reply_t * enriched_reply,
    user_data_t      * user_data,
    bool               sorted_print
){
    // Elements can be either replies or stars
    map_t * elements_by_ttl = NULL;
    uint8_t ttl_probe       = 0;
    const char * json_key   = NULL;
    FILE * f_json           = stdout;

    switch (mda_event_type) {
        case MDA_PROBE_REPLY:
            elements_by_ttl = user_data->replies_by_ttl;
            json_key        = "results";
            break;
        case MDA_PROBE_TIMEOUT:
            elements_by_ttl = user_data->stars_by_ttl;
            json_key        = "stars";
            break;
        default:
            break;
    }

    if (sorted_print) {
        if (probe_extract(probe, "ttl", &ttl_probe)) {
            if (user_data->is_first_probe_star) {
                // Fill the source field of MDA metadata
                probe_extract(probe, "src_ip", &user_data->source);
                user_data->is_first_probe_star = false;
            }

            // Check if we already have this TTL in the map.
            vector_t * elements_ttl;
            if (map_find(elements_by_ttl, &ttl_probe, &elements_ttl)) {
                // We already have the element, just push this new reply related to this probe.
                if (mda_event_type == MDA_PROBE_REPLY){
                    vector_push_element(elements_ttl, enriched_reply);
                } else if (mda_event_type == MDA_PROBE_TIMEOUT){
                    vector_push_element(elements_ttl, probe);
                }
            } else {
                if (mda_event_type == MDA_PROBE_REPLY){
                    elements_ttl = vector_create(sizeof(enriched_reply_t), enriched_reply_shallow_copy, free, NULL);
                    vector_push_element(elements_ttl, enriched_reply);
                } else if (mda_event_type == MDA_PROBE_TIMEOUT){
                    elements_ttl = vector_create(sizeof(probe_t), probe_dup, probe_free, NULL);
                    vector_push_element(elements_ttl, probe);
                }

                // map_update duplicates the value and the key passed in arg,
                // so free the vector after the call to map_update
                map_update(elements_by_ttl, &ttl_probe, elements_ttl);
                free(elements_ttl); // TODO: vector_free(elements_ttl);
            }
        }
    } else {
        if (user_data->is_first_probe_star) {
            // TODO: ALGORITHM_INIT
            char * src_ip_address = NULL;
            address_to_string(&user_data->source, &src_ip_address);

            // mda_infos_dump
            fprintf(
                f_json,
                "{\n"
                "  %-10s : \"%s\",\n"
                "  %-10s : \"%s\",\n"
                "  %-10s : \"%s\",\n"
                "  \"%s\" : [\n",
                FORMAT_JSON_FROM,     src_ip_address,
                FORMAT_JSON_TO,       user_data->destination,
                FORMAT_JSON_PROTOCOL, user_data->protocol,

            );
            fflush(f_json);

            if (src_ip_address) free(src_ip_address);

            user_data->is_first_probe_star = false;
        } else {
            fprintf(f_json, ", ");
        }

        if (mda_event_type == MDA_PROBE_REPLY){
            reply_to_json(enriched_reply, f_json);
        } else if (mda_event_type == MDA_PROBE_TIMEOUT){
            star_to_json(probe, f_json);
        }

        fflush(f_json);
    }
}

/**
 * @brief Handler for enriched output (json, xml, ... ).
 * @param loop loop of events.
 * @param mda_event event raised
 * @param traceroute_options options passed to traceroute command
 * @param user_data custom metadata/structure to compute json /xml serialization
 */

void traceroute_enriched_handler(
    pt_loop_t                  * loop,
    mda_event_t                * mda_event,
    const traceroute_options_t * traceroute_options,
    user_data_t                * user_data,
    bool                         sorted_print
) {
    probe_t * probe;
    probe_t * reply;

    switch (mda_event->type) {
        case MDA_PROBE_REPLY:
            // Retrieve the probe and its corresponding reply
            probe = ((const probe_reply_t *) mda_event->data)->probe;
            reply = ((const probe_reply_t *) mda_event->data)->reply;

            // Managed by the container that uses it.
            enriched_reply_t * enriched_reply = malloc(sizeof(enriched_reply_t));
            enriched_reply->reply = reply;
            enriched_reply->delay = delay_probe_reply(probe, reply);

            switch (user_data->format) {
                case FORMAT_JSON:
                    json_handler(MDA_PROBE_REPLY, probe, enriched_reply, user_data, sorted_print);
                    break;
                case FORMAT_XML:
                    fprintf(stderr, "Not yet implemented\n");
                    break;
                case FORMAT_DEFAULT: break;
            }

            free(enriched_reply);
            break;

        case MDA_PROBE_TIMEOUT:
            probe = (probe_t *) mda_event->data;

            switch (user_data->format) {
                case FORMAT_JSON:
                    json_handler(MDA_PROBE_TIMEOUT, probe, NULL, user_data, sorted_print);
                    break;
                case FORMAT_XML:
                    fprintf(stderr, "Not yet implemented\n");
                    break;
                case FORMAT_DEFAULT: break;
            }
            break;
        default:
            break;
    }
}

/**
 * @brief Print a reply to JSON format.
 * @param enriched_reply The MDA reply with the information for the JSON output.
 * @param f_json A valid file descriptor used to write the reply.
 */

void reply_to_json(const enriched_reply_t * enriched_reply, FILE * f_json){
    address_t reply_from_address;
    uint16_t  src_port  = 0,
              dst_port  = 0,
              flow_id   = 0;
    uint8_t   ttl_reply = 0;
    probe_t * reply     = enriched_reply->reply;
    char *    addr_str  = NULL;

    probe_extract(reply, "src_ip",   &reply_from_address);
    probe_extract(reply, "src_port", &src_port);
    probe_extract(reply, "dst_port", &dst_port);
    probe_extract(reply, "flow_id",  &flow_id);
    probe_extract(reply, "ttl",      &ttl_reply);
    address_to_string(&reply_from_address, &addr_str);

    fprintf(
        f_json,
        "  {\n"
        "    %-10s : \"%s\",\n"
        "    %-10s : \"%s\",\n"
        "    %-10s : %hu,\n"
        "    %-10s : %hu,\n"
        "    %-10s : %hu,\n"
        "    %-10s : %hhu,\n"
        "    %-10s : %5.3lf\n"
        "  }",
        FORMAT_JSON_TYPE,     "reply",
        FORMAT_JSON_FROM,     addr_str ? addr_str : "*",
        FORMAT_JSON_SRC_PORT, src_port,
        FORMAT_JSON_DST_PORT, dst_port,
        FORMAT_JSON_FLOW_ID,  flow_id,
        FORMAT_JSON_TTL,      ttl_reply,
        FORMAT_JSON_RTT,      enriched_reply->delay
    );

    if (addr_str) free(addr_str);
}

/**
 * @brief Print the output of MDA to JSON.
 * @param replies_by_ttl A map which associates for each TTL the corresponding replies.
 * @param f_json The output file descriptor (e.g. stdout).
 */

void replies_to_json(const map_t * replies_by_ttl, FILE * f_json) {
    fprintf(f_json, "\"results\" : [");
    for (size_t i = 1; i < options_traceroute_get_max_ttl(); ++i) {
        vector_t * replies_by_ttl_i = NULL;

        if (map_find(replies_by_ttl, &i, &replies_by_ttl_i)) {
            fprintf(f_json, "{\"hop\":");
            fprintf(f_json, "%zu,", i);
            fprintf(f_json, "\"result\":[");
            for (size_t j = 0; j < replies_by_ttl_i->num_cells; ++j) {

                enriched_reply_t * enriched_reply = vector_get_ith_element(replies_by_ttl_i, j);
                reply_to_json(enriched_reply, f_json);

                // If it is not the last reply, append a ','
                if (j != replies_by_ttl_i->num_cells - 1) {
                    fprintf(f_json, ",\n");
                }
            }
            fprintf(f_json, "]}");
            if (i < map_size(replies_by_ttl)) {
                fprintf(f_json, ",");
            } else {
                // Ensure that the result is flushed, even in debug mode.
                fprintf(f_json, "]\n");
                break;
            }
        }
    }
}

void star_to_json(const probe_t * star, FILE * f_json) {
    uint8_t  ttl = 0;
    uint16_t src_port = 0;
    uint16_t dst_port = 0;
    uint16_t flow_id  = 0;

    probe_extract(star, "ttl",      &ttl);
    probe_extract(star, "src_port", &src_port);
    probe_extract(star, "dst_port", &dst_port);
    probe_extract(star, "flow_id",  &flow_id);

    fprintf(
        f_json,
        "  {\n"
        "    %-10s : \"%s\",\n"
        "    %-10s : %hhu,\n"
        "    %-10s : %hu,\n"
        "    %-10s : %hu,\n"
        "    %-10s : %hu\n"
        "  }",
        FORMAT_JSON_TYPE,     "stars",
        FORMAT_JSON_TTL,      ttl,
        FORMAT_JSON_SRC_PORT, src_port,
        FORMAT_JSON_DST_PORT, dst_port,
        FORMAT_JSON_FLOW_ID,  flow_id
    );
}

void stars_to_json(const map_t * stars_by_ttl, FILE * f_json) {
    // Print the dictionnaries related to stars.
    fprintf(f_json, "\"stars\" : [");
    for (size_t i = 1; i < options_traceroute_get_max_ttl(); ++i) {
        vector_t * starts_by_hop_i = NULL;

        if (map_find(stars_by_ttl, &i, &starts_by_hop_i)) {
            fprintf(
                f_json,
                "{"
                "  %-10s : %zu,"
                "  %-10s : [\n",
                FORMAT_JSON_HOP, i,
                FORMAT_JSON_RESULT
            );

            for (size_t j = 0; j < starts_by_hop_i->num_cells; ++j) {
                probe_t * star = vector_get_ith_element(starts_by_hop_i, j);
                star_to_json(star, f_json);

                // Check if its the last response for this hop.
                if (j != starts_by_hop_i->num_cells - 1) {
                    fprintf(f_json, ", ");
                }
            }

            fprintf(f_json, "  ]\n}");

            if (i < map_size(stars_by_ttl)) {
                fprintf(f_json, ",");
            } else {
                //Ensure that the result is flushed, even in debug mode.
                fprintf(f_json,"]\n");
                break;
            }
        }
    }
}

/*
void mda_infos_to_json(const user_data_t * user_data, FILE * f_json){
    char * src_ip_address = NULL;
    address_to_string(&user_data->source, &src_ip_address);

    fprintf(f_json, "  %-10s :\"%s\",\n", "\"from\"",     src_ip_address);
    fprintf(f_json, "  %-10s :\"%s\",\n", "\"to\"",       user_data->destination);
    fprintf(f_json, "  %-10s :\"%s\",\n", "\"protocol\"", user_data->protocol);

    if (src_ip_address) free(src_ip_address);
}
*/

void replies_to_json_dump(const map_t * replies_by_ttl) {
    replies_to_json(replies_by_ttl, stdout);
    fflush(stdout);
}

void stars_to_json_dump(const map_t * stars_by_ttl){
    stars_to_json(stars_by_ttl, stdout);
    fflush(stdout);
}

/*
void mda_infos_dump(const user_data_t * user_data){
    mda_infos_to_json(user_data, stdout);
    fflush(stdout);
}
*/

#endif // USE_FORMAT_JSON
