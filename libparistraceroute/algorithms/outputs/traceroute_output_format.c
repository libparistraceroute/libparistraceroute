#include "config.h"
#include "use.h"
#include "traceroute_output_format.h"

#include <stdlib.h> // NULL
#include <string.h> // strcmp



// static const char * traceroute_output_format_names[] = {
//     "default", // --format=default : default outputs.
// #ifdef USE_FORMAT_JSON
//     "json",    // --format=json    : JSON outputs.
// #endif
// #ifdef USE_FORMAT_XML
//     "xml",     // --format=xml     : XML outputs.
// #endif
//     NULL
// };

traceroute_output_format_t traceroute_output_format_from_string(const char * format_name) {
    return
#ifdef USE_FORMAT_RIPE
        !strcmp("ripe", format_name) ? TRACEROUTE_OUTPUT_FORMAT_RIPE :
#endif
#ifdef USE_FORMAT_JSON
        !strcmp("json", format_name) ? TRACEROUTE_OUTPUT_FORMAT_JSON :
#endif
#ifdef USE_FORMAT_XML
        !strcmp("xml", format_name)  ? TRACEROUTE_OUTPUT_FORMAT_XML :
#endif
        TRACEROUTE_OUTPUT_FORMAT_DEFAULT;
}
