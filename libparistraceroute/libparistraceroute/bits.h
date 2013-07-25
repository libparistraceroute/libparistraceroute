#include "use.h"

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
 * \brief Extract 'num_bits' bits from a uint8_t starting from its
 *    'offset_in_bits'-th bit and but the result in an output byte
 *    with the an offset of 'offset_in_bits_out' bits.
 * \param byte The queried byte.
 * \param offset_in_bits The first extracted bit of 'byte'.
 * \param num_bits The number of extracted bits.
 * \param offset_in_bits_out The starting bit where we write in the
 *    returned value.
 * \return The corresponding value.
 */

uint8_t byte_extract(uint8_t byte, size_t offset_in_bits, size_t num_bits, size_t offset_in_bits_out);

/**
 * \brief Write a sequence of bits in an output byte according
 *    to a given input byte.
 * \param byte_out The address of the output byte.
 * \param offset_in_bits_out The offset of the first bit we
 *    write in byte_out.
 * \param byte_in The byte we read to update byte_out.
 * \param offset_in_bits_out The offset of the first bit we
 *    read in byte_in.
 * \param size_in_bits The number of bits copied from
 *    byte_in to byte_out. This value must satisfy:
 *      - 0 <= size_in_bits <= 8
 *      - offset_in_bits_out + size_in_bits <= 8
 * \return true iif successful
 */

bool byte_write_bits(
    uint8_t * byte_out,
    size_t    offset_in_bits_out,
    uint8_t   byte_in,
    size_t    offset_in_bits_in,
    size_t    size_in_bits
);

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
 * \brief Write a sequence of bits according to a given sequence
 *    of input bits.
 * \param out The address of the output sequence of bits 
 * \param offset_in_bits_out The offset of the first bit we
 *    write in out.
 * \param in The sequence of bits we read to update out.
 * \param offset_in_bits_out The offset of the first bit we
 *    read in the input bits.
 * \param size_in_bits The number of bits copied from
 *    in to out.
 * \return true iif successful
 */

bool bits_write(
    uint8_t       * out,
    const size_t    offset_in_bits_out,
    const uint8_t * in,
    const size_t    offset_in_bits_in,
    size_t          length_in_bits
);

/**
 * \brief Write a byte in the standard output (binary format).
 * \param byte The byte we want to write. 
 */

void bits_dump(const uint8_t * bytes, size_t num_bytes);

#endif
