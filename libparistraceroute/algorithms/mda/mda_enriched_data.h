#ifndef ALGORITHMS_MDA_MDA_ENRICHED_DATA_H
#define ALGORITHMS_MDA_MDA_ENRICHED_DATA_H
#include "address.h"
#include "containers/map.h"

//---------------------------------------------------------------------------
// User data (passed from main() to *_handler() functions.
//---------------------------------------------------------------------------

/**
 * @brief Enumeration indicating how to the MDA outputs must be formatted.
 */

typedef enum mda_output_format_t {
    FORMAT_DEFAULT, /**< Default traceroute text outputs. */
    FORMAT_JSON,    /**< JSON outputs, based on RIPE outputs. */
    FORMAT_XML      /**< XML outputs. */
} mda_output_format_t;

/**
 * @brief Format option passed to MDA application corresponding.
 * @sa mda_output_format_t enumeration.
 */

const char * format_names[] = {
    "default", // --format=default : default outputs.
    "json",    // --format=json    : JSON outputs.
    "xml",     // --format=xml     : XML outputs.
    NULL
};

/**
 * The following data structure is passed to json_handler() and
 * xml_handler() to produced extended MDA ouputs.
 * @sa mda/json.h
 * @sa mda/xml.h
 */

// TODO: Kevin: rename this data structure, user_data is a generic name. Here it should be mda_enriched_user_data_t.

typedef struct {
    mda_output_format_t     format;                /**< FORMAT_XML|FORMAT_JSON|FORMAT_DEFAULT */
    map_t                 * replies_by_ttl;        /**< Maps each TTL with the corresponding vector of replies. */
    map_t                 * stars_by_ttl;          /**< Maps each TTL with the corresponding vector of stars. */
    bool                    is_first_result;       /**< Akward variable for first probe or star recieved for json compliance. */
    // TODO: Kevin: the following fields seems to be unused?
    //address_t               source;                /**< Source address. */
    const char            * source;                /**< Source address. */
    const char            * destination;           /**< MDA target address. */
    const char            * protocol;              /**< Protocol. */
} user_data_t;

// TODO: Kevin: add options, like for algorithms

#endif
