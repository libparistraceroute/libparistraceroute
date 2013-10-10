#include "mda.h"

#include <stdio.h>         // fprintf
#include <stdlib.h>        // malloc, free
#include <stdbool.h>       // bool
#include <limits.h>        // INT_MAX

#include "../algorithm.h"  // algorithm_t
#include "../common.h"     // MAX, ELEMENT_FREE
#include "../options.h"    // option_t
#include "../pt_loop.h"    // pt_send_probe
#include "../lattice.h"    // LATTICE_*
#include "../probe.h"      // probe_t

//---------------------------------------------------------------------------
// Private structures
//---------------------------------------------------------------------------

typedef struct {
    address_t     * address;
    lattice_elt_t * result;
} mda_address_t;

typedef struct {
    uint8_t         ttl;
    uintmax_t       flow_id;
    lattice_elt_t * result;
} mda_ttl_flow_t;

//---------------------------------------------------------------------------
// Options supported by mda.
// mda also supports options supported by traceroute.
//---------------------------------------------------------------------------

static unsigned mda_values[7] = OPTIONS_MDA_BOUND_MAXBRANCH;

// MDA options
static option_t mda_opt_specs[] = {
    // action           short long          metavar             help    variable
    {opt_store_int_2,   "B",  "--mda",      "bound,max_branch", HELP_B, mda_values},
    END_OPT_SPECS
    // {opt_store_int, OPT_NO_SF, "confidence", "PERCENTAGE", "level of confidence", 0},
    // per dest
    // max missing
    //{OPT_NO_ACTION}
};

const option_t * mda_get_options() {
    return mda_opt_specs;
}

unsigned options_mda_get_bound() {
    return mda_values[0];
}

unsigned options_mda_get_max_branch() {
    return mda_values[3];
}

unsigned options_mda_get_is_set() {
    return mda_values[6];
}

void options_mda_init(mda_options_t * mda_options)
{
    mda_options->bound      = options_mda_get_bound();
    mda_options->max_branch = options_mda_get_max_branch();
}

inline mda_options_t mda_get_default_options() {

    mda_options_t mda_options = {
         .traceroute_options = traceroute_get_default_options(),
         .bound              = 95,
         .max_branch         = 5
    };

    return mda_options;
}

//---------------------------------------------------------------------------
// Precomputed number of probes
//---------------------------------------------------------------------------

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

    return k[num_interfaces][(confidence == 95) ? 0 : 1];
}

static bool mda_event_new_link(pt_loop_t * loop, mda_interface_t * src, mda_interface_t * dst)
{
    event_t          * mda_event;
    mda_interface_t ** link;

    if (!(link = malloc(2 * sizeof(mda_interface_t)))) goto ERR_LINK;
    link[0] = src;
    link[1] = dst;
    if (!(mda_event = event_create(MDA_NEW_LINK, link, NULL, free))) goto ERR_MDA_EVENT;
    return pt_raise_event(loop, mda_event);

ERR_MDA_EVENT:
    free(link);
ERR_LINK:
    return false;
}

//---------------------------------------------------------------------------
// Helper functions
//---------------------------------------------------------------------------

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

/**
 * \brief Discover next hops of a given IP hop.
 * \param elt The current IP hop.
 * \param data
 * \return
 */

