#ifndef MDA_DATA_H
#define MDA_DATA_H

#include "../../lattice.h"
#include "../../pt_loop.h"
#include "../../probe.h"

typedef enum {
    TRACEROUTE_DESTINATION_REACHED,
    TRACEROUTE_ALL_STARS,
    TRACEROUTE_PROBE_REPLY,
    TRACEROUTE_ICMP_ERROR
} traceroute_event_type_t;

typedef struct {
    unsigned     min_ttl;    /**< Minimum ttl at which to send probes */
    unsigned     max_ttl;    /**< Maximum ttl at which to send probes */
    unsigned     num_probes; /**< Number of probes per hop            */
    const char * dst_ip;     /**< The target IP */
} traceroute_options_t;

typedef struct {
    // Mandatory field
    const traceroute_options_t * options;  /**< Options passed to this instance */
    // Event-specific fields
    const char * discovered_ip;    /**< Discovered IP */
    unsigned     current_ttl;      /**< Current TTL */
    unsigned     num_sent_probes;  /**< This is i-th probe sent for this TTL */
} traceroute_probe_reply_t;

typedef union {
    traceroute_probe_reply_t probe_reply;
} traceroute_event_value_t;

typedef struct {
    traceroute_event_type_t  type;
    traceroute_event_value_t value;
} traceroute_caller_data_t;

typedef struct {
    /* Root of the lattice storing the interfaces */
    lattice_t *lattice;

    uintmax_t last_flow_id;

    unsigned int confidence;

    char * dst_ip;
    pt_loop_t *loop;
    probe_t *skel;
} mda_data_t;

mda_data_t* mda_data_create(void);
void mda_data_free(mda_data_t *data);

#endif /* MDA_DATA_H */

