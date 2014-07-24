#ifndef ALGORITHMS_PING_H
#define ALGORITHMS_PING_H

#include <stdbool.h>     // bool
#include <stdint.h>      // uint*_t
#include <stddef.h>      // size_t
#include <limits.h>      // INT_MAX

#include "../address.h"  // address_t
#include "../pt_loop.h"  // pt_loop_t
#include "../dynarray.h" // dynarray_t
#include "../options.h"  // option_t

#define OPTIONS_PING_MAX_TTL_DEFAULT                  255
#define OPTIONS_PING_PACKET_SIZE_DEFAULT              0
#define OPTIONS_PING_SHOW_TIMESTAMP_DEFAULT           false
#define OPTIONS_PING_IS_QUIET_DEFAULT                 false
#define OPTIONS_PING_COUNT_DEFAULT                    INT_MAX
#define OPTIONS_PING_DO_RESOLV_DEFAULT                true
#define OPTIONS_PING_INTERVAL_DEFAULT                 1

#define OPTIONS_PING_MAX_TTL                {OPTIONS_PING_MAX_TTL_DEFAULT,     1, 255}
#define OPTIONS_PING_PACKET_SIZE            {OPTIONS_PING_PACKET_SIZE_DEFAULT, 0, INT_MAX}
#define OPTIONS_PING_COUNT                  {OPTIONS_PING_COUNT_DEFAULT,       1, OPTIONS_PING_COUNT_DEFAULT}

#define PING_HELP_c      "Stop after sending count ECHO_REQUEST packets. With deadline option, ping waits for 'count' ECHO_REPLY packets, until the timeout expires."
#define PING_HELP_D      "Print timestamp (unix time + microseconds as in gettimeofday) before each line."
#define PING_HELP_n      "Do not resolve IP addresses to their domain names"
#define PING_HELP_q      "Quiet output. Nothing is displayed except the summary lines at startup time and when finished."
#define PING_HELP_v      "Verbose output."
#define PING_HELP_t      "Set the IP Time to Live."

// Get the different values of ping options
bool         options_ping_get_do_resolv();
double       options_ping_get_interval();
bool         options_ping_get_show_timestamp();
bool         options_ping_get_is_quiet();
unsigned int options_ping_get_count();

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
    uint8_t           max_ttl;          /**< Maximum ttl at which to send probes */ 
    unsigned int      count;            /**< Number of probes to be sent         */
    const address_t * dst_addr;         /**< The target IP */
    bool              do_resolv;        /**< Resolv each discovered IP hop */
    double            interval;         /**< The time to wait to send each packet; in seconds */
    bool              is_quiet;         /**< If enabled, only summary lines at startup time and when finished are shown */
    bool              show_timestamp;   /**< If enabled, timestamp is shown */
} ping_options_t;

const option_t * ping_get_options();

ping_options_t ping_get_default_options();

void options_ping_init(ping_options_t * ping_options, address_t * address, double interval, uint8_t max_ttl);

//--------------------------------------------------------------------
// Custom-events raised by ping algorithm
//--------------------------------------------------------------------

typedef enum {
    // event_type                       | data (type)     | data (meaning)
    // ---------------------------------+-----------------+--------------------------------------------
    PING_PROBE_REPLY,                // | probe_reply_t * | The probe and its corresponding reply
    PING_PRINT_STATISTICS,           // | ping_data_t *   | The data of the algorithm
    PING_DST_NET_UNREACHABLE,        // | probe_reply_t * | The probe and its corresponding reply
    PING_DST_HOST_UNREACHABLE,       // | probe_reply_t * | The probe and its corresponding reply
    PING_DST_PROT_UNREACHABLE,       // | probe_reply_t * | The probe and its corresponding reply
    PING_DST_PORT_UNREACHABLE,       // | probe_reply_t * | The probe and its corresponding reply
    PING_TTL_EXCEEDED_TRANSIT,       // | probe_reply_t * | The probe and its corresponding reply
    PING_TIME_EXCEEDED_REASSEMBLY,   // | probe_reply_t * | The probe and its corresponding reply
    PING_REDIRECT,                   // | probe_reply_t * | The probe and its corresponding reply
    PING_PARAMETER_PROBLEM,          // | probe_reply_t * | The probe and its corresponding reply
    PING_GEN_ERROR,                  // | probe_reply_t * | The probe and its corresponding reply
    PING_TIMEOUT,                    // | NULL            | N/A
    PING_ALL_PROBES_SENT,            // | NULL            | N/A
} ping_event_type_t;

// TODO since this structure should exactly match with a standard event_t, define a macro allowing to define custom events
// CREATE_EVENT(ping) uses ping_event_type_t and defines ping_event_t
typedef struct {
    ping_event_type_t       type;
    void                  * data;
    void                 (* data_free)(void *); /**< Called in event_free to release data. Ignored if NULL. */
    void                  * zero;
} ping_event_t;

typedef struct {
    size_t        num_replies;          /**< Total of probe sent for this instance    */
    size_t        num_losses;           /**< Number of packets lost                   */
    size_t        num_probes_in_flight; /**<The number of probes which haven't provoked a reply so far */
    dynarray_t  * rtt_results;          /**<RTTs in order to be able to compute statistics */ 
    size_t        num_sent;
} ping_data_t;

void ping_dump_statistics(ping_data_t * ping_data);

//-----------------------------------------------------------------
// Ping default handler
//-----------------------------------------------------------------

/**
 * \brief Handle raised ping_event_t events.
 * \param loop The main loop.
 * \param ping_event The handled event.
 * \param ping_options Options related to this instance of ping .
 * \param ping_data Data related to this instance of ping.
 */

void ping_handler(
    pt_loop_t            * loop,
    ping_event_t         * ping_event,
    const ping_options_t * ping_options,
    ping_data_t          * ping_data
);

#endif
