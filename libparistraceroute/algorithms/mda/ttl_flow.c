#include "ttl_flow.h"

#include <stdlib.h>         // free

#include "../../common.h"   // ELEMENT_FREE 

mda_ttl_flow_t * mda_ttl_flow_create(uint8_t ttl, mda_flow_t * mda_flow) 
{
    mda_ttl_flow_t * mda_ttl_flow;

    if (!(mda_ttl_flow = malloc(sizeof(mda_ttl_flow_t)))) {
        goto ERR_MALLOC;
    }    

    mda_ttl_flow->ttl      = ttl;
    mda_ttl_flow->mda_flow = mda_flow;

    return mda_ttl_flow;

ERR_MALLOC:
    return NULL;
}

void mda_ttl_flow_free(mda_ttl_flow_t * mda_ttl_flow) 
{
    if (mda_ttl_flow) {
        if (mda_ttl_flow->mda_flow) {
            mda_flow_free(mda_ttl_flow->mda_flow);
        }
        free(mda_ttl_flow);
    } 
}
