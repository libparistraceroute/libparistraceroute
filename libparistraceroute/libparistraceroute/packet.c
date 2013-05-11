#include <stdlib.h>
#include <sys/socket.h> // AF_INET, AF_INET6

#include "packet.h"

packet_t * packet_create(void) {
    return calloc(1, sizeof(packet_t));
}

void packet_free(packet_t * packet) {
    if (packet->dst_ip) free(packet->dst_ip);
    free(packet);
}

int packet_guess_address_family(const packet_t * packet) {
    int family = 0;

    switch (packet->buffer->data[0] >> 4) {
        case 4: family = AF_INET;  break;
        case 6: family = AF_INET6; break;
        default: break;
    }
    return family;
}

// Accessors

buffer_t * packet_get_buffer(packet_t *packet) {
    return packet->buffer;
}

void packet_set_buffer(packet_t * packet, buffer_t * buffer) {
    packet->buffer = buffer;
}

