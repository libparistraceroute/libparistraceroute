#include <stdlib.h>
#include "data.h"
#include "interface.h"

mda_data_t * mda_data_create()
{
    mda_data_t * data;

    if (!(data = calloc(1, sizeof(mda_data_t)))) {
        goto ERR_MALLOC;
    }

    if (!(data->lattice = lattice_create())) {
        goto ERR_LATTICE_CREATE;
    }

    if (!(data->dst_ip = address_create())) {
        goto ERR_ADDRESS_CREATE;
    }

    // Options
    data->confidence = 95;

    if (!(data->bound = bound_create(0.05, 16))) {
        goto ERR_BOUND_CREATE;
    }

    return data;

ERR_BOUND_CREATE:
    address_free(data->dst_ip); 
ERR_ADDRESS_CREATE:
    lattice_free(data->lattice, (ELEMENT_FREE) mda_interface_free);
ERR_LATTICE_CREATE:
    free(data);
ERR_MALLOC:
    return NULL;
}

void mda_data_free(mda_data_t * data)
{
    if (data) {
        lattice_free(data->lattice, (ELEMENT_FREE) mda_interface_free);
        address_free(data->dst_ip);
        free(data);
    }
}

