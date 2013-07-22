#include "config.h"

#include <string.h>
#include <stdlib.h>
#include <search.h>

#include "metafield.h"
#include "common.h"

static void * metafields_root;

static int metafield_compare(
    const metafield_t * metafield1,
    const metafield_t * metafield2
) {
    return strcmp(metafield1->name, metafield2->name);
}

metafield_t* metafield_search(const char * name)
{
    metafield_t **metafield, search;

    if (!name) return NULL;
    search.name = name;
    metafield = tfind(&search, &metafields_root, (ELEMENT_COMPARE) metafield_compare);

    return metafield ? *metafield : NULL;
}

void metafield_register(metafield_t * metafield)
{
    // Process the patterns
    // XXX

    // Insert the metafield in the tree if the keys does not yet exist
    tsearch(metafield, &metafields_root, (ELEMENT_COMPARE) metafield_compare);
}

////--------------------------------------------------------------------------
//// Allocation
////--------------------------------------------------------------------------
//
//// alloc
//
//metafield_t * metafield_create(
//    const char * key
//) {
//    metafield_t * metafield = calloc(1, sizeof(metafield_t));
//    if (metafield) {
//        metafield->key = strdup(key);
//        if(!metafield->key) goto FAILURE;
//    }
//
//    return metafield;
//
//FAILURE:
//    if(metafield) {
//        if(metafield->key) free(metafield->key);
//        free(metafield);
//    }
//    return NULL;
//}
//
//// free
//
//inline void metafield_free(metafield_t * metafield){
//    if (metafield) free(metafield);
//}
//
////--------------------------------------------------------------------------
//// Getter / setter 
////--------------------------------------------------------------------------
//
///*
//// |bits_concept|
//
//inline size_t metafield_num_bits(const metafield_t * metafield) {
//    return bitfield_get_num_1(metafield->bits_concept); 
//}
//
//// internal usage : seek the next readable bit
//
//static inline bool metafield_find_next(
//    const metafield_t * metafield,
//    size_t            * poffset
//) {
//    return bitfield_find_next_1(metafield->bits_concept, poffset);
//}
//
//// internal usage : seek the next {read/write}able bit
//
//static bool metafield_find_next_rw(
//    const metafield_t * metafield,
//    size_t            * poffset
//) {
//    if (!poffset || !metafield || !metafield->bits_concept) return false;
//
//    size_t offset = *poffset;
//
//    while (metafield_find_next(metafield, &offset)) {
//        switch(bitfield_get_bit(metafield->bits_ro, offset)) {
//            case -1: // outside bits_ro => this bit is rw
//            case  0: // this bit is marked as rw
//                *poffset = offset;
//                return true;
//            case  1: // this bit is marked as ro, continue to seek
//                break;
//        }
//    }
//
//    return false;
//}
//
//bool metafield_get(
//    const metafield_t * metafield,
//    unsigned char     * value
//) {
//    if (!metafield || !value) return false;
//    return true;
//}
//
//bool metafield_set(
//    metafield_t         * metafield,
//    const unsigned char * buffer_value,
//    size_t                size_in_bits
//) {
//    size_t i, offset = 0;
//    if (!metafield || !buffer_value) return false;
//    if (size_in_bits >= metafield_num_bits_rw(metafield)) return false;
//
//    for(i = 0; i < size_in_bits; i++) {
//        if(!metafield_find_next_rw(metafield, &offset)) return false;
//
//    }
//    return 0;
//}
//*/
//
////--------------------------------------------------------------------------
//// Iteration
////--------------------------------------------------------------------------
//
///**
// * \brief Set the first value in a metafield on which we will iterate.
// * \param metafield The metafield that we set.
// */
//
//bool metafield_set_first(metafield_t * metafield) {
//    // not yet implemented
//    return false;
//}
//
///**
// * \brief Set the previous value in a metafield on which we iterate.
// * \param metafield The metafield that we set.
// */
//
//bool metafield_set_previous(metafield_t * metafield) {
//    // not yet implemented
//    return false;
//}
//
///**
// * \brief Set the next value in a metafield on which we iterate.
// * \param metafield The metafield that we set. 
// */
//
//bool metafield_set_next(metafield_t * metafield) {
//    // not yet implemented
//    return false;
//}
//
///**
// * \brief Set the last value in a metafield on which we will iterate.
// * \param metafield The metafield that we set.
// */
//
//bool metafield_set_last(metafield_t * metafield) {
//    // not yet implemented
//    return false;
//}
//
//
