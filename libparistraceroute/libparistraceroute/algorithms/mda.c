/* TODO
 * The algorithm will have to expose a set of options to the commandline if it wants 
 * to be called from it
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

#include "optparse.h"
#include "../algorithm.h"

#include "mda/data.h"
#include "mda/flow.h"
#include "mda/interface.h"

/* MDA options */
struct opt_spec mda_options[] = {
    /* action       short       long         metavar       help                   variable XXX */
    {opt_store_int, OPT_NO_SF, "min-ttl",    "TTL",        "minimum TTL",         0},
    {opt_store_int, OPT_NO_SF, "max-ttl",    "TTL",        "maximum TTL",         0},
    {opt_store_int, OPT_NO_SF, "confidence", "PERCENTAGE", "level of confidence", 0},
    // per dest
    // max missing
    {OPT_NO_ACTION}
};

/*******************************************************************************
 * PRECOMPUTED NUMBER OF PROBES
 */

// borrowed from scamper
// stopping points
static int mda_stopping_points(unsigned int num_interfaces, unsigned int confidence)
{
    /*
     * number of probes (k) to send to rule out a load-balancer having n hops;
     * 95% confidence level first from 823-augustin-e2emon.pdf, then extended
     * with gmp-based code.
     * 99% confidence derived with gmp-based code.
     */
    static const int k[][2] = {
        {   0,   0 }, {   0,   0 }, {   6,   8 }, {  11,  15 }, {  16,  21 },
        {  21,  28 }, {  27,  36 }, {  33,  43 }, {  38,  51 }, {  44,  58 },
        {  51,  66 }, {  57,  74 }, {  63,  82 }, {  70,  90 }, {  76,  98 },
        {  83, 106 }, {  90, 115 }, {  96, 123 }, { 103, 132 }, { 110, 140 },
        { 117, 149 }, { 124, 157 }, { 131, 166 }, { 138, 175 }, { 145, 183 },
        { 152, 192 }, { 159, 201 }, { 167, 210 }, { 174, 219 }, { 181, 228 },
        { 189, 237 }, { 196, 246 }, { 203, 255 }, { 211, 264 }, { 218, 273 },
        { 226, 282 }, { 233, 291 }, { 241, 300 }, { 248, 309 }, { 256, 319 },
        { 264, 328 }, { 271, 337 }, { 279, 347 }, { 287, 356 }, { 294, 365 },
        { 302, 375 }, { 310, 384 }, { 318, 393 }, { 326, 403 }, { 333, 412 },
        { 341, 422 }, { 349, 431 }, { 357, 441 }, { 365, 450 }, { 373, 460 },
        { 381, 470 }, { 389, 479 }, { 397, 489 }, { 405, 499 }, { 413, 508 },
        { 421, 518 }, { 429, 528 }, { 437, 537 }, { 445, 547 }, { 453, 557 },
        { 462, 566 }, { 470, 576 }, { 478, 586 }, { 486, 596 }, { 494, 606 },
        { 502, 616 }, { 511, 625 }, { 519, 635 }, { 527, 645 }, { 535, 655 },
        { 544, 665 }, { 552, 675 }, { 560, 685 }, { 569, 695 }, { 577, 705 },
        { 585, 715 }, { 594, 725 }, { 602, 735 }, { 610, 745 }, { 619, 755 },
        { 627, 765 }, { 635, 775 }, { 644, 785 }, { 652, 795 }, { 661, 805 },
        { 669, 815 }, { 678, 825 }, { 686, 835 }, { 695, 845 }, { 703, 855 },
        { 712, 866 }, { 720, 876 }, { 729, 886 }, { 737, 896 }, { 746, 906 },
    };

    #define TRACELB_CONFIDENCE_MAX_N 99
    #define TRACELB_CONFIDENCE_NLIMIT(v) \
    ((v) <= TRACELB_CONFIDENCE_MAX_N ? (v) : TRACELB_CONFIDENCE_MAX_N)

    return k[num_interfaces][(confidence==95)?0:1];
}

