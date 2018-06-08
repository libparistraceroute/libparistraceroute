#ifndef FILTER_H
#define FILTER_H

/**
 * \file filter.h
 * \brief Header related to filters.
 *
 * A filter is a list of string corresponding. Each string
 * is typically "protocol.fieldname" (e.g. "ipv4.src_ip").
 * See libparistraceroute/protocols to see available protocols
 * and the corresponding provided field names.
 *
 * Filters can be used to define metafields (see "metafield.h").
 * See examples in "metafield.c".
 *
 * NOTE:
 * The syntax could be extended (e.g. "ip*.*_ip" could match
 * "ipv4.src_ip", "ipv4.dst_ip", "ipv6.src_ip", "ipv6.dst_ip").
 * Currently wildcards / regexp are not supported.
 */

#include <stdbool.h>            // bool
#include <stddef.h>             // size_t
#include <stdint.h>             // uint*_t
#include <stdio.h>              // FILE *

#include "containers/list.h"    // list_t
#include "layer.h"              // layer_t
#include "probe.h"              // probe_t
#include "protocol_field.h"     // protocol_field_t

typedef list_t filter_t;

/**
 * \brief Create a filter_t instance.
 * \param expr1 Describe the "protocol.name" expressions describing
 *   the filters to consider. Example: "ipv4.src_ip".
 * \return The address of the filter_t if successful, NULL otherwise.
 */

filter_t * filter_create(const char * expr1, ...);

/**
 * \brief Free a filter_t instance.
 * \param filter A filter_t intance.
 */

void filter_free(filter_t * filter);

/**
 * \brief Iterate over a probe according to a filter_t.
 * \param filter A filter_t instance.
 * \param probe A probe_t instance.
 * \param callback A function called back whenever a matching
 *    protocol.fieldname is matching.
 * \param user_data A pointer passed whenever callback is called.
 */

bool filter_iter(
    const filter_t * filter,
    const probe_t  * probe,
    bool (*callback)(
        const filter_t         * filter,
        const probe_t          * probe,
        const layer_t          * layer,
        const protocol_field_t * protocol_field,
        void                   * user_data
   ),
   void * user_data
);

/**
 * \brief Print a filter_t in the standard output.
 * \param filter A filter_t instance.
 */

void filter_dump(const filter_t * filter);

/**
 * \brief Print a filter_t in an output file.
 * \param out The output file.
 * \param filter A filter_t instance.
 */

void filter_fprintf(FILE * out, const filter_t * filter);

/**
 * \brief Compute the number of bits involved in a probe matching the whole
 *    filter.
 * \param filter A filter_t instance.
 * \param probe A probe_t instance.
 * \return The size in bits (if the filter matches the probe), 0 otherwise.
 */

size_t filter_get_matching_size_in_bits(const filter_t * filter, const probe_t * probe);

/**
 * \brief Tests whether all the expressions involved in a filter_t appears
 *   in a probe_t.
 * \param filter_t A filter_t instance.
 * \param probe A probe_t instance.
 * \return true iff the filter matches the probe.
 */

bool filter_matches(const filter_t * filter, const probe_t * probe);

/**
 * \brief Read from a packet wrapped in a probe_t the bits corresponding to
 *    a filter_t instance.
 * \param filter_t A filter_t instance.
 * \param probe A probe_t instance.
 * \param output_buffer The pre-allocated buffer where the bits from the
 *    packet read are written.
 * \param num_bits The number of bits available in output_buffer.
 * \return true iff successful.
 */

bool filter_read(const filter_t * filter, const probe_t * probe, uint8_t * output_buffer, size_t num_bits);

/**
 * \brief Update a packet wrapped in a probe_t instance according to a
 *    filter_t instance and an input buffer.
 * \param filter_t A filter_t instance.
 * \param probe A probe_t instance.
 * \param input_buffer The buffer where the bits to write into the packet are read.
 * \param num_bits The number of bits to write.
 * \return true iff successful.
 */

bool filter_write(const filter_t * filter, const probe_t * probe, const uint8_t * input_buffer, size_t num_bits);

#endif

