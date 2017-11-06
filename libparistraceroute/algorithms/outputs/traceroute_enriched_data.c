#include "../../use.h"
#include "traceroute_enriched_data.h"

#include <stddef.h>                 // size_t
#include <stdlib.h>                 // malloc, free

#ifdef USE_FORMAT_JSON
#  include "json.h"                 // json_*
#endif
#if defined(USE_FORMAT_JSON) || defined(USE_FORMAT_XML)
#  include "../../int.h"
#endif

traceroute_enriched_user_data_t * traceroute_enriched_user_data_create(const char * protocol_name, const char * dst_ip, const char * format_name) {
    traceroute_enriched_user_data_t * user_data = NULL;

    if (!(user_data = malloc(sizeof(traceroute_enriched_user_data_t)))) goto ERR_MALLOC;

    user_data->format = traceroute_output_format_from_string(format_name);
    user_data->replies_by_ttl = NULL;
    user_data->is_first_result = true;
    user_data->destination = dst_ip;
    // Maximum length for an IP address (either v4 or v6). (See IP max length in string representation)
    user_data->source = malloc(MAX_SIZE_STRING_ADDRESS_REPRESENTATION); // TODO: Kévin: use appropriate constants, remove them from address.h
    user_data->protocol = protocol_name;

#if defined(USE_FORMAT_JSON) || defined(USE_FORMAT_XML)
    switch (user_data->format) {
#  ifdef USE_FORMAT_JSON
        case TRACEROUTE_OUTPUT_FORMAT_JSON:
#  endif
#  ifdef USE_FORMAT_XML
        case TRACEROUTE_OUTPUT_FORMAT_XML:
#  endif
            user_data->replies_by_ttl = map_create(uint8_dup, free, NULL, uint8_compare, vector_dup, vector_enriched_reply_free, NULL);
            user_data->stars_by_ttl   = map_create(uint8_dup, free, NULL, uint8_compare, vector_dup, free, NULL);
            break;
        default: break;
    }
#endif
ERR_MALLOC:
    return user_data;
}

void traceroute_enriched_user_data_free(traceroute_enriched_user_data_t * user_data) {
    if (user_data) {
#if defined(USE_FORMAT_JSON) || defined(USE_FORMAT_XML)
        switch (user_data->format) {
#  ifdef USE_FORMAT_JSON
            case TRACEROUTE_OUTPUT_FORMAT_JSON:
#  endif
#  ifdef USE_FORMAT_XML
            case TRACEROUTE_OUTPUT_FORMAT_XML:
#  endif
                map_probe_free(user_data->replies_by_ttl);
                map_probe_free(user_data->stars_by_ttl);
                break;
            default: break;
        }
#endif
        free(user_data);
    }
}

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
    // TODO: Kévin add issuer
    const traceroute_options_t      * traceroute_options,
    traceroute_enriched_user_data_t * user_data,
    bool                              sorted_print
) {
    FILE * f_json = stdout;

    switch (mda_event->type) {
        //-------------------------------------------------------------------
        case MDA_BEGINS:
        //-------------------------------------------------------------------
            switch (user_data->format) {
                case TRACEROUTE_OUTPUT_FORMAT_DEFAULT:
                    // TODO move print for paris-traceroute.c here.
                    break;
#ifdef USE_FORMAT_JSON
                case TRACEROUTE_OUTPUT_FORMAT_JSON:
                    {
                        /* TODO: Kévin: get skel from issuer
                        address_t source;
                        probe_extract(skel, "src_ip", &source);
                        address_to_string(&source, &user_data->source);
                        */
                        user_data->source = "NOT YET IMPLEMENTED";
                        ///
                        json_print_header(f_json, user_data->source, user_data->destination, user_data->protocol);
                    }
                    break;
#endif
#ifdef USE_FORMAT_XML
                case TRACEROUTE_OUTPUT_FORMAT_XML:
                    fprintf(stderr, "Not yet implemented\n");
                    break;
#endif
            }

            break;
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
                case TRACEROUTE_OUTPUT_FORMAT_DEFAULT:
                    // TODO: Kévin
                    /*
                    mda_data = issuer->data;
                    printf("Lattice:\n");
                    lattice_dump(mda_data->lattice, (ELEMENT_DUMP) mda_lattice_elt_dump);
                    printf("\n");
                    */
                    break;
#ifdef USE_FORMAT_XML
                case TRACEROUTE_OUTPUT_FORMAT_XML:
                    fprintf(stderr, "Not yet implemented\n");
                    break;
#endif
#ifdef USE_FORMAT_JSON
                case TRACEROUTE_OUTPUT_FORMAT_JSON:
                    {
                        map_t * replies_by_ttl = user_data->replies_by_ttl,
                              * stars_by_ttl   = user_data->stars_by_ttl;
                        size_t max_ttl_hops    = map_size(user_data->replies_by_ttl);
                        size_t max_ttl_stars   = map_size(user_data->stars_by_ttl);
                        size_t n = max_ttl_hops > max_ttl_stars ? max_ttl_hops : max_ttl_stars;

                        if (sorted_print && n) {
                            for (size_t i = 1; i < n; ++i) {

                                bool is_first_result = true;

                                // Print each reply at TTL = i
                                vector_t * replies_by_ttl_i = NULL;
                                if (map_find(replies_by_ttl, &i, &replies_by_ttl_i)) {
                                    for (size_t j = 0; j < max_ttl_hops; ++j) {
                                        if (is_first_result) {
                                            fprintf(f_json, ",\n");
                                            is_first_result = false;
                                        }
                                        reply_to_json(
                                            vector_get_ith_element(replies_by_ttl_i, j),
                                            f_json
                                        );
                                    }
                                }

                                // Print each star at TTL = i
                                vector_t * stars_by_ttl_i = NULL;
                                if (map_find(stars_by_ttl, &i, &stars_by_ttl_i)) {
                                    for (size_t j = 0; j < max_ttl_hops; ++j) {
                                        if (is_first_result) {
                                            fprintf(f_json, ",\n");
                                            is_first_result = false;
                                        }
                                        star_to_json(
                                            vector_get_ith_element(stars_by_ttl_i, j),
                                            f_json
                                        );
                                    }
                                }
                            }
                        }
                    }
                    json_print_footer(f_json);
                    break;
#endif
                default:
                    break;
            }
        case MDA_NEW_LINK:
            // TODO Kévin
            // mda_link_dump(mda_event->data, traceroute_options->do_resolv);
            printf("TODO: Fix mda_link_dump\n");
            break;
        default:
            printf("traceroute_enriched_handler: Unhandled event %d\n", mda_event->type);
            break;
    }
}


