#ifndef PROTOCOL_H
#define PROTOCOL_H

#include <stdbool.h>

#include "packet.h"
#include "protocol_field.h"
#include "pseudoheader.h"

typedef struct {
    char* name;
    unsigned int (*get_num_fields)(void);
    bool (*need_ext_checksum)(void);
	int (*write_checksum)(unsigned char *buf, pseudoheader_t *psh);
    // create_pseudo_header
    protocol_field_t *fields;
    void (*write_default_header)(char *data);
    //socket_type
    unsigned int (*get_header_size)(void);
} protocol_t;

protocol_t* protocol_search(char *name);
void protocol_register(protocol_t *protocol);

int protocol_write_header(protocol_t *protocol, probe_t *probe, char *buf);

void protocol_iter_fields(protocol_t *protocol, void *data, void (*callback)(protocol_field_t *field, void *data));


/**
 * function to calc checksums (my be multi protocol)
 * @param len the size of header in bytes 
 * @param buff the buffer
 * @return the checksum
 */
unsigned short csum (unsigned short *buf, int nwords);

#define PROTOCOL_REGISTER(MOD)	\
static void __init_ ## MOD (void) __attribute__ ((constructor));	\
static void __init_ ## MOD (void) {	\
	protocol_register(&MOD); \
}

#endif