/*******************************************************************************
 * HELPER FUNCTIONS
 */

/*
 * During the enumeration phase, each interface keeps a list of flow_ids
 * alongside their status:
 *   . MDA_FLOW_AVAILABLE   The flow has been sent by the previous hop, has
 *                          reached this interface and is available for enumeration.
 *   . MDA_FLOW_UNAVAILABLE The flow has been sent by the previous hop, has
 *                          reached this interface and has been used for
 *                          enumeration.
 *   . MDA_FLOW_TESTING     This interface requires more flow ids for
 *                          enumeration, the flow has been sent, but it is not
 *                          sure it will reach this interface if there are many
 *                          siblings.
 *
 * Note: if we process an interfact, that means the enumeration at previous hops
 * is done, its siblings are complete.
 */
int mda_interface_find_next_hops(lattice_elt_t * elt, mda_data_t * data)
{
    mda_interface_t * interface = lattice_elt_get_data(elt);
    /* Number of interfaces at the same TTL */
    unsigned int num_next;
    /* Probe to send */
    probe_t *probe;
    /* Probe information */
    uintmax_t flow_id;

    int i;
    int tosend = 0;
    int num_flows_missing = 0, num_flows_avail = 0;
    int num_flows_testing;
    int num_siblings;

    /* Determine the number of next hop interfaces */
    num_next = lattice_elt_get_num_next(elt);

    /* ... and thus deduce how many packets we have to send */
    tosend = mda_stopping_points(MAX(num_next+1, 2), data->confidence) - interface->sent;

    //printf("processing interface %s (tosend= %u)\n", interface->address, tosend);
    
    if ((tosend <= 0) && (interface->sent == interface->received)) {
        return LATTICE_DONE; // Done enumerating, walking/DFS can continue
    }

    if (interface->address && (strcmp(interface->address, data->dst_ip) == 0)) {
        return (interface->sent == interface->received) ? LATTICE_DONE : LATTICE_CONTINUE;
    }

    /*
     * Ensure we have enough flow_ids to enumerate interfaces
     */

    /* How many interfaces at current ttl */
    // Only if the previous is done enumerating
    num_siblings = lattice_elt_get_num_siblings(elt);
    if (num_siblings > 1) {
        /* There are many interfaces at this TTL, we must ensure we have enough
         * flows available at the current ttl */

        num_flows_avail = mda_interface_get_num_flows(interface, MDA_FLOW_AVAILABLE);
        if (tosend > num_flows_avail) {
            // We cannot send flows until all inflight packets have arrived,
            // otherwise we might send too many.
            // potentially divided by num_siblings
            num_flows_testing = mda_interface_get_num_flows(interface, MDA_FLOW_TESTING);
            num_flows_missing = tosend - num_flows_avail - num_flows_testing;

            for (i = 0; i < num_flows_missing; i++) {
                /* Note: we are not sure all probes will go to the right interface, and
                 * we might go though us, though it might alimentate other interfaces at
                 * the same ttl... thus we need to share the probes in flight when we
                 * explore hops at the same ttl : we should set one potential probe in
                 * flight for each interface ? or multiply the number of probes in
                 * flight by the number of interface (might overestimate ?)*/
                probe = probe_dup(data->skel);
                flow_id = ++data->last_flow_id;
                mda_interface_add_flow_id(interface, flow_id, MDA_FLOW_TESTING);
                probe_set_fields(probe, I8("ttl", interface->ttl), IMAX("flow_id", flow_id), NULL);
                pt_send_probe(data->loop, probe);
            }
        }

    } else {
        /* The only interface, infinite amount of available flows
         * It might simplify the matching later (find_rec) XXX */
        num_flows_avail = tosend;
    }

    if (num_flows_avail > tosend)
        num_flows_avail = tosend;
    
    /* Previous processing should have ensured we have enough flow_ids
     * available... */
    for (i = 0; i < num_flows_avail; i++) {
        /* Get a new flow_id to send, or break/return XXX */
        flow_id = mda_interface_get_available_flow_id(interface, num_siblings, data);
        if (flow_id == 0) break;

        /* Send corresponding probe */
        probe = probe_dup(data->skel);
        probe_set_fields(probe, IMAX("flow_id", flow_id), I8("ttl", interface->ttl+1), NULL);
        pt_send_probe(data->loop, probe);
        //printf("Sent probe at ttl %hhu with flow_id %ju\n", interface->ttl + 1, flow_id);
        interface->sent++;
    }

    return LATTICE_INTERRUPT_NEXT; // OK, but enumeration not complete, interrupt walk
}


