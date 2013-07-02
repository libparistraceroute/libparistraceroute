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

struct opt_spec * mda_get_opt_specs();

int mda_set_options(vector_t * vector);

mda_options_t mda_get_default_options(void);

bool mda_event_new_link(pt_loop_t * loop, mda_interface_t * src, mda_interface_t * dst);

int mda_interface_find_next_hops(lattice_elt_t * elt, mda_data_t * data);

int mda_classify_interface(lattice_elt_t * elt, mda_data_t * data);

int mda_process_interface(lattice_elt_t * elt, void * data);

int mda_search_source(lattice_elt_t * elt, void * data);

int mda_delete_flow(lattice_elt_t * elt, void * data);

int mda_timeout_flow(lattice_elt_t * elt, void * data);

// Handlers
int mda_handler        (pt_loop_t * loop, event_t * event, void       ** pdata, probe_t * skel, void * options);
int mda_handler_init   (pt_loop_t * loop, event_t * event, mda_data_t ** pdata, probe_t * skel, const mda_options_t * options);
int mda_handler_reply  (pt_loop_t * loop, event_t * event, mda_data_t *  data,  probe_t * skel, const mda_options_t * options);
int mda_handler_timeout(pt_loop_t * loop, event_t * event, mda_data_t *  data,  probe_t * skel, const mda_options_t * options);

int mda_search_interface(lattice_elt_t * elt, void * data);

#endif
