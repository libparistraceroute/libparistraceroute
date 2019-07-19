#ifndef LIBPT_ALGORITHMS_TRACEROUTE_H
#define LIBPT_ALGORITHMS_TRACEROUTE_H

#include <stdbool.h>     // bool
#include <stdint.h>      // uint*_t
#include <stddef.h>      // size_t

#include "../address.h"  // address_t
#include "../pt_loop.h"  // pt_loop_t
#include "../dynarray.h" // dynarray_t
#include "../options.h"  // option_t

#define OPTIONS_TRACEROUTE_MIN_TTL_DEFAULT            1
#define OPTIONS_TRACEROUTE_MAX_TTL_DEFAULT            30
#define OPTIONS_TRACEROUTE_MAX_UNDISCOVERED_DEFAULT   3
#define OPTIONS_TRACEROUTE_NUM_QUERIES_DEFAULT        3
#define OPTIONS_TRACEROUTE_DO_RESOLV_DEFAULT          true
#define OPTIONS_TRACEROUTE_RESOLV_ASN_DEFAULT         false
#define OPTIONS_TRACEROUTE_PRINT_TTL_DEFAULT          false

#define OPTIONS_TRACEROUTE_MIN_TTL          {OPTIONS_TRACEROUTE_MIN_TTL_DEFAULT,          1, 255}
#define OPTIONS_TRACEROUTE_MAX_TTL          {OPTIONS_TRACEROUTE_MAX_TTL_DEFAULT,          1, 255}
#define OPTIONS_TRACEROUTE_MAX_UNDISCOVERED {OPTIONS_TRACEROUTE_MAX_UNDISCOVERED_DEFAULT, 1, 255}
#define OPTIONS_TRACEROUTE_NUM_QUERIES      {OPTIONS_TRACEROUTE_NUM_QUERIES_DEFAULT,      1, 255}

#define TRACEROUTE_HELP_A "Perform AS path lookups in routing registries and print results directly after the corresponding addresses."
#define TRACEROUTE_HELP_f "Start from the MIN_TTL hop (instead from 1), MIN_TTL must be between 1 and 255."
#define TRACEROUTE_HELP_m "Set the max number of hops (MAX_TTL to be reached). Default is 30, MAX_TTL must be between 1 and 255."
#define TRACEROUTE_HELP_n "Do not resolve IP addresses to their domain names"
#define TRACEROUTE_HELP_q "Set the number of probes per hop (default: 3)."
#define TRACEROUTE_HELP_PRINT_TTL "Print the TTL of the reply packet."
#define TRACEROUTE_HELP_M "Set the maximum number of consecutive unresponsive hops which causes the program to abort (default 3)."

