#include <stdlib.h>
#include <string.h>
#include <stdio.h> // printf

#include "../../common.h"
#include "../../dynarray.h"
#include "interface.h"
#include "address.h"

mda_interface_t * mda_interface_create(char * addr)//unsigned int addr)
{
    mda_interface_t * interface;

    interface = malloc(sizeof(mda_interface_t));
    if (!interface)
        goto ERROR;

    if (addr)
        interface->address = strdup(addr);
    else
        interface->address = NULL;

    interface->flows = dynarray_create();
    if (!interface->flows)
        goto ERR_FLOWS;

    interface->type = MDA_LB_TYPE_UNKNOWN;
    interface->enumeration_done = false;
    interface->ttl = 0;
    interface->sent = 0;
    interface->received = 0;
    interface->timeout = 0;
    interface->nb_stars = 0;

    return interface;

ERR_FLOWS:
    if (interface->address) 
        free(interface->address);
    free(interface);
ERROR:
    return NULL;
}

void mda_interface_free(mda_interface_t *interface)
{
    dynarray_free(interface->flows, (ELEMENT_FREE) mda_flow_free);
    if (interface->address)
        free(interface->address);
    free(interface);
}

int mda_interface_add_flow_id(mda_interface_t *interface, uintmax_t flow_id, mda_flow_state_t state)
{
    mda_flow_t *flow = mda_flow_create(flow_id, state);
    if (!flow)
        return -1;

    dynarray_push_element(interface->flows, flow);
    return 0;
}

unsigned int mda_interface_get_num_flows(mda_interface_t *interface, mda_flow_state_t state)
{
    unsigned int num_flows, cpt, i;

    num_flows = dynarray_get_size(interface->flows);
    for (i = 0; i < num_flows; i++) {
        mda_flow_t * flow = dynarray_get_ith_element(interface->flows, i);
        if (flow->state == state) cpt++;
    }

    return cpt;
}


uintmax_t mda_interface_get_available_flow_id(mda_interface_t *interface, unsigned int num_siblings, mda_data_t * data)
{
    unsigned int size, i;

    /* Search in the flow list for the first available one */
    size = dynarray_get_size(interface->flows);
    for (i = 0; i < size; i++) {
        mda_flow_t *flow = dynarray_get_ith_element(interface->flows, i);
        if (flow->state == MDA_FLOW_AVAILABLE) {
            flow->state = MDA_FLOW_UNAVAILABLE;
            return flow->flow_id;
        }
    }
    if (num_siblings == 1) {
        /* If we are the only interface at our TTL, then all new flow_ids are
         * available, and we can just add one to our flow list and mark it as
         * unavailable. No need to send any probe to verify */
        uintmax_t flow_id;
        int ret;

        flow_id = ++data->last_flow_id;/*mda_interface_get_new_flow_id(interface, data);*/
        ret = mda_interface_add_flow_id(interface, flow_id, MDA_FLOW_UNAVAILABLE);
        if (ret == -1)
            return 0; /* error adding flow id to the list */
        return flow_id;
    }
    return 0;
}

void mda_flow_dump(mda_interface_t * interface)
{
    unsigned int i, size;
    size = dynarray_get_size(interface->flows);
    for (i = 0; i < size; i++) {
        mda_flow_t *flow = dynarray_get_ith_element(interface->flows, i);
        switch (flow->state) {
            case MDA_FLOW_AVAILABLE:   printf(" "); break;
            case MDA_FLOW_UNAVAILABLE: printf("*"); break;
            case MDA_FLOW_TESTING:     printf("?"); break;
            case MDA_FLOW_TIMEOUT:     printf("!"); break;
            default:                                break;
        }
        printf("%ju, ", flow->flow_id);
    }
}

void mda_link_dump(mda_interface_t * link[2]) {

    if (!link[1]) {
        printf("%hhu %s (%s) [ ",
                link[0]->ttl,
                link[0]->address ? address_resolv(link[0]->address) : "*", 
                link[0]->address ? link[0]->address : "*"
                );
        mda_flow_dump(link[0]);
        printf(" ]\n");
        return;
    }
    printf("%hhu %s (%s) -> %s (%s) [ ", link[0]->ttl,
            link[0]->address ? address_resolv(link[0]->address) : "*", 
            link[0]->address ? link[0]->address : "*",
            link[1]->address ? address_resolv(link[1]->address) : "*", 
            link[1]->address ? link[1]->address : "*"
            );
    mda_flow_dump(link[0]);
    printf(" -> ");
    mda_flow_dump(link[1]);
    printf("]\n");
}

void mda_interface_dump(lattice_elt_t * elt)
{
    unsigned int i, num_next;
    mda_interface_t * link[2];
    
    link[0] = lattice_elt_get_data(elt);

    num_next = dynarray_get_size(elt->next);
    if (num_next == 0) {
        link[1] = NULL;
        mda_link_dump(link);
    }
    for (i = 0; i < num_next; i++) {
        lattice_elt_t *iter_elt;

        iter_elt = dynarray_get_ith_element(elt->next, i);
        link[1] = lattice_elt_get_data(iter_elt);

        mda_link_dump(link);

    }
}
