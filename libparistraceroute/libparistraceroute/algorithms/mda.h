#ifndef ALGORITHMS_MDA_H
#define ALGORITHMS_MDA_H

#include "mda/data.h"
#include "mda/flow.h"
#include "mda/interface.h"
#include "traceroute.h"
#include "../optparse.h"
#include "../vector.h"

//mda command line help messages
#define HELP_B "Multipath tracing  bound: an upper bound on the probability that multipath tracing will fail to find all of the paths (default 0.05) max_branch: the maximum number of branching points that can be encountered for the bound still to hold (default 5)"

//                                   def1 min1 max1 def2 min2 max2      mda_enabled
#define OPTIONS_MDA_BOUND_MAXBRANCH {95,  0,   100, 5,   1,   INT_MAX , 0}

typedef struct {
    traceroute_options_t traceroute_options;
    unsigned             bound;
    unsigned             max_branch;
} mda_options_t;

typedef enum {
    MDA_NEW_LINK
} mda_event_type_t;

typedef struct {
    mda_event_type_t type;
    void           * data;
    void          (* data_free)(void *); /**< Called in event_free to release data. Ignored if NULL. */
    void           * zero;
} mda_event_t;

unsigned options_mda_get_bound();
unsigned options_mda_get_max_branch();
unsigned options_mda_get_is_set();

const option_t * mda_get_options();

/**
 * \brief Retrieve the default options of mda.
 * \return The corresponding mda_options_t structure.
 */

mda_options_t mda_get_default_options();

/**
 * \brief Iinitialize the mda options structure.
 * \param mda_options The corresponding mda_options_t structure.
 */

void options_mda_init(mda_options_t * mda_options);

/**
 * \brief Default mda handler.
 * \param loop The main loop.
 * \param event The event handled by mda.
 * \param pdata Data attached to the current mda algorithm instance.
 * \param skel The probe skeleton used to craft probe packets.
 * \param options The options passed to the current mda algorithm instance.
 */

int mda_handler(pt_loop_t * loop, event_t * event, void ** pdata, probe_t * skel, void * options);

#endif