#ifdef __cplusplus
extern "C" {
#endif

// Get the different values of traceroute options
uint8_t options_traceroute_get_min_ttl();
uint8_t options_traceroute_get_max_ttl();
uint8_t options_traceroute_get_num_queries();
uint8_t options_traceroute_get_max_undiscovered();
bool    options_traceroute_get_do_resolv();
bool    options_traceroute_get_print_ttl();
bool    options_traceroute_get_resolv_asn();

/*
 * Principle: (from man page)
 *
 * traceroute - print the route packets trace to network host
 *
 * traceroute  tracks the route packets taken from an IP network on
 * their way to a given host. It utilizes the IP protocol's time to
 * live (TTL) field and * attempts to elicit an ICMP TIME_EXCEEDED
 * response from each gateway along the path to the host.
 *
 * Algorithm:
 *
 *     INIT:
 *         cur_ttl = min_ttl
 *         SEND
 *
 *     SEND:
 *         send num_probes probes with TTL = cur_ttl
 *
 *     PROBE_REPLY:
 *         if < num_probes
 *             continue waiting
 *             if all_stars or destination_reached or stopping ICMP error
 *                 EXIT
 *             cur_ttl += 1
 *             SEND
 */

//--------------------------------------------------------------------
// Options
//--------------------------------------------------------------------

typedef struct {
    uint8_t           min_ttl;          /**< Minimum ttl at which to send probes. */
    uint8_t           max_ttl;          /**< Maximum ttl at which to send probes. */
    size_t            num_probes;       /**< Number of probes per hop.            */
    size_t            max_undiscovered; /**< Maximum number of consecutives undiscovered hops. */
    const address_t * dst_addr;         /**< The target IP. */
    bool              do_resolv;        /**< Resolv each discovered IP hop. */
    bool              print_ttl;      /**< Print the TTL of the reply. */
    bool              resolv_asn;       /**< Perform AS path lookups for each discovered IP hop. */
} traceroute_options_t;

const option_t * traceroute_get_options();

traceroute_options_t traceroute_get_default_options();

void    options_traceroute_init(traceroute_options_t * traceroute_options, address_t * address);

//--------------------------------------------------------------------
// Custom-events raised by traceroute algorithm
//--------------------------------------------------------------------

typedef enum {
    // event_type                      | data (type)     | data (meaning)
    // --------------------------------+-----------------+--------------------------------------------
    TRACEROUTE_DESTINATION_REACHED, // | NULL            | N/A
    TRACEROUTE_PROBE_REPLY,         // | probe_reply_t * | The probe and its corresponding reply
    TRACEROUTE_ICMP_ERROR,          // | probe_t *       | The probe which has provoked the ICMP error
    TRACEROUTE_STAR,                // | probe_t *       | The probe which has been lost
    TRACEROUTE_MAX_TTL_REACHED,     // | NULL            | N/A
    TRACEROUTE_TOO_MANY_STARS       // | NULL            | N/A
} traceroute_event_type_t;

// TODO since this structure should exactly match with a standard event_t, define a macro allowing to define custom events
// CREATE_EVENT(traceroute) uses traceroute_event_type_t and defines traceroute_event_t
typedef struct {
    traceroute_event_type_t type;
    void                  * data;
    void                 (* data_free)(void *); /**< Called in event_free to release data. Ignored if NULL. */
    void                  * zero;
} traceroute_event_t;

typedef struct {
    bool          destination_reached; /**< True iif the destination has been reached at least once for the current TTL */
    uint8_t       ttl;                 /**< TTL currently explored                   */
    uint8_t       attemptedTTL;        /**< The ttl as given to the client           */
    uint8_t       replyTTL;            /**< Current TTL sent back with the reply		 */
		double				rtt;								 /**< Current rtt                   					 */
		uint32_t			asn;								 /**< Current ASN if resolution is required		 */
    bool          newHop;              /**< Whether it's a new hop or not. If not, it's just a new probe           */
		char					ip[INET6_ADDRSTRLEN];/**< Current replying IP											 */
		char				* hostName;						 /**< Current host name. Should be freed if filled by discovered_ip_dump() */
    size_t        num_replies;         /**< Total of probe sent for this instance    */
    size_t        num_undiscovered;    /**< Number of consecutive undiscovered hops  */
    size_t        num_stars;           /**< Number of probe lost for the current hop */
    dynarray_t  * probes;              /**< Probe instances allocated by traceroute  */
} traceroute_data_t;

//-----------------------------------------------------------------
// Traceroute default handler
//-----------------------------------------------------------------

/**
 * \brief Handle raised traceroute_event_t events.
 * \param loop The main loop.
 * \param traceroute_event The handled event.
 * \param traceroute_options Options related to this instance of traceroute .
 * \param traceroute_data Data related to this instance of traceroute.
 */

void traceroute_handler(
    pt_loop_t                  * loop,
    traceroute_event_t         * traceroute_event,
    const traceroute_options_t * traceroute_options,
    traceroute_data_t    			 * traceroute_data
);

 
#ifdef __cplusplus
}
#endif

#endif // LIBPT_ALGORITHMS_TRACEROUTE_H
