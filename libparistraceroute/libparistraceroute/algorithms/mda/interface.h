#ifndef MDA_INTERFACE_H
#define MDA_INTERFACE_H

#include <stdbool.h>
#include <stdint.h>
#include "data.h"
#include "flow.h"
#include "../../dynarray.h"
#include "../../field.h"

typedef enum {
    MDA_LB_TYPE_UNKNOWN,
    MDA_LB_TYPE_IN_PROGRESS,
    MDA_LB_TYPE_END_HOST,
    MDA_LB_TYPE_SIMPLE_ROUTER,
    MDA_LB_TYPE_PPLB,
    MDA_LB_TYPE_PFLB,
    MDA_LB_TYPE_PDLB
} mda_lb_type_t;

typedef struct {

    char         * address;
    unsigned int   sent, received, timeout;
    unsigned int   nb_stars;

    /* Stores flow information about the current interface */
    dynarray_t   * flows;

    bool           enumeration_done;
    mda_lb_type_t  type;
    unsigned int   ttl;

} mda_interface_t;

mda_interface_t * mda_interface_create(const char * str_ip);

void mda_interface_free(mda_interface_t * interface);

bool mda_interface_add_flow_id(mda_interface_t * interface, uintmax_t flow_id, mda_flow_state_t state);

size_t mda_interface_get_num_flows(const mda_interface_t * interface, mda_flow_state_t state);

//uintmax_t mda_interface_get_new_flow_id(mda_interface_t *interface);
uintmax_t mda_interface_get_available_flow_id(mda_interface_t *interface, size_t num_siblings, mda_data_t * data);

//mda_interface_t *mda_interface_find(mda_interface_t *interface, char *addr);
//mda_interface_t *mda_interface_find_rec(mda_interface_t *interface, uint8_t ttl, uintmax_t flow_id);
//int mda_interface_add(mda_interface_t *interface, mda_interface_t *added_interface);

void mda_flow_dump(const mda_interface_t * interface);
void mda_link_dump(const mda_interface_t ** link, bool do_resolv);
void mda_interface_dump(const lattice_elt_t * elt, bool do_resolv);

#endif /* MDA_INTERFACE_H */