/* \brief Classify an interface by setting its type property.
 *
 * \param interface A pointer to a mda_interface structure
 * \return 1 if the classification is done, 0 otherwise, negative value in case
 * of error XXX
 *
 * NOTE: interface->type needs to be initialized to MDA_LB_TYPE_UNKNOWN FIXME
 */
int mda_classify_interface(lattice_elt_t * elt, mda_data_t * data)
{
    mda_interface_t * interface = lattice_elt_get_data(elt);
    /* Number of next hop interfaces */
    unsigned int num_next;

    /* If the classification has already been done or started */
    if (interface->type != MDA_LB_TYPE_UNKNOWN)
        return 1;

    num_next = lattice_elt_get_num_next(elt);

    /* If enumeration has been finished, or if we already have 2 next hops */
    if (interface->enumeration_done || num_next > 1) {
        switch (num_next) {
            case 0:
                interface->type = MDA_LB_TYPE_END_HOST;
                return 1;
            case 1:
                interface->type = MDA_LB_TYPE_SIMPLE_ROUTER;
                return 1;
            default:
                /* More than one interface... classify */
                interface->type = MDA_LB_TYPE_IN_PROGRESS;
                /* XXX CLASSIFY XXX */
                return 0;
        }
    }
    /* ELSE remains MDA_LB_TYPE_UNKNOWN */
    return 0;
}

int mda_process_interface(lattice_elt_t * elt, void * data)
{
    mda_data_t      * mda_data  = data;
    int               ret, ret2;

    /* 1) continue enumerating its next hops if possible
     *    . limit is the total number of probes sent compared to the number of
     *    expected interfaces
     *    . we send at interface->ttl + 1
     *    . Requires enough flow_ids from previous interfaces .prev[] (summed)
     *    . To be transformed into a link query
     */
    ret = mda_interface_find_next_hops(elt, mda_data);
    if (ret < 0) goto error;

    /* 2) Classification:
     *    . Send a batch of probes with the same flow_id to disambiguate PPLB from
     *    PFLB
     *    . depends only on the confidence level
     *    . The processing is complete is the next interfaces have been
     *    enumerated and the interface classified, and there are no requests for
     *    populating.
     */
    ret2 = mda_classify_interface(elt, mda_data);
    if (ret2 < 0) goto error;

    return ret;

error:
    return LATTICE_ERROR;
}

/*******************************************************************************
 * MDA HANDLERS
 */

int mda_handler_init(pt_loop_t *loop, event_t *event, void **pdata, probe_t *skel)
{
    mda_data_t * data;
    int ret;

    /* Create local data structure */
    *pdata = mda_data_create();
    if (!*pdata) goto error;
    data = *pdata;

    data->dst_ip = probe_get_field(skel, "dst_ip")->value.string;
    data->loop = loop;
    data->skel = skel;

    /* Create a dummy first hop, root of a lattice of discovered interfaces:
     *  . not a tree since some interfaces might have several predecessors
     *  (diamonds)
     *  . we assume the initial hop is not a load balancer
     */
    ret = lattice_add_element(data->lattice, /*prev*/ NULL, mda_interface_create(NULL));
    if (ret < 0) goto error; // error adding element

    /* Process available interfaces */
    ret = lattice_walk(data->lattice, mda_process_interface, data, LATTICE_WALK_DFS);
    if (ret < 0) goto error; // error processing lattice

    return 0;

error:
    return -1;
}

