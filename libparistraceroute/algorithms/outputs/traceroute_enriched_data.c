#include "../../use.h"
#include "traceroute_enriched_data.h"

#include <stddef.h>                 // size_t
#include <stdlib.h>                 // malloc, free
#include <string.h>                 // strcmp


#include "algorithm.h"

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

#if defined(USE_FORMAT_JSON) || defined(USE_FORMAT_XML) || defined(USE_FORMAT_RIPE)
    switch (user_data->format) {
#  ifdef USE_FORMAT_RIPE
        case TRACEROUTE_OUTPUT_FORMAT_RIPE:
#  endif        
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
#  ifdef USE_FORMAT_RIPE
            case TRACEROUTE_OUTPUT_FORMAT_RIPE:
#  endif  
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


void traceroute_sorted_handler(
    mda_event_type_t                       mda_event_type,
    probe_t                              * probe,
    enriched_reply_t                     * enriched_reply,
    traceroute_enriched_user_data_t      * user_data
){
    // Elements can be either replies or stars
    map_t * elements_by_ttl = NULL;
    uint8_t ttl_probe       = 0;
    
    switch (mda_event_type) {
        case MDA_PROBE_REPLY:
            elements_by_ttl = user_data->replies_by_ttl;
            //json_key        = "results";
            break;
        case MDA_PROBE_TIMEOUT:
            elements_by_ttl = user_data->stars_by_ttl;
            //json_key        = "stars";
            break;
        default:
            break;
    }

    if (probe_extract(probe, "ttl", &ttl_probe)) {
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
}

void traceroute_enriched_handler(
    pt_loop_t                       * loop,
    mda_event_t                     * mda_event,
    struct algorithm_instance_s     * issuer,
    const traceroute_options_t      * traceroute_options,
    traceroute_enriched_user_data_t * user_data
) {
    FILE * f_json = stdout;
    mda_data_t * mda_data;
    switch (mda_event->type) {
        //-------------------------------------------------------------------
        case MDA_BEGINS:;
        //-------------------------------------------------------------------
            probe_t * skel = mda_event->data;
            switch (user_data->format) {
                case TRACEROUTE_OUTPUT_FORMAT_DEFAULT:
                    // TODO move print for paris-traceroute.c here.
                   
            
                    break;
#ifdef USE_FORMAT_RIPE
                case TRACEROUTE_OUTPUT_FORMAT_RIPE:                    
                    {
                        address_t source;
                        probe_extract(skel, "src_ip", &source);
                        address_to_string(&source, &user_data->source);
                        json_print_header(f_json, user_data->source, user_data->destination, user_data->protocol);
                    }
                    break;
#endif
#ifdef USE_FORMAT_JSON
                case TRACEROUTE_OUTPUT_FORMAT_JSON:                    
                    {
                        address_t source;
                        probe_extract(skel, "src_ip", &source);
                        address_to_string(&source, &user_data->source);
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
                // Not const probe for sorted handler
                probe_t * probe = ((const probe_reply_t *) mda_event->data)->probe;
                const probe_t * reply = ((const probe_reply_t *) mda_event->data)->reply;

                // Managed by the container that uses it.
                enriched_reply_t * enriched_reply = malloc(sizeof(enriched_reply_t));
                enriched_reply->reply = reply;
                enriched_reply->delay = delay_probe_reply(probe, reply);
                // Get the TTL related to the reply
                probe_extract(probe, "ttl", &enriched_reply->hop);
                switch (user_data->format) {
                    case TRACEROUTE_OUTPUT_FORMAT_DEFAULT:
                        break;
#ifdef USE_FORMAT_RIPE
                    case TRACEROUTE_OUTPUT_FORMAT_RIPE:
                        traceroute_sorted_handler(mda_event->type, probe, enriched_reply, user_data);
                        break;
#endif                        
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
                probe_t * probe = (probe_t *) mda_event->data;

                switch (user_data->format) {
                    case TRACEROUTE_OUTPUT_FORMAT_DEFAULT:
                        break;
#ifdef USE_FORMAT_RIPE
                    case TRACEROUTE_OUTPUT_FORMAT_RIPE:
                        traceroute_sorted_handler(mda_event->type, probe, NULL, user_data);
                        break;
#endif                        
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
                    mda_data = issuer->data;
                    printf("Lattice:\n");
                    lattice_dump(mda_data->lattice, (ELEMENT_DUMP) mda_lattice_elt_dump);
                    printf("\n");
                    break;
#ifdef USE_FORMAT_RIPE
                case TRACEROUTE_OUTPUT_FORMAT_RIPE:
                    {
                        map_t * replies_by_ttl = user_data->replies_by_ttl,
                              * stars_by_ttl   = user_data->stars_by_ttl;
                        size_t n               = traceroute_options->max_ttl;
                        if (n) {
                            bool is_first_hop = true;
                            for (size_t i = 1; i < n; ++i) {
                                bool is_first_result = true;
                                // Print each reply at TTL = i
                                vector_t * replies_by_ttl_i = NULL;
                                bool found_reply = map_find(replies_by_ttl, &i, &replies_by_ttl_i);
                                // Print each star at TTL = i
                                vector_t * stars_by_ttl_i = NULL;
                                bool found_star  = map_find(stars_by_ttl, &i, &stars_by_ttl_i);
                                
                                if(found_reply || found_star){
                                    if(is_first_hop){
                                        is_first_hop = false;
                                    }
                                    else{
                                        fprintf(f_json, ",\n");
                                    }
                                    fprintf(f_json,
                                        "{\n"             
                                        "\"hop\":%zu,\n"
                                        "\"result\":[\n"
                                    ,i);
                                }
                                
                                if (found_reply) {
                                    for (size_t j = 0; j < vector_get_num_cells(replies_by_ttl_i); ++j) {
                                        if (is_first_result) {
                                            is_first_result = false;
                                        } else {
                                            fprintf(f_json, ",\n");
                                        }
                                        reply_to_json(
                                            vector_get_ith_element(replies_by_ttl_i, j),
                                            f_json
                                        );
                                    }
                                }
                                
                                if (found_star) {
                                    for (size_t j = 0; j < vector_get_num_cells(stars_by_ttl_i); ++j) {
                                        if (is_first_result) {
                                            is_first_result = false;
                                        } else {
                                            fprintf(f_json, ",\n");
                                        }
                                        star_to_json(
                                            vector_get_ith_element(stars_by_ttl_i, j),
                                            f_json
                                        );
                                    }
                                }
                                if(found_reply || found_star){
                                    fprintf(f_json,
                                            "]\n"
                                            "}");
                                }
                            }
                            
                        }

                    }
                    json_print_footer(f_json);
                    break;
#endif                     
#ifdef USE_FORMAT_XML
                case TRACEROUTE_OUTPUT_FORMAT_XML:
                    fprintf(stderr, "Not yet implemented\n");
                    break;
#endif
#ifdef USE_FORMAT_JSON
                case TRACEROUTE_OUTPUT_FORMAT_JSON:
                    json_print_footer(f_json);
                    break;
#endif
                default:
                    break;
            }
        break;
        case MDA_NEW_LINK:
            switch (user_data->format){
                case TRACEROUTE_OUTPUT_FORMAT_DEFAULT:
                    
                    mda_link_dump(mda_event->data, traceroute_options->do_resolv);
                    break;
#ifdef USE_FORMAT_RIPE
                case TRACEROUTE_OUTPUT_FORMAT_RIPE:
                    
#endif                    
#ifdef USE_FORMAT_JSON                    
                case TRACEROUTE_OUTPUT_FORMAT_JSON:
#endif                
#ifdef USE_FORMAT_XML                
                case TRACEROUTE_OUTPUT_FORMAT_XML:
#endif                
                default:    
                    break;
            }        
            break;
        default:
            printf("traceroute_enriched_handler: Unhandled event %d\n", mda_event->type);
            break;
    }
}