static lattice_return_t mda_enumerate(lattice_elt_t * elt, mda_data_t * mda_data)
{
    mda_interface_t * interface = lattice_elt_get_data(elt);
    /* Number of interfaces at the same TTL */
    size_t    num_nexthops = 0;
    probe_t * probe;
    uintmax_t flow_id = 0;
    int       i = 0;
    int       to_send = 0;
    int       num_flows_missing = 0;
    int       num_flows_avail = 0;
    int       num_flows_testing = 0;
    int       num_siblings = 0;

    //printf("Processing interface %s at ttl %hhu\n", interface->address, interface->ttl);

    // Determine the number of next hop interfaces
    num_nexthops = lattice_elt_get_num_next(elt);

    // ... and thus deduce how many packets we have to send
    to_send = mda_stopping_points(MAX(num_nexthops + 1, 2), mda_data->confidence) - interface->sent;

    //printf("find next hops of %s (to_send= %u)\n", interface->address, to_send);

    //printf("Interface %s : to_send %d - sent %u - received %u\n", interface->address, to_send, interface->sent, interface->received);
    if ((to_send <= 0) && (interface->sent == interface->received + interface->timeout)) {
        return LATTICE_DONE; // Done enumerating, walking/DFS can continue
    }

    if (interface->address && (address_compare(interface->address, mda_data->dst_ip) == 0)) {
        return (interface->sent == interface->received) ? LATTICE_DONE : LATTICE_CONTINUE;
    }

    // 1) Ensure we have enough flow_ids to enumerate interfaces

    /* How many interfaces at current ttl */
    // Only if the previous is done enumerating
    num_siblings = lattice_elt_get_num_siblings(elt);
    if (num_siblings > 1) {
        /* There are many interfaces at this TTL, we must ensure we have enough
         * flows available at the current ttl */

        num_flows_avail = mda_interface_get_num_flows(interface, MDA_FLOW_AVAILABLE);
        if (to_send > num_flows_avail) {
            // We cannot send flows until all inflight packets have arrived,
            // otherwise we might send too many.
            // potentially divided by num_siblings
            num_flows_testing = mda_interface_get_num_flows(interface, MDA_FLOW_TESTING);
            num_flows_missing = to_send - num_flows_avail - num_flows_testing;
            for (i = 0; i < num_flows_missing; i++) {
                /* Note: we are not sure all probes will go to the right interface, and
                 * we might go though us, though it might alimentate other interfaces at
                 * the same ttl... thus we need to share the probes in flight when we
                 * explore hops at the same ttl : we should set one potential probe in
                 * flight for each interface ? or multiply the number of probes in
                 * flight by the number of interface (might overestimate ?)*/
                probe = probe_dup(mda_data->skel);
                flow_id = ++mda_data->last_flow_id;
                mda_interface_add_flow_id(interface, flow_id, MDA_FLOW_TESTING); // TODO control returned value
                probe_set_fields(probe, I8("ttl", interface->ttl), I16("flow_id", flow_id), NULL); // TODO control returned value, free fields
                pt_send_probe(mda_data->loop, probe); // TODO control returned value
            }
        }
    } else {
        // The only interface, infinite amount of available flows
        // It might simplify the matching later (find_rec) XXX
        num_flows_avail = to_send;
    }

    if (num_flows_avail > to_send) {
        num_flows_avail = to_send;
    }

    // 2) Previous processing should ensure we have enough flow_ids available
    // To discover the nexthop, we duplicate the corresponding probes with an
    // incremented TTL.

    for (i = 0; i < num_flows_avail; i++) {
        // Get a new flow_id to send, or break/return
        // TODO manage properly break/return
        flow_id = mda_interface_get_available_flow_id(interface, num_siblings, mda_data);
        if (flow_id == 0) {
            fprintf(stderr, "Not enough flows found reaching: ");
            address_dump(interface->address);
            break;
        }

        // Send corresponding probe with ttl + 1
        if (!(probe = probe_dup(mda_data->skel))) {
            goto ERR_PROBE_DUP;
        }

        probe_set_fields(probe, I16("flow_id", flow_id), I8("ttl", interface->ttl + 1), NULL); // TODO control returned value, free fields
        pt_send_probe(mda_data->loop, probe);
        interface->sent++;
    }

    return LATTICE_INTERRUPT_NEXT; // OK, but enumeration not complete, interrupt walk

ERR_PROBE_DUP:
    return LATTICE_ERROR;
}

/**
 * \brief Classify an IP hop by setting its type property.
 * \param elt A pointer to the corresponding lattice node.
 * \return 1 if the classification is done, 0 otherwise, negative value in case
 * of error XXX
 */

