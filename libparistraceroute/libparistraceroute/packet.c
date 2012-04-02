#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "packet.h"
#include "protocol.h"
#include "probe.h"

/**
 * \brief Allocate a packet
 *
 */
packet_t * packet_create(void)
{
    packet_t * packet = malloc(sizeof(packet_t));
    if (!packet)
        goto error;
    packet->buffer = NULL;
    packet->dip = NULL;
    packet->dport = 0;

    return packet;

error:
    return NULL;
}

void packet_free(packet_t *packet)
{
    free(packet);
    packet = NULL;
}

// Accessors

buffer_t * packet_get_buffer(packet_t *packet)
{
    return packet->buffer;
}

int packet_set_buffer(packet_t *packet, buffer_t *buffer)
{
    packet->buffer = buffer;

    return 0;
}

int packet_set_dip(packet_t *packet, unsigned char *dip)
{
    packet->dip = dip;
    return 0;
}

int packet_set_dport(packet_t *packet, unsigned short dport)
{
    packet->dport = dport;
    return 0;
}
