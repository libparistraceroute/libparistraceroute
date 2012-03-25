#ifndef PACKET_H
#define PACKET_H

#include "probe.h"

#define MAXBUF 10000

typedef struct {
    char data[MAXBUF];
    unsigned int size;
} packet_t;

packet_t* packet_create(void);
packet_t* packet_create_from_probe(probe_t *probe);
void packet_free(packet_t *packet);

#endif
