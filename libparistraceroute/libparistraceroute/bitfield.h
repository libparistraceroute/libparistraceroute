#ifndef BITFIELD_H
#define BITFIELD_H

#include <unistd.h>
#include <stdbool.h>

// #include "bitfield.h"
// size, offset -> bitfield découpé de taille manipulable
// set flow id -> modif champs
// plage de flots -> iterator
//
// bitfield associés à un flot kind of module
// - flot au sens udp/ip, tcp/ip, icmp...
//

typedef struct bitfield_s {
    unsigned char * mask;         /**< A buffer that stores a bitfield */
    size_t          size_in_bits; /**< Size of the layer and its sublayers */
} bitfield_t;

//--------------------------------------------------------------------------
// Allocation
//--------------------------------------------------------------------------

/**
 * \brief Allocate a bitfield
 * \param size_in_bits Size of the layer and its sublayer
 *   For example if the bitfield is define on a probe, pass the
 *   maximum size of the probe (in bits)
 * \return The address of the bitfield (NULL if error)
 */

bitfield_t * bitfield_create(size_t size_in_bits);

/**
 * \brief Delete a bitfield from the memory
 * \param bitfield Address of this bitfield
 */

void bitfield_free(bitfield_t * bitfield);

//--------------------------------------------------------------------------
// Metafield's setter/getter 
//--------------------------------------------------------------------------

/**
 * \brief Define whether a bit belongs to a bitfield
 * \warning This function is based on the endianess of your system.
 * \param bitfield The bitfield
 * \param value Pass 0 if this bit must be set to 0, pass any 
 *     other value if this bit must be set to 1.
 * \param offset_in_bits The position of the bit in the bitfield.
 *     This value must be less than to bitfield.size_in_bits
 * \return 0 if everything is fine, another value otherwise
 */

int bitfield_set_mask_bit (
    bitfield_t * bitfield,
    int           value,
    size_t        offset_in_bits
);

/**
 * \brief Define whether a block of bits belongs to a bitfield
 * \warning This function is based on the endianess of your system.
 * \param bitfield The bitfield
 * \param value Pass 0 if each altered bit must be set to 0, pass any 
 *     other value if they must be set to 1.
 * \param offset_in_bits The position of the block of bits in the bitfield.
 *     This value must be less than bitfield.size_in_bits - 
 * \return 0 if everything is fine, another value otherwise
 */

int bitfield_set_mask_bits(
    bitfield_t * bitfield,
    int           value,
    size_t        offset_in_bits,
    size_t        num_bits
);

//--------------------------------------------------------------------------
// Access
//--------------------------------------------------------------------------

/**
 * \brief Set a value in a bitfield. This function may fail if
 *    you pass a value that does not fit in the bitfield.
 *    Bits outside the bitfield aren't modified.
 * \param bitfield The bitfield on which we will iterate.
 * \param fixed A bitfield that covers bits that must not change.
 * \param buffer_value The value that we'll set in the bitfield.
 *    For instance it may be an integer and then you will pass
 *    its adress.
 * \param size_in_bits The size of buffer_value in bits.
 *    For instance if buffer_value is an integer, pass sizeof(int)*8.
 * \return 0 if success, another value (see <errrno.h>) otherwise.
 */

int bitfield_set (
    bitfield_t         * bitfield,
    const bitfield_t   * fixed,
    const unsigned char * buffer_value,
    size_t                size_in_bits
);

size_t bitfield_size_mask (const bitfield_t * bitfield);

size_t bitfield_size_covered (const bitfield_t * bitfield);

size_t bitfield_size_modifiable (
    const bitfield_t * bitfield,
    const bitfield_t * fixed 
);

/**
 * \brief Retrieve the value stored in a bitfield.
 * \param bitfield The bitfield
 * \param value The pre-allocated target buffer.
 *      It must have at least bitfield_size_covered(*bitfield)
 *      allocated.
 */

void bitfield_get (
    const bitfield_t    * bitfield,
    unsigned char       * value
);

//--------------------------------------------------------------------------
// Iteration
//--------------------------------------------------------------------------

/**
 * \brief Set the first value in a bitfield on which we will iterate.
 *    Bits outside the bitfield aren't modified.
 * \param bitfield The bitfield that we set.
 * \param fixed A bitfield that covers bits that must not change.
 */

void bitfield_set_first (
    bitfield_t       * bitfield,
    const bitfield_t * fixed
);

/**
 * \brief Set the previous value in a bitfield on which we iterate.
 *    Bits outside the bitfield aren't modified.
 * \param bitfield The bitfield that we set. 
 * \param fixed A bitfield that covers bits that must not change.
 * \return true if we have been able to iterate, false otherwise.
 */

bool bitfield_set_previous (
     bitfield_t      * bitfield,
    const bitfield_t * fixed
);

/**
 * \brief Set the next value in a bitfield on which we iterate.
 *    Bits outside the bitfield aren't modified.
 * \param bitfield The bitfield that we set. 
 * \param fixed A bitfield that covers bits that must not change.
 * \return true if we have been able to iterate, false otherwise.
 */

bool bitfield_set_next (
    bitfield_t       * bitfield,
    const bitfield_t * fixed
);

/**
 * \brief Set the last value in a bitfield on which we will iterate.
 *    Bits outside the bitfield aren't modified.
 * \param bitfield The bitfield that we set.
 * \param fixed A bitfield that covers bits that must not change.
 */

void bitfield_set_last (
    bitfield_t       * bitfield,
    const bitfield_t * fixed
);

#endif

