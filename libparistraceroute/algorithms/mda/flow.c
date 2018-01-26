#include "flow.h"

#include <stdlib.h>

mda_flow_t * mda_flow_create(uintmax_t flow_id, mda_flow_state_t state, mda_flow_certainty_t certainty)
{
    mda_flow_t * mda_flow;

    if ((mda_flow = malloc(sizeof(mda_flow_t)))) {
        mda_flow->flow_id   = flow_id;
        mda_flow->state     = state;
        mda_flow->certainty = certainty;
    }

    return mda_flow;
}

void mda_flow_free(mda_flow_t * mda_flow)
{
    if (mda_flow) free(mda_flow);
}

char mda_flow_state_to_char(const mda_flow_t * mda_flow) {
    char c;

    switch (mda_flow->state) {
        case MDA_FLOW_AVAILABLE:   c = ' '; break;
        case MDA_FLOW_UNAVAILABLE: c = '*'; break;
        case MDA_FLOW_TESTING:     c = '?'; break;
        case MDA_FLOW_TIMEOUT:     c = '!'; break;
        default:                   c = 'E'; break;
    }

    return c;
}
