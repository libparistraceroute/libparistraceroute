#ifndef METAFIELD_H
#define METAFIELD_H

#include <unistd.h>
#include "bitfield.h"

/**
 * A metafield provide an abstraction of a set of bits stored in a
 * data structure. This is a convenient way to abstract a concept
 * (for example "what is a flow").
 *
 * Suppose that you want ot encode a flow_id in this metafield. It
 * consists to encode this integer among the set of bits related to
 * the metafield. However, you may desire to remain constant several
 * bits related to the concept (for example those relatad to "dst_port")
 * That is why metafield allows you to define which bit in the concept
 * are read-only. Pass NULL if you do not have read only bits.
 *
 * Once bits_concept and bits_ro are defined, you just have to map you
 * metafield to a memory address (for instance the probe that you
 * forge) and alter this probe through the metafield's functions. If
 * you have several probes to forge, you just have to make points
 * buffer to your next probe.
 */

typedef struct metafield_s {
    unsigned char * buffer;       /**< Memory managed by the metafield */
    bitfield_t    * bits_concept; /**< Bits related to the metafield */
    bitfield_t    * bits_ro;      /**< Read only bits. NULL if not used */
} metafield_t;

//--------------------------------------------------------------------------
// Allocation
//--------------------------------------------------------------------------

/**
 * \brief Allocate a metafield. You are then suppose to set
 *    - buffer (for instance the probe you forge)
 *    - bits_concept (for instance bits related to a flow)
 *    - bits_ro (for instance bits corresponding to dst_port)
 *    by yourself.
 * \return Address of the metafield if success, NULL otherwise
 */

metafield_t * metafield_create();

/**
 * \brief Delete a metafield from the memory.
 * It only releases the metafield_t instance, not the pointed structures.
 */

void metafield_free(metafield_t * metafield);

//--------------------------------------------------------------------------
// Getter / setter 
//--------------------------------------------------------------------------

/**
 * \brief Retrieve |bits_concept|, e.g. number of bits related to the
 *   concept of a metafield. These bits may be either modifable or
 *   read-only. See metafield_num_bits_rw() if you only
 *   consider alterable bits.
 * \param metafield The metafield
 * \return The number of bits
 */

size_t metafield_num_bits(const metafield_t * metafield);

/**
 * \brief Retrieve |bits_rw|, e.g. the number of bits related to the
 *   concept that may be altered.
 * \param metafield The metafield
 * \return The number of bits
 */

size_t metafield_num_bits_rw(const metafield_t * metafield);

/**
 * \brief Set a value in a metafield.
 * \warning This function may fail if you pass a value that does not
 *    fit in the metafield.
 * \param metafield The metafield on which we will iterate.
 * \param buffer_value The value that we'll set in the metafield.
 *    For instance it may be an integer and then you will pass
 *    its adress.
 * \param size_in_bits The size of buffer_value in bits.
 *    It should be less or equal to metafield_num_bits_rw(metafield)
 * \return 0 if success, another value (see <errno.h>) otherwise.
 */

int metafield_set(
    metafield_t         * metafield,
    const unsigned char * buffer_value,
    size_t                size_in_bits
);

/**
 * \brief Retrieve the value stored in a metafield.
 * \param metafield The metafield
 * \param value The pre-allocated target buffer.
 * \return 0 if success, another value (see <errno.h>) otherwise.
 */

int metafield_get(
    const metafield_t * metafield,
    unsigned char     * value
);

//--------------------------------------------------------------------------
// Iteration
//--------------------------------------------------------------------------

/**
 * \brief Set the first value in a metafield on which we will iterate.
 * \param metafield The metafield that we set.
 */

bool metafield_set_first(metafield_t * metafield);

/**
 * \brief Set the previous value in a metafield on which we iterate.
 * \param metafield The metafield that we set.
 */

bool metafield_set_previous(metafield_t * metafield);

/**
 * \brief Set the next value in a metafield on which we iterate.
 * \param metafield The metafield that we set. 
 */

bool metafield_set_next(metafield_t * metafield);

/**
 * \brief Set the last value in a metafield on which we will iterate.
 * \param metafield The metafield that we set.
 */

bool metafield_set_last(metafield_t * metafield);

#endif

