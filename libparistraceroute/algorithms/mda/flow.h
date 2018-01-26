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

typedef enum {
    MDA_FLOW_REAL,
    MDA_FLOW_HYPOTHETIC
} mda_flow_certainty_t;

/**
 * A structure containing flow information, to be used within MDA link discovery
 */

typedef struct {
    uintmax_t            flow_id;   /**< Flow identifier            */
    mda_flow_state_t     state;     /**< Current state of this flow */
    mda_flow_certainty_t certainty; /**< Certainty whether or not it has passed through the interface*/
} mda_flow_t;

/**
 * \brief Allocate a mda_data_t structure
 * \param flow_id Flow identifier related to this flow
 * \param state Current state of this flow
 * \param certainty Whether we are sure that this flow has passed through an interface
 * \return A pointer to the mda_data_t structure, NULL otherwise
 */

mda_flow_t * mda_flow_create(uintmax_t flow_id, mda_flow_state_t state, mda_flow_certainty_t certainty);

/**
 * \brief Release a mda_data_t structure from the memory
 * \param mda_flow A pointer to the mda_data_t instance
 */

void mda_flow_free(mda_flow_t * mda_flow);

/**
 * \brief Convert a flow state in its corresponding character output.
 * \param mda_flow A pointer to the mda_data_t instance.
 * \return The corresponding caractère, 'E' in case of failure.
 */
char mda_flow_state_to_char(const mda_flow_t * mda_flow);

#endif
