#ifndef METAFIELD_H
#define METAFIELD_H

#include <unistd.h>
#include "dynarray.h"
#include "bitfield.h"
#include "protocol_field.h"

// metafield = sur champ
// définit pour une clé par exemple flow le bon bitfield
// abstrait un concept (par exemple flow) ce qui permet d'avoir une implem commune entre ipv4 et ipv6
// probe_set_field("flow_id", ...)
// probe_set_constraint("flow_id", CONSTANT)

// ==, !=, pas de < ou de >

/**
 * A metafield provides an abstraction of a set of bits stored in a
 * data structure. This is a convenient way to abstract a concept
 * (for example "what is a flow").
 * This extend the concept of protocol_field.
 */

typedef struct metafield_s {
    /* Exposed fields */
    const char  * name;
    char       ** patterns;

    /* Internal fields */
    bitfield_t   bitfield;    /**< Bits related to the metafield    */

} metafield_t;

metafield_t* metafield_search(const char * name);
void metafield_register(metafield_t * metafield);

// - pattern matching
// - successor of a value
// - bitmask of unauthorized bits ?
// - prevent some fields to be used : eg. do not vary dst_port not to appear as
//   a port scan. how to do it for flow_id and ipv6 for example ?
// - an options to parametrize metafields 











typedef long long int metafield_value_t;

//--------------------------------------------------------------------------
// Allocation
//--------------------------------------------------------------------------

/**
 * \brief Allocate a metafield. You are then suppose to set
 * \param key Name of the metafield (for example "flow")
 * \param fields Fields related to the metafield (for example
 *   "src_ip", "dst_ip", "src_port", "dst_port", "proto" for
 *   a tcp/ip flow)
 * \return Address of the metafield if success, NULL otherwise
 */

/*
metafield_t * metafield_create(
    const char * key
    // TO COMPLETE
);
*/

/**
 * \brief Delete a metafield from the memory.
 * It only releases the metafield_t instance, not the pointed structures.
 */

//void metafield_free(metafield_t * metafield);

//--------------------------------------------------------------------------
// Getter / setter 
//--------------------------------------------------------------------------

/**
 * \brief Sum the size of every underlying fields 
 * \param metafield The metafield
 * \return The number of bytes 
 */

//size_t metafield_get_size(const metafield_t * metafield);

/**
 * \brief Retrieve the value stored in a metafield.
 * \param metafield The metafield
 * \return The integer stored in the metafield. 
 */

//metafield_value_t metafield_get(const metafield_t * metafield);

/**
 * \brief Set a value stored in a buffer in a metafield.
 * \param value The value we will set in the metafield.
 */

/*
bool metafield_set(
    metafield_t       * metafield,
    metafield_value_t   value
);
*/

//--------------------------------------------------------------------------
// Operator 
//--------------------------------------------------------------------------

/**
 * \brief Test whether two metafields are equal
 * \param x The first metafield
 * \param y The first metafield
 * \return true iif x == y
 */

/*
bool metafield_equal(
    const metafield_t * x,
    const metafield_t * y 
);
*/

/**
 * \brief Test whether two metafields are different 
 * \param x The first metafield
 * \param y The first metafield
 * \return true iif x != y
 */

/*
bool metafield_not_equal(
    const metafield_t * x,
    const metafield_t * y 
);
*/

#endif

