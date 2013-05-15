#include <stdlib.h>
#include "data.h"
#include "interface.h"

mda_data_t* mda_data_create()
{
    mda_data_t * data;

    if ((data = malloc(sizeof(mda_data_t)))) {
        data->lattice = lattice_create();
        data->last_flow_id = 0;
        
        // Options
        data->confidence = 95;

        // Internal data
        data->loop = NULL;
        data->skel = NULL;
    }
    return data;
}

void mda_data_free(mda_data_t * data)
{
    if (data) {
        lattice_free(data->lattice, (ELEMENT_FREE) mda_interface_free);
        free(data);
    }
}

