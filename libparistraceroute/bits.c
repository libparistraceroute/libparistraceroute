#include "config.h"

#include <assert.h> // assert
#include <stdlib.h> // malloc, free
#include <stdio.h>  // FILE *
#include <string.h> // memcpy

#include "bits.h"
#include "common.h" // MIN, MAX

//---------------------------------------------------------------------------
// Bit-level operations on a single byte
//---------------------------------------------------------------------------

/**
 * \brief Make a uint8_t mask made of 'num_bits' bits set to 1, the
 * following ones are set to 0.
 * \param num_bits The number of bits set to 1.
 * \return The corresponding mask.
 */

static inline uint8_t make_msb_mask(size_t num_bits) {
    return num_bits > 7 ? 0xff : 0xff << (8 - num_bits);
}

/**
 * \brief Make a uint8_t mask made of 8 - 'num_bits' bits set to 0, the
 * following ones are set to 1.
 * \param num_bits The number of bits set to 1.
 * \return The corresponding mask.
 */

static inline uint8_t make_lsb_mask(size_t num_bits) {
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

uint8_t byte_extract(uint8_t byte, size_t offset_in_bits, size_t num_bits, size_t offset_in_bits_out) {
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

//---------------------------------------------------------------------------

uint8_t byte_make_mask(size_t offset_in_bits, size_t num_bits) {
    uint8_t mask;

    if (offset_in_bits == 0) {
        // Optimization: write "num_bits" 1, then the missing 0.
        mask = make_msb_mask(num_bits);
    } else if (offset_in_bits + num_bits == 8) {
        // Optimization: write some 0, then "num_bits" 1.
        mask = make_lsb_mask(num_bits);
    } else {
        // Craft the mask by hand.
        mask = byte_make_mask_impl(offset_in_bits, num_bits);
    }
    return mask;
}

bool byte_write_bits(
    uint8_t * byte_out,
    size_t    offset_in_bits_out,
    uint8_t   byte_in,
    size_t    offset_in_bits_in,
    size_t    size_in_bits
) {
    int offset;

    if (size_in_bits == 0) return true;
    offset = offset_in_bits_out - offset_in_bits_in;

    // Ensure that reading starts in the input byte.
    if (offset_in_bits_in > 7) {
        fprintf(stderr, "byte_write_bits: offset_in_bits_in = %zu > 7\n", offset_in_bits_in);
        return false;
    }

    // Ensure that reading ends in the input byte.
    if (offset_in_bits_in + size_in_bits > 8) {
        fprintf(stderr, "byte_write_bits: offset_in_bits_in + size_in_bits = %zu + %zu > 8\n", offset_in_bits_in, size_in_bits);
        return false;
    }

    // Extract the relevant bits of the input_byte, then set to 0 the
    // corresponding bits of output_byte.
    byte_in   &=  byte_make_mask(offset_in_bits_in,  size_in_bits);
    *byte_out &= ~byte_make_mask(offset_in_bits_out, size_in_bits);

    // Offset the extracted byte to conform to the output offset.
    if (offset < 0) {
        byte_in <<= -offset;
    } else if (offset > 0) {
        byte_in >>= offset;
    }

    // Update the output byte.
    *byte_out |= byte_in;
    return true;
}

void byte_dump(uint8_t byte) {
    bits_dump(&byte, 8, 0);
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
    size_t  i          = offset_in_bits >> 3,
            idest      = 0,
            j,
            num_bits   = length_in_bits % 8,  // Number of bits extracted from the first byte
            num_bytes  = length_in_bits >> 3, // Number of complete byte to extract
            offset     = (offset_in_bits + num_bits) % 8,
            size       = num_bytes + (num_bits ? 1 : 0);
    uint8_t msb, lsb;
    bool    is_aligned = ((offset_in_bits + length_in_bits % 8) == 0);

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

#define BITS_MOVE(p, offset, num_bits) \
  p     += (offset + num_bits) >> 3; \
  offset = (offset + num_bits) % 8

bool bits_write(
    uint8_t       * out,
    const size_t    offset_in_bits_out,
    const uint8_t * in,
    const size_t    offset_in_bits_in,
    size_t          length_in_bits
) {
    bool success = true;

    // Notations:
    //   . bit not modified
    //   X bit being updated (same for Y) at this step.
    //   * bit to be update, but not in this step.
    //   | corresponds to byte bounds in the output.
    // Principle:
    //   Writting .....**|********|** is achieved in three steps.
    // Steps:
    //   1) Write .....XX|********|**.....
    //   2) Write .....**|XXXXXXXX|**....
    //   3) Write .....**|********|XX....

    size_t num_remaining_bits = length_in_bits;
    size_t n;
    size_t offset_out = offset_in_bits_out;
    size_t offset_in  = offset_in_bits_in;

    if (offset_in  >= 8) {
        fprintf(stderr, "bits_write: offset_in (%zu) must be < 8\n", offset_in);
        return false;
    }
    if (offset_out  >= 8) {
        fprintf(stderr, "bits_write: offset_out (%zu) must be < 8\n", offset_out);
        return false;
    }

    // 1st output (incomplete) byte:
    if (offset_out) {

        // Case 1:
        //     in: |.....XXX|YY******|
        //    out: |...XXXYY|********|
        // Case 2:
        //     in: |...XXX**|********|
        //    out: |.....XXX|********|
        // Case 3:
        //     in: |XXX*****|
        //    out: |.....XXX|

        // Cases 1, 2, 3: Write XXX
        n = MIN(8 - MAX(offset_in, offset_out), num_remaining_bits);
        success &= byte_write_bits(out, offset_out, *in, offset_in, n);
        num_remaining_bits -= n;
        assert(success);
        BITS_MOVE(out, offset_out, n);
        BITS_MOVE(in,  offset_in,  n);

        // If case 1: Write YY|
        if (offset_out) {
            n = MIN(8 - n - offset_out, num_remaining_bits);
            success &= byte_write_bits(out, offset_out, *in, offset_in, n);
            num_remaining_bits -= n;
            assert(success);
            BITS_MOVE(out, offset_out, n);
            BITS_MOVE(in,  offset_in,  n);
        }
    }

    // Full output bytes
    assert(!offset_out);
    if (!offset_in) {
        n = num_remaining_bits >> 3;
        memcpy(out, in, n);
        num_remaining_bits -= n << 3;
    } else {
        for (; num_remaining_bits >= 8; num_remaining_bits -= 8) {
            //  in: |***XXXXX|YYY.....
            // out: |XXXXXYYY|

            // Write |XXXXX***|
            n = MIN(8 - offset_in, num_remaining_bits);
            success &= byte_write_bits(out, offset_out, *in, offset_in, n);
            assert(success);
            BITS_MOVE(out, offset_out, n);
            BITS_MOVE(in,  offset_in,  n);

            // Write |*****YYY|
            n = MIN(8 - n, num_remaining_bits);
            success &= byte_write_bits(out, offset_out, *in, offset_in, n);
            assert(success);
            BITS_MOVE(out, offset_out, n);
            BITS_MOVE(in,  offset_in,  n);
        }
    }

    // Last byte
    n = num_remaining_bits;
    assert(n < 8);
    assert(offset_out == 0);
    if (n) {
        // Case1:
        //    in: |**XXX...|
        //   out: |XXX.....|
        // Case2:
        //    in: |*****XXX|YY......
        //   out: |XXXYY...|

        // Case 1, 2: Write XXX
        n = MIN(8 - offset_in, num_remaining_bits);
        success &= byte_write_bits(out, offset_out, *in, offset_in, n);
        num_remaining_bits -= n;
        assert(success);

        // Case 2: Write YY (if any)
        if (num_remaining_bits) {
            n = num_remaining_bits;
            success &= byte_write_bits(out, 0, *in, offset_in, n);
            assert(success);
        }
    }

    return success;
}

void bits_fprintf(FILE * out, const uint8_t * bytes, size_t num_bits, size_t offset_in_bits) {
    size_t i, n;
    if (offset_in_bits >= 8) {
        fprintf(stderr, "bits_fprintf: offset_in_bits (%zu) must be < 8", offset_in_bits);
        return;
    }

    for (i = 0; i < offset_in_bits; i++) {
        fprintf(out, ".");
    }

    while (num_bits) {
        // Grab the next 8, else 4, else 1 bit(s)
        n = (num_bits >= 8 && offset_in_bits     == 0) ? 8 :
            (num_bits >= 4 && offset_in_bits % 4 == 0) ? 4 :
            1;

        switch (n) {
            case 8:
                fprintf(out, "%02x", *bytes);
                break;
            case 4:
                fprintf(
                    out, "%01x",
                    offset_in_bits >> 2 == 1 ?
                         (*bytes & 0x0f) :
                        ((*bytes & 0xf0) >> 4)
                );
                break;
            case 1:
                fprintf(
                    out, "%d",
                    *bytes & 0x1 << (8 - offset_in_bits) ? 1 : 0
                );
                break;
        }
        BITS_MOVE(bytes, offset_in_bits, n);
        num_bits -= n;

        if (offset_in_bits % 8 == 0 && num_bits) fprintf(out, " ");
    }

    for (; i % 8; ++i) {
        fprintf(out, ".");
    }
}

void bits_dump(const uint8_t * bits, size_t num_bits, size_t offset_in_bits) {
    bits_fprintf(stdout, bits, num_bits, offset_in_bits);
}


