#include <stdlib.h>
#include "flow.h"

mda_flow_t * mda_flow_create(uintmax_t flow_id, mda_flow_state_t state)
{
    mda_flow_t *flow;

    flow = malloc(sizeof(mda_flow_t));
    if (!flow)
        goto error;

    flow->flow_id = flow_id;
    flow->state = state;

    return flow;

error:
    return NULL;
}

void mda_flow_free(mda_flow_t *flow)
{
    free(flow);
}
