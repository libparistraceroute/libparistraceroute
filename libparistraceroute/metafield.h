#ifndef METAFIELD_H
#define METAFIELD_H

#include <stdbool.h>            // bool
#include <stddef.h>             // size_t
#include <stdint.h>             // uint*_t
#include <stdio.h>              // FILE *

#include "containers/list.h"    // list_t
#include "probe.h"              // probe_t
#include "filter.h"             // filter_t

typedef struct metafield_s {
    const char * name;    /**< Reference to the metafield's name. */
    list_t     * filters; /**< Fields involved in the metafield. list<filter_t *>. */
} metafield_t;

/**
 * \brief Create an empty metafield_t instance.
 * \param The name of the metafield_t (e.g. "flow_id").
 * \return The address of the metafield_t if successful, NULL otherwise.
 */

metafield_t * metafield_create(const char * name);

/**
 * \brief Free a metafield_t instance from the memory.
 * \param metafield A metafield_t instance.
 */

void metafield_free(metafield_t * metafield);

/**
 * \brief Print a metafield_t instance in an output file.
 * \param out The output file.
 * \param metafield A metafield_t instance.
 */

void metafield_fprintf(FILE * out, const metafield_t * metafield);

/**
 * \brief Print a metafield_t instance in the standard output.
 * \param metafield A metafield_t instance.
 */

void metafield_dump(const metafield_t * metafield);

/**
 * \brief Add a filter_t instance in a metafield_t instance.
 * \param metafield A metafield_t instance.
 * \param filter A filter_t instance.
 * \return true iff successful.
 */

bool metafield_add_filter(metafield_t * metafield, filter_t * filter);

/**
 * \brief Find the first filter_t instance involved in a metafield matching
 *    a probe_t.
 * \param metafield A metafield_t instance.
 * \param probe A probe_t instance.
 * \return The address of the matching filter_t (if any), NULL otherwise.
 */

filter_t * metafield_find_filter(const metafield_t * metafield, const probe_t * probe);

/**
 * \brief Find the first filter_t instance involved in a metafield matching
 *    a probe_t and returns the number of matching bits.
 * \param metafield A metafield_t instance.
 * \param probe A probe_t instance.
 * \return The number of matching bits (if any filter matches the probe),
 *   0 otherwise.
 */

size_t metafield_get_matching_size_in_bits(const metafield_t * metafield, const probe_t * probe);

/**
 * \brief Read from a packet wrapped in a probe_t the bits corresponding to
 *   the first matching filter involved in a metafield_t instance.
 * \param metafield_t A metafield_t instance.
 * \param probe A probe_t instance.
 * \param buffer The pre-allocated buffer where the bits from the packet read
 *   are written.
 * \param num_bits The number of bits available in output_buffer.
 * \return true iff successful.
 */

bool metafield_read(const metafield_t * metafield, const probe_t * probe, uint8_t * buffer, size_t num_bits);

/**
 * \brief Update a packet wrapped in a probe_t instance according to the first
 *   matching filter_t instance and an input buffer.
 * \param filter_t A filter_t instance.
 * \param probe A probe_t instance.
 * \param input_buffer The buffer where the bits to write into the packet
 *   are read.
 * \param num_bits The number of bits to write.
 * \return true iff successful.
 */

bool metafield_write(const metafield_t * metafield, const probe_t * probe, uint8_t * buffer, size_t num_bits);

//---------------------------------------------------------------------------
// Example: flow_id
//---------------------------------------------------------------------------

/**
 * \brief Create an empty metafield_t instance.
 * \param The name of the metafield_t (e.g. "flow_id").
 * \return The address of the metafield_t if successful, NULL otherwise.
 */

metafield_t * metafield_make_flow_id();

#endif

