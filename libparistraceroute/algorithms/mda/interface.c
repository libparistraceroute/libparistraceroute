#include "interface.h"

#include <stdlib.h>         // free
#include <stdio.h>          // printf
#include <string.h>         // strdup

#include "../../common.h"   // ELEMENT_FREE 

mda_interface_t * mda_interface_create(const address_t * address)
{
    mda_interface_t * mda_interface;

    if (!(mda_interface = calloc(1, sizeof(mda_interface_t)))) {
        goto ERR_INTERFACE;
    }

    if (address) {
        if(!(mda_interface->address = address_dup(address))) {
            goto ERR_ADDRESS;
        }
    }

    if (!(mda_interface->ttl_flows = dynarray_create())) {
        goto ERR_FLOWS;
    }

    memset(mda_interface->ttl_set, 0, MAX_TTLS);
    mda_interface->num_ttls = 1;

    mda_interface->type = MDA_LB_TYPE_UNKNOWN;
    return mda_interface;

ERR_FLOWS:
    if (mda_interface->address) address_free(mda_interface->address);
ERR_ADDRESS:
    free(mda_interface);
ERR_INTERFACE:
    return NULL;
}

void mda_interface_free(mda_interface_t * interface)
{
    if (interface) {
        dynarray_free(interface->ttl_flows, (ELEMENT_FREE) mda_ttl_flow_free);
        if (interface->address) address_free(interface->address);
        free(interface);
    }
}

bool mda_interface_add_flow_id(mda_interface_t * interface, uint8_t ttl, uintmax_t flow_id, mda_flow_state_t state)
{
    mda_flow_t     * mda_flow;
    mda_ttl_flow_t * mda_ttl_flow;

    if (!(mda_flow = mda_flow_create(flow_id, state, MDA_FLOW_HYPOTHETIC))) {
        goto ERR_MDA_FLOW_CREATE;
    }

    if (!(mda_ttl_flow = mda_ttl_flow_create(ttl, mda_flow))) {
        goto ERR_TTL_FLOW_CREATE;
    }

    if (!(dynarray_push_element(interface->ttl_flows, mda_ttl_flow))) {
        goto ERR_DYNARRAY_PUSH_ELEMENT;
    }

    return true;

ERR_DYNARRAY_PUSH_ELEMENT:
    mda_ttl_flow_free(mda_ttl_flow);
ERR_TTL_FLOW_CREATE:
    mda_flow_free(mda_flow);
ERR_MDA_FLOW_CREATE:
    return false;
}

size_t mda_interface_get_num_flows(const mda_interface_t * interface, mda_flow_state_t state)
{
    const mda_ttl_flow_t * mda_ttl_flow;
    size_t                 i,
                           num_flows = dynarray_get_size(interface->ttl_flows),
                           num_flows_with_state = 0;

    for (i = 0; i < num_flows; i++) {
        mda_ttl_flow = dynarray_get_ith_element(interface->ttl_flows, i);
        if (mda_ttl_flow->mda_flow->state == state) {
            num_flows_with_state++;
        }
    }

    return num_flows_with_state;
}

mda_ttl_flow_t * mda_interface_get_available_flow_id(mda_interface_t * interface, size_t num_siblings, mda_data_t * data)
{
    uintmax_t        flow_id;
    mda_ttl_flow_t * mda_ttl_flow;
    mda_flow_t *     mda_flow;
    size_t           i, size = dynarray_get_size(interface->ttl_flows);
    uint8_t          ttl;

    // Search in the flow list for the first available one
    for (i = 0; i < size; i++) {
        mda_ttl_flow = dynarray_get_ith_element(interface->ttl_flows, i);
        mda_flow = mda_ttl_flow->mda_flow;
        if (mda_flow->state == MDA_FLOW_AVAILABLE) {
            mda_flow->state = MDA_FLOW_UNAVAILABLE;
            return mda_ttl_flow;
        }
    }

    // TODO the num ttl_set restriction could be a problem
    if (num_siblings == 1 && interface->num_ttls == 1) {
        // If we are the only interface at our TTL and have only one possible
        // ttl, then all new flow_ids are available, and we can just add one 
        // to our flow list and mark it as unavailable. No need to send any 
        // probe to verify.

        flow_id = ++data->last_flow_id; // mda_interface_get_new_flow_id(interface, data);
        ttl = interface->ttl_set[interface->num_ttls - 1];
        if (!mda_interface_add_flow_id(interface, ttl, flow_id, MDA_FLOW_UNAVAILABLE)) {
            return NULL; // error adding flow id to the list
        }
        return dynarray_get_ith_element(interface->ttl_flows, size);
    }

    return NULL;
}

