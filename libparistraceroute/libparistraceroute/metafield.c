#include "metafield.h"

#include <stdlib.h>

// internal usage

static inline size_t min(size_t x, size_t y) {
    return x < y ? x : y;
}

//--------------------------------------------------------------------------
// Allocation
//--------------------------------------------------------------------------

// alloc

inline metafield_t * metafield_create() {
    return calloc(1, sizeof(metafield_t));
}

// free

inline void metafield_free(metafield_t * metafield){
    if (metafield) free(metafield);
}

//--------------------------------------------------------------------------
// Getter / setter 
//--------------------------------------------------------------------------

// |bits_concept|

inline size_t metafield_num_bits(const metafield_t * metafield) {
    return bitfield_get_num_1(metafield->bits_concept); 
}

// internal usage : seek the next read/write bit

bool metafield_find_next_rw(
    const metafield_t * metafield,
    size_t            * poffset
) {
    if (!poffset || !metafield || !metafield->bits_concept) return false;

    size_t offset = *poffset;

    while (bitfield_find_next_1(metafield->bits_concept, &offset)) {
        switch(bitfield_get_bit(metafield->bits_ro, offset)) {
            case -1: // outside bits_ro => this bit is rw
            case  0: // this bit is marked as rw
                *poffset = offset;
                return true;
            case  1: // this bit is marked as ro, continue to seek
                break;
        }
    }

    return false;
}

// |bits_rw| 

size_t metafield_num_bits_rw(const metafield_t * metafield) {
    size_t res    = 0;
    size_t offset = 0;

    while(metafield_find_next_rw(metafield, &offset)) res++;
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


