#ifndef MDA_DATA_H
#define MDA_DATA_H

#include "bound.h"          // bound_t
#include "../../address.h"  // address_t
#include "../../lattice.h"  // lattice_t
#include "../../pt_loop.h"  // pt_loop_t
#include "../../probe.h"    // probe_t

typedef struct {
    lattice_t    * lattice;      /**< Root of the lattice storing the interfaces */
    uintmax_t      last_flow_id;
    unsigned int   confidence;
    address_t    * dst_ip;       /**< Destination IP */
    pt_loop_t    * loop;         /**< Main loop */
    probe_t      * skel;         /**< Probe skeleton */
    bound_t      * bound;        /**< Bound on probes to send */
} mda_data_t;

/**
 * \brief Allocate a mda_data_t structure
 * \return A pointer to the mda_data_t structure, NULL otherwise
 */

mda_data_t * mda_data_create();

/**
 * \brief Release a mda_data_t structure from the memory
 * \param A pointer to the mda_data_t instance
 */

void mda_data_free(mda_data_t * data);

#endif

