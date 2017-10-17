#ifndef MDA_TTL_FLOW_H
#define MDA_TTL_FLOW_H

#include "flow.h"           // mda_flow_state_t

#define MAX_TTLS 255 // Max ttls we assume can be associated with this interface
                   // TODO Avoid hardcoding

typedef struct {
    uint8_t      ttl;
    mda_flow_t * mda_flow;
} mda_ttl_flow_t;

/**
 * \brief Allocate new ttl/flow tuple
 * \param ttl The ttl for this tuple
 * \param mda_flow the flow for this tuple
 * \return A pointer to the ttl/flow tuple
 */

mda_ttl_flow_t * mda_ttl_flow_create(uint8_t ttl, mda_flow_t * mda_flow);

/**
 * \brief Free a given ttl/flow tuple
 * \param mda_ttl_flow The given tuple to free
 */

void mda_ttl_flow_free(mda_ttl_flow_t * mda_ttl_flow);

#endif // MDA_TTL_FLOW_H
