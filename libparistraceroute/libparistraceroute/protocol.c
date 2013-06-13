#include "protocol.h"

#include <string.h>         // strcmp(), ...
#include <search.h>         // tfind(), ...
#include <stdio.h>          // perror()


#include "common.h"         // ELEMENT_COMPARE
#include "protocol_field.h" // protocol_field_t

// static ?
void * protocols_root;

static int protocol_compare(
    const protocol_t * protocol1,
    const protocol_t * protocol2
) {
    return strcmp(protocol1->name, protocol2->name);
}

static int protocol_compare_id(
    const protocol_t * protocol1,
    const protocol_t * protocol2
) {
    return protocol1->protocol - protocol2->protocol;
}

const protocol_t * protocol_search(const char * name)
{
    protocol_t ** protocol, search;

    if (!name) return NULL;
    search.name = name;
    protocol = tfind(&search, &protocols_root, (ELEMENT_COMPARE) protocol_compare);

    return protocol ? *protocol : NULL;
}

const protocol_t * protocol_search_by_id(uint8_t id)
{
    protocol_t ** protocol, search;

    if (id == 58) {
        perror("protocol_search_by_id: fix this hack :)");
        return protocol_search("icmp6");
    }

    search.protocol = id;
    protocol = tfind(&search, &protocols_root, (ELEMENT_COMPARE) protocol_compare_id);

    return protocol ? *protocol : NULL;
}

void protocol_register(protocol_t * protocol)
{
    // Insert the protocol in the tree if the keys does not exist yet
    tsearch(protocol, &protocols_root, (ELEMENT_COMPARE) protocol_compare);
}

void protocol_write_header_callback(field_t * field, void * data)
{
    protocol_field_t * protocol_field;
    char *buf = data;

    protocol_field = NULL; // TODO search field->key

    memcpy(buf + protocol_field->offset, field->value.value, field_get_type_size(protocol_field->type));
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

void protocol_iter_fields(protocol_t *protocol, void *data, void (*callback)(protocol_field_t *field, void *data))
{
    // TODO iterate on protocol->fields until reaching .key = NULL and remove get_num_fields callback from each protocol
    size_t i, num_fields = protocol->get_num_fields();
    for (i = 0; i < num_fields; i++) {
        callback(&protocol->fields[i], data);
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

