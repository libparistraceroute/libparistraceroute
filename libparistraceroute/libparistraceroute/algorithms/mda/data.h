#ifndef MDA_DATA_H
#define MDA_DATA_H

#include "../../lattice.h"
#include "../../pt_loop.h"
#include "../../probe.h"

typedef struct {
    /* Root of the lattice storing the interfaces */
    lattice_t   * lattice;
    uintmax_t     last_flow_id;
    unsigned int  confidence;
    char        * dst_ip;
    pt_loop_t   * loop;
    probe_t     * skel;
} mda_data_t;

//mda_data_t* mda_data_create(pt_loop_t * loop, probe_t * probe);
mda_data_t* mda_data_create(void);
void mda_data_free(mda_data_t *data);

#endif /* MDA_DATA_H */

