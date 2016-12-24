#include "config.h"

#include <string.h>         // strcmp(), ...
#include "os/search.h"      // tfind(), tdestroy(), twalk(), preorder...
#include <stdio.h>          // perror()

#include "protocol.h"

#include "common.h"         // ELEMENT_COMPARE
#include "protocol_field.h" // protocol_field_t
#include "layer.h"          // layer_t, layer_extract()

// Protocols are registered in the following trees.
// We require two trees since a protocol may be retrieved
// by using either its name or its protocol_id.

static void * protocols_root    = NULL;  /**< Tree ordered by name */
static void * protocols_id_root = NULL;  /**< Tree ordered by id   */

static void protocol_clear() __attribute__((destructor));

static int protocol_compare(
    const protocol_t * protocol1,
    const protocol_t * protocol2
) {
    return strcmp(protocol1->name, protocol2->name);
}

static int protocol_id_compare(
    const protocol_t * protocol1,
    const protocol_t * protocol2
) {
    return protocol1->protocol - protocol2->protocol;
}

static void nothing_to_free() {}

const protocol_t * protocol_search(const char * name) {
    protocol_t ** protocol, search;

    if (!name) return NULL;
    search.name = name;
    protocol = tfind(&search, &protocols_root, (ELEMENT_COMPARE) protocol_compare);

    return protocol ? *protocol : NULL;
}

const protocol_t * protocol_search_by_id(uint8_t id) {
    protocol_t ** protocol, search;

    search.protocol = id;
    protocol = tfind(&search, &protocols_id_root, (ELEMENT_COMPARE) protocol_id_compare);

    return protocol ? *protocol : NULL;
}

void protocol_register(protocol_t * protocol) {
    // Insert the protocol in the tree if the keys does not exist yet
    tsearch(protocol, &protocols_root,    (ELEMENT_COMPARE) protocol_compare);
    tsearch(protocol, &protocols_id_root, (ELEMENT_COMPARE) protocol_id_compare);
}

static void protocol_clear() {
    tdestroy(protocols_root, nothing_to_free);
    tdestroy(protocols_id_root, nothing_to_free);
}

const protocol_field_t * protocol_get_field(const protocol_t * protocol, const char * name)
{
    protocol_field_t * protocol_field;

    for (protocol_field = protocol->fields; protocol_field->key; protocol_field++) {
        if (strcmp(protocol_field->key, name) == 0) {
            return protocol_field;
        }
    }
    return NULL;
}

void protocol_iter_fields(
    const protocol_t * protocol,
    void             * data,
    void            (* callback)(const protocol_field_t * field, void * data)
) {
    const protocol_field_t * protocol_field;
    for (protocol_field = protocol->fields; protocol_field->key; protocol_field++) {
        callback(protocol_field, data);
    }
}

uint16_t csum(const uint16_t * bytes, size_t size) {
    // Adapted from http://www.netpatch.ru/windows-files/pingscan/raw_ping.c.html
    uint32_t sum = 0;

    while (size > 1) {
        sum += *bytes++;
        size -= sizeof(uint16_t);
    }
    if (size) {
        sum += * (const uint8_t *) bytes;
    }
    sum  = (sum >> 16) + (sum & 0xffff);
    sum += (sum >> 16);
    return (uint16_t) ~sum;
}

static inline void callback_protocol_field_dump(const protocol_field_t * protocol_field, void * data) {
    protocol_field_dump(protocol_field);
}

void protocol_dump(const protocol_t * protocol) {
    printf("%3d %s\n", protocol->protocol, protocol->name);
//    protocol_iter_fields(protocol, NULL, callback_protocol_field_dump);
}

static void callback_protocols_dump(const void * node, VISIT visit, int level) {
    const protocol_t * protocol;

    switch (visit) {
        case leaf:      // 1st visit (leaf)
        case postorder: // 3rd visit
            protocol = *((protocol_t * const *) node);
            protocol_dump(protocol);
            break;
        default:        // endorder (2nd visit) and preorder (1st visit, not leaf)
            break;
    }
}

void protocols_dump() {
    twalk(protocols_root, callback_protocols_dump);
}

const protocol_t * protocol_get_next_protocol(const layer_t * layer) {
    const protocol_t * next_protocol = NULL;
    uint8_t            next_protocol_id;

    if (!layer_extract(layer, "protocol", &next_protocol_id)) {
        // This protocol does not have "protocol" field so we stop to dissect
        // the packet. If the library should continue to dissect the packet
        // the current protocol must implement a get_next_protocol callback.
        return NULL;
    }

    if (!(next_protocol = protocol_search_by_id(next_protocol_id))) {
        fprintf(stderr, "Protocol not supported (protocol_id = %d)\n", next_protocol_id);
        protocols_dump();
        return NULL;
    }

    return next_protocol;
}


