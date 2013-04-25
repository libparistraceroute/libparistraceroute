#include <string.h>
#include <search.h>
#include "protocol.h"
#include "probe.h"
#include "protocol_field.h"
#include "field.h"
#include "buffer.h"
#include <stdio.h>
/* static ? */
void *protocols_root;

int protocol_compare(const void *protocol1, const void *protocol2)
{
    return strcmp (((const protocol_t*)protocol1)->name,
            ((const protocol_t*)protocol2)->name);
}

int protocol_compare_id(const void *protocol1, const void *protocol2)
{
    return ((const protocol_t*)protocol1)->protocol - ((const protocol_t*)protocol2)->protocol;
}


     

protocol_t* protocol_search(char *name)
{
    protocol_t **protocol, search;

    if (!name) return NULL;

    search.name = name;
    protocol = tfind((const void*)&search, (void* const*)&protocols_root,
            protocol_compare);

    return protocol ? *protocol : NULL;
}

protocol_t* protocol_search_by_id(uint8_t id)
{
    protocol_t **protocol, search;
   // Ugly hack XXX
    if (id == 58){
	return protocol_search("icmp6");
    }

    search.protocol = id;
    protocol = tfind((const void*)&search, (void* const*)&protocols_root,
            protocol_compare_id);

    return protocol ? *protocol : NULL;

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

    memcpy(buf + protocol_field->offset, field->value.value, field_get_type_size(protocol_field->type));
}

protocol_field_t * protocol_get_field(protocol_t *protocol, const char *name)
{
    protocol_field_t *pfield;
    
    for(pfield = protocol->fields; pfield->key; pfield++) {
        if (strcmp(pfield->key, name) == 0) {
            return pfield;
        }
    }
    return NULL;
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