static int mda_classify(lattice_elt_t * elt, mda_data_t * data)
{
    mda_interface_t * interface = lattice_elt_get_data(elt);
    size_t            num_nexthops;

    // If the classification has already been done or started
    if (interface->type != MDA_LB_TYPE_UNKNOWN) {
        return 1;
    }

    // If enumeration has been finished, or if we already have 2 next hops
    num_nexthops = lattice_elt_get_num_next(elt);
    if (interface->enumeration_done || num_nexthops > 1) {
        switch (num_nexthops) {
            case 0:
                interface->type = MDA_LB_TYPE_END_HOST;
                return 1;
            case 1:
                interface->type = MDA_LB_TYPE_SIMPLE_ROUTER;
                return 1;
            default:
                // More than one interface... classify
                interface->type = MDA_LB_TYPE_IN_PROGRESS;
                // XXX CLASSIFY XXX
                return 0;
        }
    }
    // ELSE remains MDA_LB_TYPE_UNKNOWN
    return 0;
}

static lattice_return_t mda_process_interface(lattice_elt_t * elt, void * data)
{
    mda_data_t       * mda_data = data;
    lattice_return_t   ret;

    // 1) Enumeration phase:
    //
    //    Continue enumerating its next hops if possible
    //    - Limit is the total number of probes sent compared to the number of
    //      expected interfaces
    //    - We send at interface->ttl + 1
    //    - Requires enough flow_ids from previous interfaces .prev[] (summed)
    //    - To be transformed into a link query

    if ((ret = mda_enumerate(elt, mda_data)) < 0) {
        goto ERR_FIND_NEXT_HOPS;
    }

    // 2) Classification phase:
    //
    //    - Send a batch of probes with the same flow_id to disambiguate
    //    PPLB from PFLB
    //    - Depends only on the confidence level
    //    - The processing is complete is the next interfaces have been
    //      enumerated and the interface classified, and there are no
    //      requests for populating.

    if (mda_classify(elt, mda_data) < 0) {
        goto ERR_CLASSIFY;
    }

    return ret;

ERR_FIND_NEXT_HOPS:
ERR_CLASSIFY:
    return LATTICE_ERROR;
}

//---------------------------------------------------------------------------
// Callbacks lattice_walk
//---------------------------------------------------------------------------

static lattice_return_t mda_search_source(lattice_elt_t * elt, void * data)
{
    mda_interface_t * interface = lattice_elt_get_data(elt);
    mda_ttl_flow_t  * search    = data;
    mda_flow_t      * mda_flow;
    size_t            i, size;

    if (interface->ttl == search->ttl) {
        size = dynarray_get_size(interface->flows);
        for (i = 0; i < size; i++) {
            mda_flow = dynarray_get_ith_element(interface->flows, i);
            if ((mda_flow->flow_id == search->flow_id)
            &&  (mda_flow->state != MDA_FLOW_TESTING)) {
                search->result = elt;
                return LATTICE_INTERRUPT_ALL;
            }
        }

        return LATTICE_INTERRUPT_NEXT; // don't process children
    }

    return LATTICE_CONTINUE; // continue until we reach the right ttl
}

static lattice_return_t mda_delete_flow(lattice_elt_t * elt, void * data)
{
    mda_interface_t * interface = lattice_elt_get_data(elt);
    mda_ttl_flow_t  * search    = data;
    size_t            i, size;

    if (interface->ttl == search->ttl) {
        size = dynarray_get_size(interface->flows);
        for (i = 0; i < size; i++) {
            mda_flow_t *flow = dynarray_get_ith_element(interface->flows, i);
            if ((flow->flow_id == search->flow_id) && (flow->state == MDA_FLOW_TESTING)) {
                dynarray_del_ith_element(interface->flows, i, (ELEMENT_FREE) mda_flow_free);
                return LATTICE_INTERRUPT_ALL;
            }
        }
        return LATTICE_INTERRUPT_NEXT; // don't process children
    }
    return LATTICE_CONTINUE; // continue until we reach the right ttl
}

