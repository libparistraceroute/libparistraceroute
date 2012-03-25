#include <string.h>
#include <search.h>
#include "protocol.h"
#include "probe.h"
#include "protocol_field.h"
#include "field.h"

/* static ? */
void *protocols_root;

int protocol_compare(const void *protocol1, const void *protocol2)
{
    return strcmp (((const protocol_t*)protocol1)->name,
            ((const protocol_t*)protocol2)->name);
}

protocol_t* protocol_search(char *name)
{
    protocol_t *protocol, search;

    if (!name) return;

    search.name = name;
    protocol = tfind((const void*)&search, (void* const*)&protocols_root,
            protocol_compare);

    return protocol;
}

void protocol_register(protocol_t *protocol)
{
    /* insert the protocol in the tree if the keys does not exist yet */
    (void) tsearch((const void *)protocol, (void **)&protocols_root,
            protocol_compare);
}

void protocol_write_header_callback(field_t *field, void *data)
{
    protocol_field_t *protocol_field;
    char *buf = data;

    protocol_field = NULL; // TODO search field->key

    memcpy(buf + protocol_field->offset, field->value, field_get_type_size(protocol_field->type));
}

int protocol_write_header(protocol_t *protocol, probe_t *probe, char *buf)
{
    field_t **fields;
    unsigned int num_fields, i;
    
    /* Initialize with default values */
    protocol->write_default_header(buf);

    probe_iter_fields(probe, buf, protocol_write_header_callback);
}

int protocol_get_field(protocol_t *protocol, char *name, packet_t *packet)
{
    field_t *field;
    char *key = strdup(name);
    protocol_field_t * protocol_field = NULL; // search

    switch(protocol_field->type) {
        case TYPE_INT16:
            field = field_create_int16(key, (uint16_t)(*(packet->data + protocol_field->offset)));
            break;
        case TYPE_INT32:
            field = field_create_int16(key, (uint32_t)(*(packet->data + protocol_field->offset)));
            break;
        default:
            break;
    }
    field->type = protocol_field->type;
}

void protocol_iter_fields(protocol_t *protocol, void *data, void (*callback)(protocol_field_t *field, void *data))
{
    unsigned int num_fields, i;
    
    num_fields = protocol->get_num_fields();
    for (i=0; i < num_fields; i++) {
        callback(&protocol->fields[i], data);
    }

}

//from http://mixter.void.ru/rawip.html
u_int16_t csum (u_int16_t *buf, int nwords){ //nword = taille du paquet (divisé par 2 ( >>1) pour faire des words)
  unsigned long sum;
  for (sum = 0; nwords > 0; nwords--)
    sum += *buf++;
  sum = (sum >> 16) + (sum & 0xffff);
  sum += (sum >> 16);
  return (u_int16_t)~sum;
}

