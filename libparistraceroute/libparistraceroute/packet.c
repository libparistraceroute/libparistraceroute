#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "packet.h"
#include "protocol.h"
#include "probe.h"

packet_t * packet_create(void) {
    return calloc(1, sizeof(packet_t));
}

void packet_free(packet_t * packet) {
    if (packet->dip) free(packet->dip);
    free(packet);
}

// Accessors

buffer_t * packet_get_buffer(packet_t *packet) {
    return packet->buffer;
}

void packet_set_buffer(packet_t * packet, buffer_t * buffer) {
    packet->buffer = buffer;
}

void packet_set_dip(packet_t * packet, char * dip) {
    packet->dip = dip;
}

void packet_set_dport(packet_t * packet, uint16_t dport) {
    packet->dport = dport;
}
