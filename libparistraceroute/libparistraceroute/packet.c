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
    if (packet->dst_ip) free(packet->dst_ip);
    free(packet);
}

// Accessors

buffer_t * packet_get_buffer(packet_t *packet) {
    return packet->buffer;
}

void packet_set_buffer(packet_t * packet, buffer_t * buffer) {
    packet->buffer = buffer;
}

packet_t * packet_create_from_probe(probe_t * probe)
{
    char     * dst_ip;
    uint16_t   dst_port;
    packet_t * packet;
    
    // The destination IP is a mandatory field
    if (!(probe_extract(probe, "dst_ip", &dst_ip))) {
        perror("packet_create_from_probe: this probe has no 'dst_ip' field\n");
        goto ERR_EXTRACT_DST_IP;
    }

    // The destination port is a mandatory field
    if (!(probe_extract(probe, "dst_port", &dst_port))) {
        perror("packet_create_from_probe: this probe has no 'dst_port' field\n");
        goto ERR_EXTRACT_DST_PORT;
    }

    // Create the corresponding packet
    if (!(packet = packet_create())) {
        goto ERR_PACKET_CREATE;
    }
    packet->dst_ip = dst_ip;
    packet->dst_port = dst_port;
    packet_set_buffer(packet, probe_get_buffer(probe));

    // Do not free dst_ip since it is not dup in packet_set_dip()
    return packet;

ERR_PACKET_CREATE:
ERR_EXTRACT_DST_PORT:
    free(dst_ip);
ERR_EXTRACT_DST_IP:
    return NULL;
}


