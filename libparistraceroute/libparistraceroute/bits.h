#ifndef BITS_H
#define BITS_H

#include <stdbool.h> // bool
#include <stdint.h>  // uint*_t
#include <stddef.h>  // size_t

//---------------------------------------------------------------------------
// Bit-level operations on a single byte
//---------------------------------------------------------------------------

/**
 * \brief Make a uint8_t mask having num_bits bits set to 1 starting to the
 *    offset_in_bits-th bit. The other bits are set to 0.
 * \param offset_in_bits The first bit set to 1.
 * \param num_bits The number of bits set to 1.
 * \return The corresponding mask.
 */

uint8_t byte_make_mask(size_t offset_in_bits, size_t num_bits);

/**
 * \brief Write a byte in the standard output (binary format).
 * \param byte The byte we want to write. 
 */

void byte_dump(uint8_t byte);

//---------------------------------------------------------------------------
// Bit-level operations on one or more bytes 
//---------------------------------------------------------------------------

/**
 * \brief Extract 'num_bits' bits from a sequence of bytes starting from its
 *    'offset_in_bits'-th bit and but the result in an output sequence of bytes
 *    with the an offset of 'offset_in_bits_out' bits.
 * \param bytes The queried bytes.
 * \param offset_in_bits The first extracted bit of 'bytes'.
 * \param num_bits The number of extracted bits.
 * \param offset_in_bits_out The starting bit where we write in the *pdest.
 * \param dest The uint8_t * which will store the output value. Pass NULL if
 *    bits_extract must allocate automatically this buffer. This buffer must
 *    be at least of size (offset_in_bits + num_bits % 8) 
 * \return The corresponding output buffer, NULL in case of failure.
 *
 * Example:
 * 
 *   bits_extract(x = 00111010 11111010 11000000 00000000, 2, 21) = 00011101 01111101 01100000
 *   21 bits are extract from x (starting from the 2nd bit of x)
 *   They are copied and right-aligned in y.
 * 
 * This example is detailed in bits.c (see function bits_test).
 */

uint8_t * bits_extract(
    const uint8_t * bytes,
    size_t          offset_in_bits,
    size_t          num_bits,
    uint8_t       * dest
);

/**
 * \brief Write a byte in the standard output (binary format).
 * \param byte The byte we want to write. 
 */

void bits_dump(const uint8_t * bytes, size_t num_bytes);

#endif