static lattice_return_t mda_timeout_flow(lattice_elt_t * elt, void * data)
{
    mda_interface_t * interface = lattice_elt_get_data(elt);
    mda_ttl_flow_t  * search    = data;

    if (interface->ttl == search->ttl) {
        unsigned int i, size;

        size = dynarray_get_size(interface->flows);
        for (i = 0; i < size; i++) {
            mda_flow_t *flow = dynarray_get_ith_element(interface->flows, i);
            if ((flow->flow_id == search->flow_id) && (flow->state == MDA_FLOW_UNAVAILABLE)) {
                flow->state = MDA_FLOW_TIMEOUT;
                return LATTICE_INTERRUPT_ALL;
            }
        }
        return LATTICE_INTERRUPT_NEXT; // don't process children
    }
    return LATTICE_CONTINUE; // continue until we reach the right ttl
}

static lattice_return_t mda_search_interface(lattice_elt_t * elt, void * data)
{
    mda_interface_t * interface = lattice_elt_get_data(elt);
    mda_address_t   * search    = data;

    if (interface->address && address_compare(interface->address, search->address) == 0) {
        search->result = elt;
        return LATTICE_INTERRUPT_ALL;
    }
    return LATTICE_CONTINUE;
}

//---------------------------------------------------------------------------
// mda handlers
//---------------------------------------------------------------------------

/**
 * \brief Process ALGORITHM_INIT nested events handled by an mda algorithm instance.
 * \param loop The main loop.
 * \param event (Unused) you could pass NULL.
 * \param pdata The data related to this algorithm instance.
 * \param skel The probe skeleton.
 * \param options The options passed to mda.
 */

static void mda_handler_init(pt_loop_t * loop, event_t * event, mda_data_t ** pdata, probe_t * skel, const mda_options_t * options)
{
    mda_data_t * data;

    /*
    // DEBUG
    probe_dump(skel);
    printf("min_ttl = %d max_ttl = %d num_probes = %lu dst_ip = %s bound = %d max_branch = %d\n",
        options->traceroute_options.min_ttl,
        options->traceroute_options.max_ttl,
        options->traceroute_options.num_probes,
        options->traceroute_options.dst_ip ? options->traceroute_options.dst_ip : "",
        options->bound,
        options->max_branch
    );
    */

    // Create local data structure
    if (!(data = mda_data_create()))                    goto ERR_MDA_DATA_CREATE;
    if (!(probe_extract(skel, "dst_ip", data->dst_ip))) goto ERR_EXTRACT_DST_IP;

    // Initialize algorithm's data
    data->skel = skel;
    data->loop = loop;
    *pdata = data;

    // Create a dummy first hop, root of a lattice of discovered interfaces:
    // - not a tree since some interfaces might have several predecessors (diamonds)
    // - we assume the initial hop is not a load balancer
    if (!lattice_add_element(data->lattice, NULL, mda_interface_create(NULL))) {
        goto ERR_LATTICE_ADD_ELEMENT;
    }

    return;

ERR_LATTICE_ADD_ELEMENT:
ERR_EXTRACT_DST_IP:
    mda_data_free(data);
ERR_MDA_DATA_CREATE:
    return;
}

/**
 * \brief Process PROBE_REPLY nested events handled by an mda algorithm instance
 * \param loop The main loop
 * \param event The nested event
 * \param skel The probe skeleton
 * \param options The options passed to mda
 */