typedef struct {
    uint8_t         ttl;
    uintmax_t       flow_id;
    lattice_elt_t * result;
} mda_ttl_flow_t;

int mda_search_source(lattice_elt_t * elt, void * data)
{
    mda_interface_t     * interface = lattice_elt_get_data(elt);
    mda_ttl_flow_t * search    = data;

    if (interface->ttl == search->ttl) {
        unsigned int i, size;

        size = dynarray_get_size(interface->flows);
        for (i = 0; i < size; i++) {
            mda_flow_t *flow = dynarray_get_ith_element(interface->flows, i);
            if ((flow->flow_id == search->flow_id) && (flow->state != MDA_FLOW_TESTING)) {
                search->result = elt;
                return LATTICE_INTERRUPT_ALL;
            }
        }
    
        return LATTICE_INTERRUPT_NEXT; // don't process children
    }

    return LATTICE_CONTINUE; // continue until we reach the right ttl
}

int mda_delete_flow(lattice_elt_t * elt, void * data)
{
    mda_interface_t     * interface = lattice_elt_get_data(elt);
    mda_ttl_flow_t * search    = data;

    if (interface->ttl == search->ttl) {
        unsigned int i, size;

        size = dynarray_get_size(interface->flows);
        for (i = 0; i < size; i++) {
            mda_flow_t *flow = dynarray_get_ith_element(interface->flows, i);
            if ((flow->flow_id == search->flow_id) && (flow->state == MDA_FLOW_TESTING)) {
                dynarray_del_ith_element(interface->flows, i);
                return LATTICE_INTERRUPT_ALL;
            }
        }
        return LATTICE_INTERRUPT_NEXT; // don't process children
    }
    return LATTICE_CONTINUE; // continue until we reach the right ttl
}


typedef struct {
    char * address;
    lattice_elt_t * result;
} mda_address_t;

int mda_search_interface(lattice_elt_t * elt, void * data)
{
    mda_interface_t        * interface = lattice_elt_get_data(elt);
    mda_address_t * search    = data;

    if (interface->address && strcmp(interface->address, search->address) == 0) {
        search->result = elt;
        return LATTICE_INTERRUPT_ALL;
    }
    return LATTICE_CONTINUE;
}

