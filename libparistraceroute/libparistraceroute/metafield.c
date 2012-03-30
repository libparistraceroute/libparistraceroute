#include "metafield.h"

#include <stdlib.h>

//--------------------------------------------------------------------------
// Allocation
//--------------------------------------------------------------------------

inline metafield_t * metafield_create() {
    return calloc(1, sizeof(metafield_t));
}

/**
 * \brief Delete a metafield from the memory.
 * It only releases the metafield_t instance, not the pointed structures.
 */

inline void metafield_free(metafield_t * metafield){
    if (metafield) free(metafield);
}

//--------------------------------------------------------------------------
// Getter / setter 
//--------------------------------------------------------------------------

inline size_t metafield_num_bits_concept(const metafield_t * metafield) {
    return bitfield_get_num_1(metafield->bits_concept); 
}

static inline size_t min(size_t x, size_t y) {
    return x < y ? x : y;
}

// TODO expose in .h
size_t metafield_get_next_rw_offset(const metafield_t * metafield) {
    // not yet implemented
    return 0;
}

// TODO rewrite by using get_next_rw_offset
size_t metafield_num_bits_rw(const metafield_t * metafield) {
    if (!metafield->bits_concept || metafield->bits_concept->mask) {
        return 0; // invalid parameter
    }
    if (!metafield->bits_ro || metafield->bits_ro->mask) {
        return metafield_num_bits_concept(metafield); 
    }

    // We've to compare the two bitfields manually
    size_t i, j, res = 0;
    size_t size_in_bits_concept = bitfield_get_size_in_bits(metafield->bits_concept);
    size_t size_in_bits_ro      = bitfield_get_size_in_bits(metafield->bits_ro);
    size_t size_concept         = size_in_bits_concept / 8; 
    size_t size_ro              = size_in_bits_ro / 8;
    size_t size_min             = min(size_concept, size_ro);

    // Add the bits set to 1 in bits_concept and bits_ro
    for(i = 0; i < size_min; i++) {
        if(i + 1 == size_ro || i + 1 == size_concept) {
            // Matching bit per bit
            for(j = i * 8; j < size_in_bits_ro && j < size_in_bits_concept; j++) {
                if((1 << (j % 8))
                   & metafield->bits_concept->mask[i]
                   & metafield->bits_ro->mask[i]
                ) {
                    ++res;
                }
            }
        } else {
            // Matching byte per byte
            unsigned char byte = metafield->bits_concept->mask[i]
                               & metafield->bits_ro->mask[i];

            for (j = 0; j < 8; j++) {
                if (byte & (1 << j)) ++res;
            }
        }        
    }

    // Add the remaining bits set to 1 in bits_concept (if any)
    for(; i < size_concept; i++) {
        if(i + 1 == size_concept) {
            unsigned char byte = metafield->bits_concept->mask[i];
            for(j = 0; j < size_in_bits_concept % 8; j++) {
                if((1 << j) & byte) ++res;
            }
        }
    }

    return res;
}

// TODO size_t bitfield_find_next_1(const bitfield_t * bitfield, size_t cur_offset)
/**
 * \brief Retrieve the value stored in a metafield.
 * \param metafield The metafield
 * \param value The pre-allocated target buffer.
 * \return 0 if success, another value (see <errno.h>) otherwise.
 */

int metafield_get(
    const metafield_t * metafield,
    unsigned char     * value
) {
    // not yet implemented
    return false;
}

int metafield_set(
    metafield_t         * metafield,
    const unsigned char * buffer_value,
    size_t                size_in_bits
) {
    // not yet implemented
    return 0;
}


//--------------------------------------------------------------------------
// Iteration
//--------------------------------------------------------------------------

/**
 * \brief Set the first value in a metafield on which we will iterate.
 * \param metafield The metafield that we set.
 */

bool metafield_set_first(metafield_t * metafield) {
    // not yet implemented
    return false;
}

/**
 * \brief Set the previous value in a metafield on which we iterate.
 * \param metafield The metafield that we set.
 */

bool metafield_set_previous(metafield_t * metafield) {
    // not yet implemented
    return false;
}

/**
 * \brief Set the next value in a metafield on which we iterate.
 * \param metafield The metafield that we set. 
 */

bool metafield_set_next(metafield_t * metafield) {
    // not yet implemented
    return false;
}

/**
 * \brief Set the last value in a metafield on which we will iterate.
 * \param metafield The metafield that we set.
 */

bool metafield_set_last(metafield_t * metafield) {
    // not yet implemented
    return false;
}


