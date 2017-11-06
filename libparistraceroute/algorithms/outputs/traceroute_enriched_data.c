#include "../../use.h"
#include "traceroute_enriched_data.h"

#include <stddef.h>                 // size_t
#include <stdlib.h>                 // malloc, free

#ifdef USE_FORMAT_JSON
#  include "json.h"                 // json_*
#endif

enriched_reply_t * enriched_reply_shallow_copy(const enriched_reply_t * reply) {
    enriched_reply_t * reply_dup = malloc(sizeof(enriched_reply_t));
    reply_dup->reply = reply->reply;
    reply_dup->delay = reply->delay;
    return reply_dup;
}

void vector_enriched_reply_free(vector_t * vector) {
    for (size_t i = 0; i < vector->num_cells; ++i) {
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

void traceroute_enriched_handler(
    pt_loop_t                       * loop,
    mda_event_t                     * mda_event,
    const traceroute_options_t      * traceroute_options,
    traceroute_enriched_user_data_t * user_data,
    bool                              sorted_print
) {
    FILE * f_json = stdout;

    switch (mda_event->type) {
        //-------------------------------------------------------------------
        case MDA_PROBE_REPLY:
        //-------------------------------------------------------------------
            {
                // Retrieve the probe and its corresponding reply
                const probe_t * probe = ((const probe_reply_t *) mda_event->data)->probe;
                const probe_t * reply = ((const probe_reply_t *) mda_event->data)->reply;

                // Managed by the container that uses it.
                enriched_reply_t * enriched_reply = malloc(sizeof(enriched_reply_t));
                enriched_reply->reply = reply;
                enriched_reply->delay = delay_probe_reply(probe, reply);

                switch (user_data->format) {
                    case TRACEROUTE_OUTPUT_FORMAT_DEFAULT:
                        break;
#ifdef USE_FORMAT_JSON
                    case TRACEROUTE_OUTPUT_FORMAT_JSON:
                        if (user_data->is_first_result) {
                            user_data->is_first_result = false;
                        } else {
                            fprintf(f_json, ", ");
                        }
                        reply_to_json(enriched_reply, f_json);
                        break;
#endif
#ifdef USE_FORMAT_XML
                    case TRACEROUTE_OUTPUT_FORMAT_XML:
                        fprintf(stderr, "Not yet implemented\n");
                        break;
#endif
                }

                free(enriched_reply);
            }
            break;

        //-------------------------------------------------------------------
        case MDA_PROBE_TIMEOUT:
        //-------------------------------------------------------------------
            {
                const probe_t * probe = (probe_t *) mda_event->data;

                switch (user_data->format) {
                    case TRACEROUTE_OUTPUT_FORMAT_DEFAULT:
                        break;
#ifdef USE_FORMAT_JSON
                    case TRACEROUTE_OUTPUT_FORMAT_JSON:
                        if (user_data->is_first_result) {
                            user_data->is_first_result = false;
                        } else {
                            fprintf(f_json, ", ");
                        }
                        star_to_json(probe, f_json);
                        break;
#endif
#ifdef USE_FORMAT_XML
                    case TRACEROUTE_OUTPUT_FORMAT_XML:
                        fprintf(stderr, "Not yet implemented\n");
                        break;
#endif
                }
            }
            break;

        //-------------------------------------------------------------------
        case MDA_ENDS:
        //-------------------------------------------------------------------

            switch (user_data->format){
#ifdef USE_FORMAT_XML
                case TRACEROUTE_OUTPUT_FORMAT_XML:
                    fprintf(stderr, "Not yet implemented\n");
                    break;
#endif
#ifdef USE_FORMAT_JSON
                case TRACEROUTE_OUTPUT_FORMAT_JSON:
                    {
                        size_t max_ttl_hops  = map_size(user_data->replies_by_ttl);
                        size_t max_ttl_stars = map_size(user_data->replies_by_ttl);
                        size_t n = max_ttl_hops > max_ttl_hops ? max_ttl_hops : max_ttl_hops;

                        if (sorted_print && n) {
                            for (size_t i = 1; i < options_traceroute_get_max_ttl(); ++i) {
                                // Print each reply at TTL = i
                                for (size_t j = 0; j < max_ttl_hops; ++j) {

                                    enriched_reply_t * enriched_reply = vector_get_ith_element(replies_by_ttl_i, j);
                                    reply_to_json(enriched_reply, f_json);

                                    // If it is not the last reply, append a ','
                                    if (j != replies_by_ttl_i->num_cells - 1) {
                                        fprintf(f_json, ",\n");
                                    }
                                }

                                // Print each star at TTL = i
                            }
                        }
                    }
                    json_print_footer(f_json);
                    break;
#endif
                default:
                    break;
            }
        default:
            printf("traceroute_enriched_handler: Unhandled event %d\n", mda_event->type);
            break;
    }
}