static void mda_handler_reply(pt_loop_t * loop, event_t * event, mda_data_t * data, probe_t * skel, const mda_options_t * options)
{
    // manage this XXX
    const probe_t    * probe,
                     * reply;
    lattice_elt_t    * source_elt,
                     * dest_elt;
    mda_interface_t  * source_interface,
                     * dest_interface;
    mda_ttl_flow_t     search_ttl_flow;
    mda_address_t      search_interface;
    mda_flow_t       * mda_flow;
    address_t          addr;
    uintmax_t          flow_id;
    uint8_t            ttl;
    int                ret;

    probe = ((const probe_reply_t *) event->data)->probe;
    reply = ((const probe_reply_t *) event->data)->reply;

    if (!(probe_extract(probe, "ttl",     &ttl)))     goto ERR_EXTRACT_TTL;
    if (!(probe_extract(probe, "flow_id", &flow_id))) goto ERR_EXTRACT_FLOW_ID;
    if (!(probe_extract(reply, "src_ip",  &addr)))    goto ERR_EXTRACT_SRC_IP;

    //printf("Probe reply received: %hhu %s [%ju]\n", ttl, addr, flow_id);

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

    search_interface.address = &addr;
    search_interface.result = NULL;
    ret = lattice_walk(data->lattice, mda_search_interface, &search_interface, LATTICE_WALK_DFS);
    if (ret == LATTICE_INTERRUPT_ALL) {
        // Destination found
        dest_elt = search_interface.result;
        dest_interface = lattice_elt_get_data(dest_elt);
    } else {
        dest_elt = NULL;
        dest_interface = mda_interface_create(&addr);
        dest_interface->ttl = ttl;
    }

    search_ttl_flow.ttl = ttl - 1;
    search_ttl_flow.flow_id = flow_id;
    search_ttl_flow.result = NULL;
    ret = lattice_walk(data->lattice, mda_search_source, &search_ttl_flow, LATTICE_WALK_DFS);
    if (ret == LATTICE_INTERRUPT_ALL) {
        // Found
        source_elt = search_ttl_flow.result;
        source_interface = lattice_elt_get_data(source_elt);

        if (dest_elt) {
            if (!lattice_connect(data->lattice, source_elt, dest_elt)) {
                goto ERR_LATTICE_CONNECT;
            }

            // An interface has the minimum TTL possible, in case of multiple
            // previous hops
            if (source_interface->ttl + 1 < dest_interface->ttl) {
                dest_interface->ttl = source_interface->ttl + 1;
            }

            /* dest_interface->num_stars = 0;
             *
             * XXX should be always 0
             * e.g. for closing diamonds with stars
             * but we cannot have several parallel interfaces with stars at the
             * moment
             */

        } else {
            if (!lattice_add_element(data->lattice, source_elt, dest_interface)) {
                goto ERR_LATTICE_ADD_ELEMENT;
            }
        }

        source_interface->received++;

        // We have received the last needed flow
        if (source_interface->received + source_interface->timeout == source_interface->sent) {
            if (!mda_event_new_link(loop, source_interface, dest_interface)) {
                goto ERR_MDA_EVENT_NEW_LINK;
            }
        }

    } else {
        // Delete flow in all siblings
        search_ttl_flow.ttl = ttl;
        search_ttl_flow.flow_id = flow_id;
        search_ttl_flow.result = NULL;
        lattice_walk(data->lattice, mda_delete_flow, &search_ttl_flow, LATTICE_WALK_DFS);
    }

    // Insert flow in the right interface
    if (!(mda_flow = mda_flow_create(flow_id, MDA_FLOW_AVAILABLE))) {
        goto ERR_MDA_FLOW_CREATE;
    }

    if (!dynarray_push_element(dest_interface->flows, mda_flow)) {
        goto ERR_DYNARRAY_PUSH_ELEMENT;
    }

    return;

ERR_DYNARRAY_PUSH_ELEMENT:
    mda_flow_free(mda_flow);
ERR_MDA_FLOW_CREATE:
ERR_MDA_EVENT_NEW_LINK:
ERR_LATTICE_ADD_ELEMENT:
ERR_LATTICE_CONNECT:
ERR_EXTRACT_SRC_IP:
ERR_EXTRACT_FLOW_ID:
ERR_EXTRACT_TTL:
    return;
}

