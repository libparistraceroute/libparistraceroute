#include "bits.h"
#include <stdlib.h>

//---------------------------------------------------------------------------
// Bit-level operations on a single byte
//---------------------------------------------------------------------------

/**
 * \brief Make a uint8_t mask made of 'num_bits' bits set to 1, the
 * following ones are set to 0.
 * \param num_bits The number of bits set to 1.
 * \return The corresponding mask.
 */

static uint8_t make_msb_mask(size_t num_bits) {
    return num_bits > 7 ? 0xff : 0xff << (8 - num_bits);
}

/**
 * \brief Make a uint8_t mask made of 8 - 'num_bits' bits set to 0, the
 * following ones are set to 1.
 * \param num_bits The number of bits set to 1.
 * \return The corresponding mask.
 */

static uint8_t make_lsb_mask(size_t num_bits) {
    return num_bits > 7 ? 0xff : ~(0xff << num_bits);
}

/**
 * \brief Make a uint8_t mask having num_bits bits set to 1 starting to the
 *    offset_in_bits-th bit. The other bits are set to 0.
 * \param offset_in_bits The first bit set to 1.
 * \param num_bits The number of bits set to 1.
 * \return The corresponding mask.
 */

static uint8_t byte_make_mask_impl(size_t offset_in_bits, size_t num_bits) {
    uint8_t mask;
    size_t  j;
    
    mask = 0;
    for (j = 0; j < 8 && j < num_bits; ++j) {
        mask |= 1 << (7 - j - offset_in_bits); 
    }
    return mask;
}

uint8_t byte_make_mask(size_t offset_in_bits, size_t num_bits) {
    uint8_t mask;

    if (offset_in_bits == 0) {
        mask = make_msb_mask(num_bits);
    } else if (offset_in_bits + num_bits == 8) {
        mask = make_lsb_mask(num_bits);
    } else {
        mask = byte_make_mask_impl(offset_in_bits, num_bits);
    }
    return mask;
}

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

static uint8_t byte_extract(uint8_t byte, size_t offset_in_bits, size_t num_bits, size_t offset_in_bits_out) {
    int     offset = offset_in_bits_out - offset_in_bits;
    uint8_t ret;

    ret = byte & byte_make_mask(offset_in_bits, num_bits);

    if (offset < 0) {
        ret <<= -offset; 
    } else if (offset > 0) {
        ret >>= offset;
    }

    return ret;
}

void byte_dump(uint8_t byte) {
    bits_dump(&byte, 1); 
}

//---------------------------------------------------------------------------
// Bit-level operations on one or more bytes 
//---------------------------------------------------------------------------

uint8_t * bits_extract(
    const uint8_t * bytes,
    size_t          offset_in_bits,
    size_t          length_in_bits,
    uint8_t       * dest
) {
    size_t    i         = offset_in_bits / 8,
              idest     = 0,
              j,
              num_bits  = length_in_bits % 8, // Number of bits extracted from the first byte
              num_bytes = length_in_bits / 8, // Number of complete byte to extract
              offset    = (offset_in_bits + num_bits) % 8,
              size      = num_bytes + (num_bits ? 1 : 0);
    uint8_t   msb, lsb;
    bool      is_aligned = ((offset_in_bits + length_in_bits % 8) == 0);

    // Allocate the destination buffer
    if (!dest) {
        if (!(dest = calloc(1, size))) goto ERR_CALLOC; 
    }

    // Grab the n first bits from the first relevant byte (i)
    // and store them in the 1st destination byte.
    if (num_bits) {
        dest[idest++] = byte_extract(bytes[i++], offset_in_bits, num_bits, 8 - num_bits);
    }

    // Retrieve the num_bytes remaining bytes to duplicate
    // into dest.
    for (j = 0; j < num_bytes; ++j, ++idest, ++i) {
        if (is_aligned) {
            dest[idest] = bytes[i]; 
        } else {
            // Fetch the 'idest'-th byte of 'dest'
            // - its 'offset' most significant bits are in bytes[i-1]
            // - its remaining less significant bits are in bytes[i]
            msb = byte_extract(bytes[i - 1], offset, 8 - offset, 0);
            lsb = byte_extract(bytes[i], 0, offset, 8 - offset);
            dest[idest] = msb | lsb;
        }
    }

    return dest;

ERR_CALLOC:
    return NULL;
}

void bits_dump(const uint8_t * bytes, size_t num_bytes) {
    uint8_t byte;
    size_t i, j;
    for (i = 0; i < num_bytes; ++i) {
        byte = bytes[i];
        for (j = 0; j < 8; ++j) {
            printf("%d", (byte & 1 << (7 - j)) ? 1 : 0);
        }
        printf(" ");
    }
}

/*
// Example:
// x = 00111010 11111010 11000000 00000000 
// y = 00011101 01111101 01100000 

void bits_test() {
    uint32_t x = htonl(0x3afac000);
    uint8_t * y;
    size_t offset_in_bits = 2;
    size_t length_in_bits = 21;
    printf("x = ");
    bits_dump((const uint8_t *) &x, sizeof(x));
    printf("\n");

    y= bits_extract(offset_in_bits, length_in_bits, (const uint8_t *) &x, NULL);
    printf("y = ");
    bits_dump(y, length_in_bits / 8 + (length_in_bits % 8 ? 1 : 0));
    printf("\n");
    if (y) free(y);
}
*/
