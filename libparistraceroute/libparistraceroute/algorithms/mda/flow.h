#ifndef MDA_FLOW_H
#define MDA_FLOW_H

#include <stdint.h>
#include <stdbool.h>

typedef enum {
    MDA_FLOW_AVAILABLE,
    MDA_FLOW_UNAVAILABLE,
    MDA_FLOW_TESTING,
    MDA_FLOW_TIMEOUT
} mda_flow_state_t;

/**
 * A structure containing flow information, to be used within MDA link discovery
 */

typedef struct {
    uintmax_t        flow_id; /**< Flow identifier            */
    mda_flow_state_t state;   /**< Current state of this flow */
} mda_flow_t;

/**
 * \brief Allocate a mda_data_t structure
 * \param flow_id Flow identifier related to this flow
 * \param state Current state of this flow
 * \return A pointer to the mda_data_t structure, NULL otherwise
 */

mda_flow_t * mda_flow_create(uintmax_t flow_id, mda_flow_state_t state);

/**
 * \brief Release a mda_data_t structure from the memory
 * \param A pointer to the mda_data_t instance
 */

void mda_flow_free(mda_flow_t * flow);

#endif