static void mda_handler_timeout(pt_loop_t *loop, event_t *event, mda_data_t * data, probe_t *skel, const mda_options_t * options)
{
    probe_t               * probe;
    lattice_elt_t         * source_elt;
    mda_interface_t       * source_interface;
    mda_ttl_flow_t          search_ttl_flow;
    uintmax_t               flow_id;
    uint8_t                 ttl;
    int                     ret;
    size_t                  i, num_next;

    probe = event->data;

    if (!(probe_extract(probe, "ttl",     &ttl)))     goto ERR_EXTRACT_TTL;
    if (!(probe_extract(probe, "flow_id", &flow_id))) goto ERR_EXTRACT_FLOW_ID;

    search_ttl_flow.ttl = ttl - 1;
    search_ttl_flow.flow_id = flow_id;
    search_ttl_flow.result = NULL;
    ret = lattice_walk(data->lattice, mda_search_source, &search_ttl_flow, LATTICE_WALK_DFS);
    if (ret == LATTICE_INTERRUPT_ALL) {
        // Found
        source_elt = search_ttl_flow.result;
        source_interface = lattice_elt_get_data(source_elt);
        source_interface->timeout++;

        // Mark the flow as timeout
        search_ttl_flow.ttl = ttl - 1;
        search_ttl_flow.flow_id = flow_id;
        search_ttl_flow.result = NULL;
        mda_timeout_flow(source_elt, &search_ttl_flow);

        if (source_interface->timeout == source_interface->sent) { // XXX to_send ??
            // All timeouts, we need to add a star interface, and start a new
            // discovery at the next ttl. Currently, that supposes we have only
            // one interface...
            if (source_interface->num_stars < options->traceroute_options.max_undiscovered) {
                mda_interface_t * new_iface = mda_interface_create(NULL);
                new_iface->ttl = ttl;
                new_iface->num_stars = source_interface->num_stars + 1;

                if (!lattice_add_element(data->lattice, source_elt, new_iface)) {
                    goto ERROR;
                }

                if (!mda_event_new_link(loop, source_interface, new_iface)) {
                    goto ERROR;
                }
            } else if (!mda_event_new_link(loop, source_interface, NULL)) {
                goto ERROR;
            }
        } else if (source_interface->timeout + source_interface->received == source_interface->sent) {
            // We have received all answers, and the last is a timeout (since we
            // are processing it
            num_next = dynarray_get_size(source_elt->next);
            for (i = 0; i < num_next; i++) {
                lattice_elt_t   * next_elt = dynarray_get_ith_element(source_elt->next, i);
                mda_interface_t * next_iface = lattice_elt_get_data(next_elt);
                if (!mda_event_new_link(loop, source_interface, next_iface)) {
                    goto ERROR;
                }
            }
        }

    } else {
        // Delete flow in all siblings
        search_ttl_flow.ttl = ttl;
        search_ttl_flow.flow_id = flow_id;
        search_ttl_flow.result = NULL;

        // Mark the flow as timeout
        lattice_walk(data->lattice, mda_timeout_flow, &search_ttl_flow, LATTICE_WALK_DFS);
    }

    return;

ERROR:
ERR_EXTRACT_FLOW_ID:
ERR_EXTRACT_TTL:
    return;
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
 * \return 0 iif successful
 */

int mda_handler(pt_loop_t * loop, event_t * event, void ** pdata, probe_t * skel, void * opts)
{
    mda_data_t          * data = (mda_data_t *) *pdata;
    const mda_options_t * options = opts;

    switch (event->type) {
        case ALGORITHM_INIT:
            mda_handler_init(loop, event, (mda_data_t **) pdata, skel, options);
            data = *pdata;
            break;
        case PROBE_REPLY:
            data = *pdata;
            mda_handler_reply(loop, event, data, skel, options);
            break;
        case PROBE_TIMEOUT:
            data = *pdata;
            mda_handler_timeout(loop, event, data, skel, options);
            break;
        default:
            fprintf(stderr, "mda_handler: ignoring unhandled event (type = %d)\n", event->type);
            return 0;
    }

    // Process available interfaces
    switch (lattice_walk(data->lattice, mda_process_interface, data, LATTICE_WALK_DFS)) {
        case LATTICE_ERROR:
            fprintf(stderr, "mda_handler: LATTICE_ERROR\n");
            return -1;
        case LATTICE_DONE:  break;
        default:            return 0;
    }

    pt_raise_terminated(loop);
    return 0;
}

static algorithm_t mda = {
    .name     = "mda",
    .handler  = mda_handler,
    .options  = (const struct opt_spec *) &mda_opt_specs
};

ALGORITHM_REGISTER(mda);
