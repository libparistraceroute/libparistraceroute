#include "config.h"

#include <errno.h>  // errno
#include <stdlib.h> // malloc, calloc, free
#include <string.h> // memcpy

#include "bitfield.h"
#include "common.h" // MIN()

//--------------------------------------------------------------------------
// Allocation
//--------------------------------------------------------------------------

// alloc

bitfield_t * bitfield_create(size_t size_in_bits)
{
    // Allocate bitfield structure
    bitfield_t * bitfield = calloc(1, sizeof(bitfield_t));
    if (!bitfield) goto ERROR;

    // Allocate bitfield mask
    if (size_in_bits > 0) {
        bitfield->mask = malloc(size_in_bits / 8);
        if (!bitfield->mask) goto ERROR_MASK;
    }

    // Set size
    bitfield->size_in_bits = size_in_bits;
    return bitfield;    
ERROR_MASK:
    bitfield_free(bitfield);
ERROR:
    return NULL;
}

// dup

bitfield_t * bitfield_dup(const bitfield_t * bitfield)
{
    bitfield_t *bf;

    if (!bitfield) return NULL;
    if (!(bf = bitfield_create(bitfield->size_in_bits))) return NULL; 
    memcpy(bf->mask, bitfield->mask, bitfield->size_in_bits / 8);
    return bf;
}

// free

void bitfield_free(bitfield_t * bitfield)
{
    if (bitfield) {
        if (bitfield->mask) free(bitfield->mask);
        free(bitfield);
    }
}

//--------------------------------------------------------------------------
// Additions
//--------------------------------------------------------------------------

uint8_t *bitfield_get_mask(const bitfield_t *bitfield)
{
    return bitfield->mask;
}

//--------------------------------------------------------------------------
// Metafield's setter/getter 
//--------------------------------------------------------------------------

// internal usage

static inline void bitfield_set_0(
    uint8_t * mask,
    size_t    offset_in_bits
) {
    mask[offset_in_bits / 8] &= ~(1 << (offset_in_bits % 8));
}

// internal usage

static inline void bitfield_set_1(
    uint8_t * mask,
    size_t    offset_in_bits
) {
    mask[offset_in_bits / 8] |= 1 << (offset_in_bits % 8);
}

// bitfield initialization (per bit)

bool bitfield_set_bit(
    bitfield_t * bitfield,
    int          value,
    size_t       offset_in_bits
) {
    if (!bitfield) return false;
    if (offset_in_bits >= bitfield->size_in_bits) return false;

    if (value) bitfield_set_0(bitfield->mask, offset_in_bits);
    else       bitfield_set_1(bitfield->mask, offset_in_bits);

    return true;
}

// bitfield initialization (per block of bits)

int bitfield_set_bits(
    bitfield_t * bitfield,
    int          value,
    size_t       offset_in_bits,
    size_t       num_bits
) {
    size_t offset;
    size_t offset_end = offset_in_bits + num_bits;

    if (!bitfield
    || offset_in_bits + num_bits >= bitfield->size_in_bits
    ) {
        errno = EINVAL;
        return 1;
    }

    if (num_bits) {
        // to improve to set byte per byte
        for (offset = offset_in_bits; offset < offset_end; offset++) {
            bitfield_set_bit(bitfield, value, offset);
        }
    }
    return 0;
}

// Get i-th bit

inline int bitfield_get_bit(const bitfield_t * bitfield, size_t i) {
    if (!bitfield || !bitfield->mask || i >= bitfield->size_in_bits) return -1;
    return bitfield->mask[i / 8] & (1 << (i % 8));
}

// Find the next bit set to 1 in the bitfield

bool bitfield_find_next_1(
    const bitfield_t * bitfield,
    size_t           * pcur_offset
) {
    uint8_t cur_byte;
    size_t  i, j, jmin, jmax, size, size_in_bits, cur_offset;

    if (!bitfield)    return false; 
    if (!pcur_offset) return false; 

    cur_offset = *pcur_offset;
    size_in_bits = bitfield_get_size_in_bits(bitfield);
    if (cur_offset > size_in_bits) return false; 

    size = bitfield->size_in_bits / 8;
    for (i = cur_offset / 8; i < size; i++) {
        jmin = (i == cur_offset) ? (cur_offset % 8) : 0; 
        jmax = (i == size - 1) ? (size_in_bits % 8) : 8;
        cur_byte = bitfield->mask[i];
        for (j = jmin; j < jmax; j++) {
            if (cur_byte & (1 << j)) {
                *pcur_offset = i * 8 + j;
                return true;
            }
        }
    }

    return false;
}

// Count how many 1 are set in the bitfield

size_t bitfield_get_num_1(const bitfield_t * bitfield) {
    typedef uint8_t cell_t;
    size_t i, j, jmax, size, size_in_bits;
    size_t res = 0;

    if (!bitfield) return 0; // invalid parameter 

    size_in_bits = bitfield_get_size_in_bits(bitfield);
    size = size_in_bits / 8;

    for (i = 0; i < size; i++) {
        cell_t cur_byte = bitfield->mask[i];
        jmax = (i == size - 1) ? (size_in_bits % 8) : 8;
        for (j = 0; j < jmax; j++) {
            if (cur_byte & (1 << j)) res++;
        } 
    }
    return res;
}

// Size (in bits)

inline size_t bitfield_get_size_in_bits(const bitfield_t * bitfield) {
    return bitfield->size_in_bits;
}

//--------------------------------------------------------------------------
// Operators 
//--------------------------------------------------------------------------

// tgt &= src

void bitfield_and(bitfield_t * tgt, const bitfield_t * src) {
    size_t i, j, jmax, size_in_bits, size;

    if (!tgt || !src) return; // invalid parameter(s)

    size_in_bits = MIN(tgt->size_in_bits, src->size_in_bits);
    size = size_in_bits / 8;
    jmax = size_in_bits % 8;

    for (i = 0; i < size; ++i) {
        if (i + 1 == size) {
            for (j = 0; j < jmax; ++j) {
                if (!(src->mask[i] & (1 << j))) {
                    bitfield_set_bit(tgt, 0, 8 * i + j);
                }
            }
        } else {
            tgt->mask[i] &= src->mask[i]; 
        }
    }
}

// tgt |= src

void bitfield_or(bitfield_t * tgt, const bitfield_t * src) {
    size_t i, j, jmax, size_in_bits, size;

    if (!tgt || !src) return; // invalid parameter(s)

    size_in_bits = MIN(tgt->size_in_bits, src->size_in_bits);
    size = size_in_bits / 8;
    jmax = size_in_bits % 8;

    for (i = 0; i < size; ++i) {
        if (i + 1 == size) {
            for (j = 0; j < jmax; ++j) {
                if (src->mask[i] & (1 << j)) {
                    bitfield_set_bit(tgt, 1, 8 * i + j);
                }
            }
        } else {
            tgt->mask[i] |= src->mask[i]; 
        }
    }
}

// tgt = ~src

void bitfield_not(bitfield_t * tgt) {
    size_t i, j, jmax, size_in_bits, size;

    if (!tgt) return; // invalid parameter

    size_in_bits = tgt->size_in_bits;
    size = size_in_bits / 8;
    jmax = size_in_bits % 8;

    for (i = 0; i < size; ++i) {
        if (i + 1 == size) {
            for (j = 0; j < jmax; ++j) {
                bitfield_set_bit(
                    tgt,
                    tgt->mask[i] & (1 << j),
                    8 * i + j
                );
            }
        } else {
            tgt->mask[i] = ~tgt->mask[i]; 
        }
    }
}