static void flow_dump(const mda_interface_t * interface)
{
    const  mda_flow_t * mda_flow;
    const  mda_ttl_flow_t * mda_ttl_flow;
    size_t              i, size;

    if(!interface) {
        printf("(null)");
    } else {
        size = dynarray_get_size(interface->ttl_flows);
        for (i = 0; i < size; i++) {
            mda_ttl_flow = dynarray_get_ith_element(interface->ttl_flows, i);
            mda_flow = mda_ttl_flow->mda_flow;
            printf(
                " %d%c%ju%c",
                mda_ttl_flow->ttl,
                mda_flow_state_to_char(mda_flow),
                mda_flow->flow_id,
                i + 1 < size ? ',' : ' '
            );
        }
    }
}

/**
 * \brief Print to the standard output a mda_interface_t instance.
 * \param hop The mda_interface_t we want to print.
 * \param hostname The FQDN related to this hop.
 */

// TODO improve the 3 following functions
static void mda_hop_dump(const mda_interface_t * hop, char * hostname)
{
    if (hop->address) {
        address_dump(hop->address);
    } else printf("None");
    if (hostname) {
        printf(" (%s)", hostname);
    }
}

static inline void mda_hop_dump_without_resolv(const lattice_elt_t * elt) {
    const mda_interface_t * hop = lattice_elt_get_data(elt);
    mda_hop_dump(hop, NULL);
}

static inline void mda_hop_dump_with_resolv(const lattice_elt_t * elt) {
    const mda_interface_t * hop = lattice_elt_get_data(elt);
    char                  * hostname;

    address_resolv(hop->address, &hostname, CACHE_ENABLED);
    mda_hop_dump(hop, hostname);
    if (hostname) free(hostname);
}

void mda_link_dump(const mda_interface_t * link[2], bool do_resolv)
{
    char *  hostname = NULL;
    uint8_t ttl;
    size_t  i;

    // Print TTL
    for (i = 0; i < link[0]->num_ttls; ++i) {
        ttl = link[0]->ttl_set[i];
        printf("%hhu ", ttl);
    }

    // Print source of the link
    if (do_resolv && link[0]->address) {
        address_resolv(link[0]->address, &hostname, CACHE_ENABLED);
    }
    mda_hop_dump(link[0], hostname);
    if (hostname) free(hostname);

    // Print target of the link (if any)
    if (link[1]) {
        printf(" -> ");
        mda_hop_dump(link[1], NULL);
    }

    // Print flow information
    printf(" [{");
    flow_dump(link[0]);
    printf("} -> { ");
    flow_dump(link[1]);
    printf("}]\n");
}

void mda_lattice_elt_dump(const lattice_elt_t * lattice_elt) //, bool do_resolv)
{
    size_t                  num_nexthops;
//    const mda_interface_t * curr_hop;
    const dynarray_t      * next_hops;
    char                  * hostname = NULL;

    if (!lattice_elt) goto ERROR;

    // Current hop
//    curr_hop = lattice_elt_get_data(lattice_elt);
    mda_hop_dump_without_resolv(lattice_elt);
    
    // Get next hops
    if (!(next_hops = lattice_elt->next)) {
        // We should retrieve an empty dynarray if there is no next hop
        goto ERROR_NEXTHOPS;
    }

    num_nexthops = dynarray_get_size(next_hops);

    // Print next hops
    if (num_nexthops) {
        printf(" -> ");
        dynarray_dump(next_hops, (ELEMENT_DUMP) mda_hop_dump_without_resolv);
    }
    printf("\n");

/*
    // Flow information
    printf(" [");
    flow_dump(curr_hop);
    if (num_nexthops) {
        printf(" -> ");
        for (i = 0; i < num_nexthops; ++i) {
            next_hop = lattice_elt_get_data(dynarray_get_ith_element(next_hops, i));
            flow_dump(next_hop);
        }
    }
    printf("]");
*/

ERROR_NEXTHOPS:
    if (hostname) free(hostname);
ERROR:
    return;
}
