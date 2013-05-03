#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <stdio.h> // printf

#include "../../common.h"
#include "../../dynarray.h"
#include "interface.h"
#include "address.h"

mda_interface_t * mda_interface_create(const char * str_ip)
{
    mda_interface_t * interface;

    if (!(interface = calloc(1, sizeof(mda_interface_t)))) {
        goto ERR_INTERFACE;
    }

    if (str_ip) {
        if(!(interface->address = strdup(str_ip))) {
            goto ERR_ADDRESS;
        }
    }

    if (!(interface->flows = dynarray_create())) {
        goto ERR_FLOWS;
    }

    interface->type = MDA_LB_TYPE_UNKNOWN;
    return interface;

ERR_FLOWS:
    if (interface->address) free(interface->address);
ERR_ADDRESS:
    free(interface);
ERR_INTERFACE:
    errno = ENOMEM;
    return NULL;
}

void mda_interface_free(mda_interface_t * interface)
{
    if (interface) {
        dynarray_free(interface->flows, (ELEMENT_FREE) mda_flow_free);
        if (interface->address) free(interface->address);
        free(interface);
    }
}

bool mda_interface_add_flow_id(mda_interface_t * interface, uintmax_t flow_id, mda_flow_state_t state)
{
    mda_flow_t * flow;

    if ((flow = mda_flow_create(flow_id, state))) {
        dynarray_push_element(interface->flows, flow);
    }
    return flow != NULL;
}

size_t mda_interface_get_num_flows(const mda_interface_t * interface, mda_flow_state_t state)
{
    size_t i, num_flows, num_flows_with_state = 0;

    num_flows = dynarray_get_size(interface->flows);
    for (i = 0; i < num_flows; i++) {
        const mda_flow_t * flow = dynarray_get_ith_element(interface->flows, i);
        if (flow->state == state) num_flows_with_state++;
    }

    return num_flows_with_state;
}


uintmax_t mda_interface_get_available_flow_id(mda_interface_t * interface, size_t num_siblings, mda_data_t * data)
{
    size_t i, size = dynarray_get_size(interface->flows);

    // Search in the flow list for the first available one
    for (i = 0; i < size; i++) {
        mda_flow_t *flow = dynarray_get_ith_element(interface->flows, i);
        if (flow->state == MDA_FLOW_AVAILABLE) {
            flow->state = MDA_FLOW_UNAVAILABLE;
            return flow->flow_id;
        }
    }

    if (num_siblings == 1) {
        // If we are the only interface at our TTL, then all new flow_ids are
        // available, and we can just add one to our flow list and mark it as
        // unavailable. No need to send any probe to verify.

        uintmax_t flow_id = ++data->last_flow_id; // mda_interface_get_new_flow_id(interface, data);
        if (!mda_interface_add_flow_id(interface, flow_id, MDA_FLOW_UNAVAILABLE)) {
            return 0; // error adding flow id to the list
        }
        return flow_id;
    }

    return 0;
}

void mda_flow_dump(const mda_interface_t * interface)
{
    char c;
    size_t i, size = dynarray_get_size(interface->flows);

    for (i = 0; i < size; i++) {
        const mda_flow_t * flow = dynarray_get_ith_element(interface->flows, i);

        switch (flow->state) {
            case MDA_FLOW_AVAILABLE:   c = ' '; break;
            case MDA_FLOW_UNAVAILABLE: c = '*'; break;
            case MDA_FLOW_TESTING:     c = '?'; break;
            case MDA_FLOW_TIMEOUT:     c = '!'; break;
            default:                   c = 'E'; break;
        }

        printf(
            " %c%ju%c",
            c,
            flow->flow_id,
            i + 1 < size ? ',' : ' '
        );
    }
}

// Do not forget to free *phostname
void mda_hop_dump(const mda_interface_t * hop, char * hostname)
{
    printf("%s ", hop->address ? hop->address : "None");
    if (hostname) printf("(%s) ", hostname);
}

void mda_link_dump(const mda_interface_t * link[2], bool do_resolv)
{
    char * hostname = NULL;

    // Print TTL
    printf("%hhu ", link[0]->ttl);

    // Print source of the link
    if (do_resolv && link[0]->address) {
        address_resolv(link[0]->address, &hostname);
    }
    mda_hop_dump(link[0], hostname ? hostname : link[0]->address);
    if (hostname) free(hostname);

    // Print target of the link (if any)
    if (link[1]) {
        printf("-> ");
        mda_hop_dump(link[1], NULL);
    }

    // Print flow information
    printf("[");
    mda_flow_dump(link[0]);
    printf("->");
    mda_flow_dump(link[1]);
    printf("]\n");
}

void mda_interface_dump(const lattice_elt_t * lattice_elt, bool do_resolv)
{
    size_t i, num_nexthops;
    const mda_interface_t * curr_hop,
                          * next_hop;
    const dynarray_t      * next_hops;
    char                  * hostname = NULL;

    if (!lattice_elt) goto ERROR;

    // Current hop
    curr_hop = lattice_elt_get_data(lattice_elt);
    if (do_resolv) address_resolv(curr_hop->address, &hostname);
    mda_hop_dump(curr_hop, hostname);
    
    // Get next hops
    if (!(next_hops = lattice_elt->next)) {
        // We should retrieve an empty dynarray if there is no next hop
        goto ERROR_NEXTHOPS;
    }

    num_nexthops = dynarray_get_size(next_hops);

    // Print next hops
    if (num_nexthops) {
        printf(" -> ");
        for (i = 0; i < num_nexthops; ++i) {
            next_hop = lattice_elt_get_data(dynarray_get_ith_element(next_hops, i));
            mda_hop_dump(next_hop, NULL); 
        }
    }

    // Flow information
    printf(" [");
    mda_flow_dump(curr_hop);
    if (num_nexthops) {
        printf(" -> ");
        for (i = 0; i < num_nexthops; ++i) {
            next_hop = lattice_elt_get_data(dynarray_get_ith_element(next_hops, i));
            mda_flow_dump(next_hop);
        }
    }
    printf("]");

ERROR_NEXTHOPS:
    if (hostname) free(hostname);
ERROR:
    return;
}
