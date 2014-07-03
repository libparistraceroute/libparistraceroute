#include <stdlib.h>
#include "data.h"
#include "interface.h"
#include "../mda.h"

#define PERCENT_TO_INVERSE_DECIMAL(X) ((double)(100 - (X)) / 100.0)

mda_data_t * mda_data_create()
{
    double        failure;
    mda_data_t  * data;
    mda_options_t mda_options = mda_get_default_options();

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
    options_mda_init(&mda_options);
    data->confidence = mda_options.bound;

    failure = PERCENT_TO_INVERSE_DECIMAL(mda_options.bound);

    if (!(data->bound = bound_create(failure, mda_options.max_branch))) {
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

