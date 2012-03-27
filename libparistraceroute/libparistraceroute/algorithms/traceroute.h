#ifndef ALGORITHMS_TRACEROUTE_H
#define ALGORITHMS_TRACEROUTE_H

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
 * --------------------------------------------------------------------
 * Uses
 * --------------------------------------------------------------------
 *
 * - probe/reply: IP  / ICMP TTL expired
 *
 *      This means that we send IP packet and we're expecting ICMP TTL
 *      expired responses.
 *
 * - tag: TCP / UDP ports
 *
 *      Where we store the identifier of each sent probes.
 */

/* --------------------------------------------------------------------
 * Options
 * --------------------------------------------------------------------
 */

typedef struct {
    unsigned min_ttl;    /**< Minimum ttl at which to send probes */
    unsigned max_ttl;    /**< Maximum ttl at which to send probes */
    unsigned num_probes; /**< Number of probes per hop            */
} traceroute_options_t;

// Default options

static traceroute_options_t traceroute_default = {
    .min_ttl    = 1,
    .max_ttl    = 30,
    .num_probes = 3,
};

/* --------------------------------------------------------------------
 * Algorithm
 * --------------------------------------------------------------------
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

/* --------------------------------------------------------------------
 * Probe output
 * --------------------------------------------------------------------
 *
 *     A set of probes and their replies
 *
 */

/* --------------------------------------------------------------------
 * Interpretation output
 * --------------------------------------------------------------------
 *
 *     The set of interfaces met at each ttl, in order, annotated with
 *     DNS and RTT or ICMP errors * or stars
 */

#endif
