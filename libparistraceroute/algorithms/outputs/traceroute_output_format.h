#ifndef ALGORITHMS_OUTPUTS_TRACEROUTE_OUTPUT_FORMAT_H
#define ALGORITHMS_OUTPUTS_TRACEROUTE_OUTPUT_FORMAT_H

#include "../../options.h"

/**
 * @brief Enumeration indicating how to the MDA outputs must be formatted.
 */

typedef enum traceroute_output_format_t {
    TRACEROUTE_OUTPUT_FORMAT_DEFAULT, /**< Default traceroute text outputs. */
    TRACEROUTE_OUTPUT_FORMAT_JSON,    /**< JSON outputs, based on RIPE outputs. */
    TRACEROUTE_OUTPUT_FORMAT_XML      /**< XML outputs. */
} traceroute_output_format_t;

/**
 * @return Retrieve the options related to the output format of traceroute-like tools.
 */

const option_t * traceroute_format_get_options();

/**
 * @brief Parse a string to get the corresponding traceroute_output_format_t.
 * @param format_name A string among "xml", "json", "default".
 * @returns The corresponding traceroute_output_format_t.
 */

traceroute_output_format_t traceroute_output_format_from_string(const char * format_name);

#endif