int mda_handler_reply(pt_loop_t *loop, event_t *event, void **pdata, probe_t *skel)
{
    // manage this XXX
    mda_data_t             * data;
    probe_reply_t          * pr;
    lattice_elt_t          * source_elt, * dest_elt;
    mda_interface_t        * source_interface, * dest_interface;
    mda_ttl_flow_t           search_ttl_flow;
    mda_address_t            search_interface;
    mda_flow_t             * flow;
    char                   * addr;
    uintmax_t                flow_id;
    uint8_t                  ttl;
    int                      ret;

    data = *pdata;
    pr = event->data;

    ttl     = probe_get_field(pr->probe, "ttl"    )->value.int8;
    flow_id = probe_get_field(pr->probe, "flow_id")->value.intmax;
    addr    = probe_get_field(pr->reply, "src_ip" )->value.string;

    printf("Probe reply received: %hhu %s [%ju]\n", ttl, addr, flow_id);

    /* The couple probe-reply defines a link (origin, destination)
     * 
     * origin: can be identified thanks to two parameters of the probe
     *  - probe->ttl - 1 : since we typically probe the next hop
     *  - probe->flow_id : disambiguate between several possible
     *      interfaces at the same ttl, since one flow_id will typically
     *      pass though one only.
     *  The corresponding addr is searched thanks to
     *  mda_interface_find_rec, recursively from the root interface
     *
     *  destination: reply->src_ip
     */

    search_interface.address = addr;
    search_interface.result = NULL;
    ret = lattice_walk(data->lattice, mda_search_interface, &search_interface, LATTICE_WALK_DFS);
    if (ret == LATTICE_INTERRUPT_ALL) {
        /* Destination found */
        dest_elt = search_interface.result;
        dest_interface = lattice_elt_get_data(dest_elt);
    } else {
        dest_elt = NULL;
        dest_interface = mda_interface_create(addr);
        dest_interface->ttl = ttl;
    }

    search_ttl_flow.ttl = ttl - 1;
    search_ttl_flow.flow_id = flow_id;
    search_ttl_flow.result = NULL;
    ret = lattice_walk(data->lattice, mda_search_source, &search_ttl_flow, LATTICE_WALK_DFS);
    if (ret == LATTICE_INTERRUPT_ALL) {
        /* Found */
        source_elt = search_ttl_flow.result;
        source_interface = lattice_elt_get_data(source_elt);

        if (dest_elt) {
            ret = lattice_connect(data->lattice, source_elt, dest_elt);
            if (ret < 0) goto error;

            /* An interface has the minimum TTL possible, in case of multiple previous
             * hops */
            if (source_interface->ttl + 1 < dest_interface->ttl)
                dest_interface->ttl = source_interface->ttl + 1;

        } else {
            ret = lattice_add_element(data->lattice, source_elt, dest_interface);
            if (ret < 0) goto error;

        }

        source_interface->received++;

    } else {
        /* Delete flow in all siblings */
        search_ttl_flow.ttl = ttl;
        search_ttl_flow.flow_id = flow_id;
        search_ttl_flow.result = NULL;
        lattice_walk(data->lattice, mda_delete_flow, &search_ttl_flow, LATTICE_WALK_DFS);
    }

    /* Insert flow in the right interface */
    flow = mda_flow_create(flow_id, MDA_FLOW_AVAILABLE);
    dynarray_push_element(dest_interface->flows, flow);

    /* Process available interfaces */
    ret = lattice_walk(data->lattice, mda_process_interface, data, LATTICE_WALK_DFS);
    switch(ret) {
        case LATTICE_ERROR: goto error;
        case LATTICE_DONE:  break;
        default:       return 0;
    }

    pt_raise_event(loop, ALGORITHM_TERMINATED, data->lattice);
    return 0;

error:
    return -1;
}

int mda_handler_timeout(pt_loop_t *loop, event_t *event, void **pdata, probe_t *skel)
{
    //mda_data_t * data;
    probe_t    * probe;
    uintmax_t    flow_id;
    uint8_t      ttl;

    //data = *pdata;
    probe = event->data;

    ttl     = probe_get_field(probe, "ttl"    )->value.int8;
    flow_id = probe_get_field(probe, "flow_id")->value.intmax;

    printf("Probe timeout received: %hhu [%ju]\n", ttl, flow_id);

    return 0;

}
/* Very similar to a set of fields + command line flags... */
/* algorithm_set_options just like probe_set_fields */

/* When an algo received a probe_skel, it has been appended one layer so that we can pop it afterwards */

/**
 * \brief how the algorithm handles the different events
 * \param cache a pointer to a cache instance needed to store the results
 * \param probe_skel a skeleton of a probe that will be used as a model
 * \param data a personal structure that can be used by the algorithm
 * \param events an array of events that have occured since last invocation
 */
int mda_handler(pt_loop_t *loop, event_t *event, void **pdata, probe_t *skel)
{ 
    switch (event->type) {
        case ALGORITHM_INIT:       return mda_handler_init(loop, event, pdata, skel);
        case PROBE_REPLY:          return mda_handler_reply(loop, event, pdata, skel);
        case PROBE_TIMEOUT:        return mda_handler_timeout(loop, event, pdata, skel);
        case ALGORITHM_TERMINATED: return 0;
        default:                   return -1; // EINVAL;
    }
}

static algorithm_t mda = {
    .name    = "mda",
    .handler = mda_handler
};

ALGORITHM_REGISTER(mda);
