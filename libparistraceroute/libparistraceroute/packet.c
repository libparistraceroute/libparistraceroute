#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "packet.h"
#include "protocol.h"
#include "probe.h"

packet_t* packet_create(void)
{
    packet_t *packet;

    packet = (packet_t*)malloc(sizeof(packet_t));
    packet->size = 0;

    return packet;
}

packet_t* packet_create_from_probe(probe_t *probe)
{
    unsigned int num_proto, i;
    unsigned int payload_size;
    char *data, *payload;
    char **offsets;
    packet_t *packet;
    unsigned int offset = 0;

    packet = packet_create();
    
    num_proto = probe_get_num_proto(probe);
    offsets = (char**)malloc(num_proto * sizeof(char*));

    /* Writing packet headers */
    data = packet->data;
	for(i = 0; i < num_proto; i++) {
        protocol_t *protocol;
        char *name;

        

        name = probe_get_protocol_by_index(i);
        protocol = protocol_search(name);
        offset = protocol_write_header(protocol, probe, data); // calls after_filling
        offsets[i] = data;

        data += offset;
    }

    /* Writing payload */
    payload = probe_get_payload(probe);
    payload_size = probe_get_payload_size(probe);

    if (payload_size > MAXBUF - offset);
        payload_size = MAXBUF - offset;

    if (payload) {
		memcpy(data, payload, payload_size);
    } else {
        *data = 0; // BAD
	}
	
    /* Compute checksums */
	unsigned char** psd_hed = malloc(sizeof(char*));
	*psd_hed=NULL;
	int* size_psd = malloc(sizeof(int));
	*size_psd=0;
	char* offset_data;
	char* offset_prev;

    /* We take them in the reverse order */

	for(i = num_proto - 1; i >= 0; i--) {
        protocol_t *protocol;
        char *name;

        name = probe_get_protocol_by_index(i);
        protocol = protocol_search(name);

        if (protocol->need_ext_checksum()) {
            protocol_t *protocol_prev;
            char *name_prev;
            pseudoheader_t *psh;


            if (i < 1) {
                printf("ERROR\n");
            }
            name_prev = probe_get_protocol_by_index(i);
            protocol_prev = protocol_search(name_prev);

            psh = NULL; //protocol->create_pseudo_header(protocol_prev, offsets[i-1]);
            protocol->write_checksum(data, psh);
            
            free(psh); 
            psh = NULL;
        } else {
            protocol->write_checksum(data, NULL);
        }

//		//create the fields containing info from packet
//		offset_data = (*packet->data)+offsets[i];
//		*psd_hed=NULL;
//		*size_psd=0;
//		if(tab[i]->need_ext_checksum()){
//			if(i==0) return -1;
//			offset_prev = (*packet->data)+packet->offsets[i-1];
//			int res_int = tab[i-1]->create_pseudo_header(&offset_prev,tab[i]->name,psd_hed,size_psd);
//			if(res_int ==-1){
//				return -1;
//			}
//		}
//	    protocol->compute_checksum(&offset_data,packet->size-packet->offsets[i],psd_hed,*size_psd);
//        // calls after checksum
//		if(tab[i]->after_checksum!=NULL){
//			tab[i]->after_checksum(&offset_data);
//		}
//		free(*psd_hed);
	}
	//free(psd_hed);
	//free(size_psd);
	return 0;
}

void packet_free(packet_t *packet)
{
    free(packet->data);
    free(packet);
    packet = NULL;
}
