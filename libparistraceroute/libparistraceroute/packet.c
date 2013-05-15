#include <stdlib.h>     // malloc, calloc, free
#include <string.h>     // strdup
#include <sys/socket.h> // AF_INET, AF_INET6

#include "packet.h"

packet_t * packet_create(void) {
    packet_t * packet;

    if (!(packet = calloc(1, sizeof(packet_t)))) goto ERR_CALLOC;
    if (!(packet->buffer = buffer_create()))     goto ERR_BUFFER;
    return packet;

ERR_BUFFER:
    free(packet);
ERR_CALLOC:
    return NULL; 
}

packet_t * packet_wrap_bytes(uint8_t * bytes, size_t num_bytes) {
    packet_t * packet;

    if ((packet = packet_create())) { 
        packet->buffer->data = bytes;
        packet->buffer->size = num_bytes;
    }
    return packet;
}

packet_t * packet_create_from_bytes(uint8_t * bytes, size_t num_bytes) {
    packet_t * packet;

    if ((packet = packet_create())) { 
        buffer_write_bytes(packet->buffer, bytes, num_bytes);
    }
    return packet;
}

packet_t * packet_dup(const packet_t * packet) {
    packet_t * ret = NULL;
    if ((ret = malloc(sizeof(packet_t)))) {
        if (!(ret->buffer = buffer_dup(packet->buffer))) goto ERR_BUFFER_DUP;
        if (packet->dst_ip) {
            if (!(ret->dst_ip = strdup(packet->dst_ip))) goto ERR_DST_IP_DUP;
        } else ret->dst_ip = NULL;
        ret->dst_port = packet->dst_port;
    }
    return ret;

ERR_DST_IP_DUP:
    buffer_free(ret->buffer);
ERR_BUFFER_DUP:
    return NULL;
}

void packet_free(packet_t * packet) {
    if (packet) {
        if (packet->buffer) buffer_free(packet->buffer);
        if (packet->dst_ip) free(packet->dst_ip);
        free(packet);
    }
}

bool packet_resize(packet_t * packet, size_t new_size) {
    return buffer_resize(packet->buffer, new_size);
}

uint8_t * packet_get_bytes(packet_t * packet) {
    return buffer_get_data(packet->buffer);
}

size_t packet_get_size(const packet_t * packet) {
    return buffer_get_size(packet->buffer);
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

buffer_t * packet_get_buffer(packet_t * packet) {
    return packet->buffer;
}

void packet_set_buffer(packet_t * packet, buffer_t * buffer) {
    packet->buffer = buffer;
}

