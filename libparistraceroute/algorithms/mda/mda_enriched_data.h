#ifndef ALGORITHMS_MDA_MDA_ENRICHED_DATA_H
#define ALGORITHMS_MDA_MDA_ENRICHED_DATA_H
#include "address.h"
#include "containers/map.h"

//---------------------------------------------------------------------------
// User data (passed from main() to *_handler() functions.
//---------------------------------------------------------------------------

/**
 * @brief Structure used to pass some user data to loop handler.
 */
typedef enum mda_output_format_t {
    FORMAT_DEFAULT,
    FORMAT_JSON,
    FORMAT_XML
} mda_output_format_t;

const char * format_names[] = {
    "default", // default value
    "json",
    "xml",
    NULL
};

typedef struct {
    mda_output_format_t     format;                /**< FORMAT_XML|FORMAT_JSON|FORMAT_DEFAULT */
    map_t                 * replies_by_ttl;
    map_t                 * stars_by_ttl;
    bool                    is_first_probe_star;   /**< Akward variable for first probe or star recieved for json compliance. */
    address_t               source;
    const char            * destination;
    const char            * protocol;
} user_data_t; 


#endif 
