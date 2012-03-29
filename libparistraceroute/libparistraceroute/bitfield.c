#include "bitfield.h"

#include <stdlib.h>
#include <errno.h>

//--------------------------------------------------------------------------
// Allocation
//--------------------------------------------------------------------------

// alloc

bitfield_t * bitfield_create(size_t size_in_bits) {
    bitfield_t * bitfield = calloc(1, sizeof(bitfield_t));
    if (!bitfield) goto FAILURE;

    bitfield->mask = malloc(size_in_bits / 8);
    if (!bitfield->mask) goto FAILURE;

    bitfield->size_in_bits = size_in_bits;
    return bitfield;    
FAILURE:
    bitfield_free(bitfield);
    return NULL;
}

// free

void bitfield_free(bitfield_t * bitfield) {
    if (bitfield){
        if (bitfield->mask) free(bitfield->mask);
        free(bitfield);
    }
}

//--------------------------------------------------------------------------
// Metafield's setter/getter 
//--------------------------------------------------------------------------

// internal usage

inline void bitfield_set_0(unsigned char *mask, size_t offset_in_bits) {
    mask[offset_in_bits / 8] |= 1 << (offset_in_bits % 8);
}

// internal usage

inline void bitfield_set_1(unsigned char *mask, size_t offset_in_bits) {
    mask[offset_in_bits / 8] &= ~(1 << (offset_in_bits % 8));
}

// bitfield initialization (per bit)

int bitfield_set_mask_bit (
    bitfield_t * bitfield,
    int           value,
    size_t        offset_in_bits
) {
    if (!bitfield) return EINVAL;
    if (offset_in_bits >= bitfield->size_in_bits) return EINVAL;

    if (value) bitfield_set_0(bitfield->mask, offset_in_bits);
    else       bitfield_set_1(bitfield->mask, offset_in_bits);

    return 0;
}

// bitfield initialization (per block of bits)

int bitfield_set_mask_bits(
    bitfield_t * bitfield,
    int           value,
    size_t        offset_in_bits,
    size_t        num_bits
) {
    size_t offset;
    size_t offset_end = offset_in_bits + num_bits;

    if (!bitfield) return EINVAL;
    if (offset_in_bits + num_bits >= bitfield->size_in_bits) return EINVAL;

    if (num_bits) {
        // to improve to set byte per byte
        for(offset = offset_in_bits; offset < offset_end; offset++) {
            if (value) bitfield_set_0(bitfield->mask, offset);
            else       bitfield_set_1(bitfield->mask, offset);
        }
    }
    return 0;
}

//--------------------------------------------------------------------------
// Access
//--------------------------------------------------------------------------

int bitfield_set (
    bitfield_t         * bitfield,
    const bitfield_t   * fixed,
    const unsigned char * buffer_value,
    size_t                size_in_bits
) {
    return 0;
}

// size of the allocated buffer (in bits)

size_t bitfield_size_mask (const bitfield_t * bitfield) {
    return bitfield->size_in_bits;
}

// number of bits set to 1 (in bits)

size_t bitfield_size_covered (const bitfield_t * bitfield) {
    size_t i, j, size;
    size_t res = 0;
    size_t size_cell = sizeof(unsigned char) * 8;

    if (!bitfield) return 0; // :(
    size = bitfield->size_in_bits / 8;
    for (i = 0; i < size; i++) {
        unsigned char cur_byte = bitfield->mask[i];
        for (j = 0; j < size_cell; j++) {
            if (i * size_cell + j == bitfield->size_in_bits) break;
            if (cur_byte & (1 << j)) res++;
        } 
    }
    return res;
}

// number of alterable bits set to 1 (in bits) 

size_t bitfield_size_modifiable (
    const bitfield_t * bitfield,
    const bitfield_t * fixed 
) {
    size_t i, j, size;
    size_t res = 0;
    size_t size_cell = sizeof(unsigned char) * 8;

    if (!bitfield) return 0; // :(
    size = bitfield->size_in_bits / 8;
    for (i = 0; i < size; i++) {
        unsigned char cur_byte = bitfield->mask[i];
        for (j = 0; j < size_cell; j++) {
            if (i * size_cell + j == bitfield->size_in_bits) break;
            if (fixed && j < fixed->size_in_bits) {
                unsigned char cur_fixed_byte = fixed->mask[i];
                if (cur_byte & cur_fixed_byte & (1 << j)) res++;
            } else {
                if (cur_byte & (1 << j)) res++;
            }
        } 
    }
    return res;
}

void bitfield_get (
    const bitfield_t * bitfield,
    unsigned char    * value
) {
    // Not yet implemented
}

//--------------------------------------------------------------------------
// Iteration
//--------------------------------------------------------------------------

void bitfield_set_first (
    bitfield_t       * bitfield,
    const bitfield_t * fixed
) {
    // Not yet implemented
}

bool bitfield_set_previous (
    bitfield_t       * bitfield,
    const bitfield_t * fixed
) {
    // Not yet implemented
    return false;
}

bool bitfield_set_next (
    bitfield_t       * bitfield,
    const bitfield_t * fixed
) {
    // Not yet implemented
    return false;
}

void bitfield_set_last (
    bitfield_t       * bitfield,
    const bitfield_t * fixed
) {
    // Not yet implemented
}

