#ifndef MDA_INTERFACE_H
#define MDA_INTERFACE_H

#include <stdbool.h>        // bool
#include <stddef.h>         // size_t

#include "data.h"           // mda_data_t
#include "flow.h"           // mda_flow_state_t
#include "../../dynarray.h" // dynarray_t
#include "../../address.h"  // address_t

#define MAX_TTLS 5 // Max ttls we assume can be associated with this interface
                   // TODO Avoid hardcoding

typedef enum {
    MDA_LB_TYPE_UNKNOWN,             /**< IP hop state not yet classified  */
    MDA_LB_TYPE_IN_PROGRESS,         /**< IP hop not yet classified        */
    MDA_LB_TYPE_END_HOST,            /**< Target destination of mda        */
    MDA_LB_TYPE_SIMPLE_ROUTER,       /**< Simple router                    */
    MDA_LB_TYPE_PPLB,                /**< Per packet load balancer         */
    MDA_LB_TYPE_PFLB,                /**< Per flow load balancer           */
    MDA_LB_TYPE_PDLB                 /**< Per destination load balancer    */
} mda_lb_type_t;

typedef struct {
    address_t   * address;          /**< Interface attached to this hop   */
    size_t        sent,             /**< Number of probes to discover its next hops */
                  received,         
                  timeout,
                  num_stars;        /**< Number of timeout for this hop         */
    dynarray_t  * ttl_flows;        /**< ttl-flow_id tuples related to this hop */
    uint8_t       ttls[MAX_TTLS];   /**< All ttls related to this hop           */
    size_t        num_ttls;         /**< Number of ttls contained in this hop   */
    bool          enumeration_done;
    mda_lb_type_t type;             /**< Type of load balancer            */
} mda_interface_t;

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

/**
 * \brief Allocate a new mda_interface_t instance, which corresponds to
 *    an IP hop discovered by mda.
 * \param addr The address of the discovered IP hop.
 * \return A pointer to the newly created mda_interface_t instance,
 *    NULL otherwise.
 */

mda_interface_t * mda_interface_create(const address_t * address);

/**
 * \brief Release a mda_interface_t instance from the memory.
 * \param interface a preallocated mda_interface_t instance.
 */

void mda_interface_free(mda_interface_t * interface);

/**
 * \brief Allocate and attach a new mda_flow_t instance to a given
 *    mda_interface_t instance.
 * \param flow_id The new flow id.
 * \param flow_state The flow state.
 */

bool mda_interface_add_flow_id(mda_interface_t * interface, uint8_t ttl, uintmax_t flow_id, mda_flow_state_t state);

/**
 * \brief Retrieve the number of flows having a given state.
 * \param state The flow state. This is a value among {MDA_FLOW_AVAILABLE,
 *    MDA_FLOW_UNAVAILABLE, MDA_FLOW_TESTING, MDA_FLOW_TIMEOUT}.
 * \return The corresponding number of flows.
 */

size_t mda_interface_get_num_flows(const mda_interface_t * interface, mda_flow_state_t state);

/**
 * \brief Retrieve the next available flow ID related to an
 *    IP node discovered by mda.
 * \param interface An IP hop discovered by mda.
 */

// TODO
//uintmax_t mda_interface_get_new_flow_id(mda_interface_t *interface);

/**
 * \brief Retrieve an available flow id.
 * \param interface An IP hop discovered by mda.
 * \param num_siblings The number of interface hops discovered by mda at
 *    this TTL.
 * \param data A mda_data_t instance which stores the last used flow id.
 *    This last ID is updated.
 * \return A flow-id > 0 if successful, 0 otherwise.
 */

mda_ttl_flow_t * mda_interface_get_available_flow_id(mda_interface_t * interface, size_t num_siblings, mda_data_t * data);

/**
 * \brief Print to the standard output the flow related to a given
 *    mda_interface_t instance.
 * \param interface A mda_interface_t instance.
 */

void mda_flow_dump(const mda_interface_t * interface);

/**
 * \brief Print to the standard output a pair of mda_interface_t
 *    instances.
 * \param link Points to a pair of mda_interface_t interfaces
 *    (link[0] and link[1] must be set).
 * \param do_resolv Pass true to resolv IP address and print
 *    the corresponding FQDN.
 */

void mda_link_dump(const mda_interface_t ** link, bool do_resolv);

/**
 * \brief Callback used by lattice_dump
 * \param elt A lattice node instance
 * \param do_resolv Pass true to resolv IP address and print
 *    the corresponding FQDN.
 */

void mda_lattice_elt_dump(const lattice_elt_t * elt); //, bool do_resolv);

#endif // MDA_INTERFACE_H

