
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <netinet/ip6.h>
#include <arpa/inet.h> 

#include "packet.h"

//number of protocols for a packet (ex : IPv4/udp -> 2 protocols)
#define NUMBER_PROTO_DEFAULT 2

//TODO : fonction ajout d'options dans en tete IP?



packet_s* create_packet(int payload_size, char** protos, int nb_protos){
	if(payload_size<0){
		#ifdef DEBUG
		fprintf(stderr,"create_packet : argument of size is negative, putting it to 0\n");
		#endif
		payload_size=0;
	}
	//char** data=NULL;
	char** data=malloc(sizeof(char*));
	if(data==NULL){
		#ifdef DEBUG
		fprintf(stderr,"create_packet : error : no space available to malloc\n");
		#endif
		return NULL;
	}
	*data=NULL;
	packet_s* packet = malloc(sizeof(packet_s));
	if(packet==NULL){
		#ifdef DEBUG
		fprintf(stderr,"create_packet : error : no space available to malloc\n");
		#endif
		return NULL;
	}
	packet->data=data;
	int* offs = malloc((nb_protos)*sizeof(int));
	memset(offs, 0, 4*(nb_protos));
	packet->offsets=offs;
	packet->size=payload_size;
	packet->payload_size=payload_size;
	packet->protos = protos;
	packet->nb_protos=nb_protos;
	return packet;
}

void view_packet(packet_s* packet){
	int i=0,j=0;
	char** data = packet->data;
	fprintf(stdout,"information about packet\n");
	fprintf(stdout,"size : %d with %d bytes of payload\n",packet->size,packet->payload_size);
	fprintf(stdout,"containing protocols :");
	for(i=0; i<packet->nb_protos; i++){
		fprintf(stdout,"%s -- ",packet->protos[i]);
	}
	fprintf(stdout,"\ncontained data :\n");
	for(i=0;i<packet->size;i=i+8){
		if(packet->size-i<8){
			for(j=0;j<packet->size-i;j++){
				fprintf(stdout,"%02x ",(unsigned char) (*data)[j+i]);
			}
			fprintf(stdout,"\n");
		}
		else{
			fprintf(stdout,"%02x %02x %02x %02x %02x %02x %02x %02x\n",(unsigned char) (*data)[i],(unsigned char) (*data)[i+1],(unsigned char) (*data)[i+2],(unsigned char) (*data)[i+3],(unsigned char) (*data)[i+4],(unsigned char) (*data)[i+5],(unsigned char) (*data)[i+6],(unsigned char) (*data)[i+7]);
		}
	}
}

void free_packet(packet_s* packet){
	free(packet->data);
	free(packet->offsets);
	free(packet);
}


field_s* create_proto_infos_fields(packet_s* packet, int actual_proto, int offset_packet, const proto_s** tab){
	int *total_lenght = malloc(sizeof(int));
	*total_lenght = packet->size-offset_packet;//lenght of the packet without superior headers
	field_s* fields = create_field("total_lenght",total_lenght);
	#ifdef DEBUG
	fprintf(stdout,"creating fields : total_lenght created\n");
	#endif
	if(actual_proto>0){
		char* prev_proto = packet->protos[actual_proto-1];
		char** previous_proto = malloc(sizeof(char*));
		*previous_proto = malloc((strlen(prev_proto)+1)*sizeof(char));
		strncpy(*previous_proto,prev_proto,strlen(prev_proto)+1);
		fields = add_field(fields,"prev_proto",previous_proto);//for pseudo header
		#ifdef DEBUG
		fprintf(stdout,"creating fields : prev_proto created\n");
		#endif
	}
	if(actual_proto<(packet->nb_protos-1)){
		char* fol_proto = packet->protos[actual_proto+1];
		char** next_proto = malloc(sizeof(char*));
		*next_proto = malloc((strlen(fol_proto)+1)*sizeof(char));
		strncpy(*next_proto,fol_proto,strlen(fol_proto)+1);
		fields = add_field(fields,"next_proto",next_proto);//for pseudo header
		#ifdef DEBUG
		fprintf(stdout,"creating fields : next_proto created\n");
		#endif
	}
	#ifdef DEBUG
	fprintf(stdout,"creating fields : over\n");
	#endif
	return fields;
}


int fill_packet(packet_s* packet, fieldlist_s* fields, unsigned short* checksums){
	field_s* current_fieldlist_packet;
	if (packet->nb_protos==0||packet->protos==NULL){
		#ifdef DEBUG
		fprintf(stderr,"fill packet : no protocol specified.end\n");
		#endif
		return -1;
	}
	const proto_s* tab[packet->nb_protos];
	int i = 0;
	#ifdef DEBUG
	fprintf(stdout,"fill packet : %i protocols to add\n",packet->nb_protos);
	#endif
	//get the functions in a array and estimate size of packet
	for(i = 0; i<packet->nb_protos; i++){
		#ifdef DEBUG
		fprintf(stderr,"fill packet : associating protocol %s\n",packet->protos[i]);
		#endif
		tab[i]= choose_protocol(packet->protos[i]);
		if(tab[i]==NULL){
			#ifdef DEBUG
			fprintf(stderr,"fill packet : protocol %s has no options associatied.end\n",packet->protos[i]);
			#endif	
			return -1;
		}
		#ifdef DEBUG
		fprintf(stdout,"fill packet : initialisation of protocol %s done\n",packet->protos[i]);
		#endif	
		packet->size=packet->size+tab[i]->get_header_size(packet->data, fields);
	}
	#ifdef DEBUG
	fprintf(stdout,"fill packet : init of packet protos done. final size : %i\n",packet->size);
	#endif
	*(packet->data)=malloc(packet->size*sizeof(char));
	memset (*(packet->data), 0, packet->size);
	#ifdef DEBUG
	fprintf(stdout,"fill packet : data initialised.\n");
	#endif
	//int offsets[packet->nb_protos];
	int offset_packet=0;//offset from beginning of data
	//fill the headers
	for(i = 0; i<packet->nb_protos; i++){
		packet->offsets[i]=offset_packet;
		//create the fields containing info from packet
		#ifdef DEBUG
		fprintf(stdout,"fill packet : protocol %s : \n",packet->protos[i]);
		fflush(stdout);
		#endif
		current_fieldlist_packet = create_proto_infos_fields(packet,i,offset_packet, tab);
		#ifdef DEBUG
		//fprintf(stdout,"fill packet : packet info for protocol %s : \n",packet->protos[i]);
		//fflush(stdout);
		//view_fields(current_fieldlist_packet);
		#endif
		fields = add_fieldlist(fields,"infos_packet",current_fieldlist_packet);
		char* offset_data = (*packet->data)+offset_packet;
		//if(tab[i]->fill_packet_header(fields,packet->data+offset_packet)==-1){
		if(tab[i]->fill_packet_header(fields,&offset_data)==-1){
			#ifdef DEBUG
			fprintf(stderr,"fill_packet : missing mandatory fields in %s protocol.end\n",tab[i]->name);
			#endif
			return -1;
		}
		if(tab[i]->after_filling!=NULL){
			tab[i]->after_filling(&offset_data);
		}
		#ifdef DEBUG
		fprintf(stdout,"fill_packet : calculating new offset\n");
		#endif
		char* datagram = *(packet->data)+offset_packet;
		offset_packet=offset_packet+tab[i]->get_header_size(&datagram, fields); //next offset
		//no need to free the fieldlist of packet, will be updated
		#ifdef DEBUG
		fprintf(stdout,"fill packet : new offset : %d. Protocol %s filled\n",offset_packet,packet->protos[i]);
		#endif
		if(remove_fieldlist(&fields,"infos_packet",1)==-1){//to delete traces at the end
			#ifdef DEBUG
			fprintf(stderr,"fill packet : remove of infos failed\n");
			#endif
		}
	}
	//packet->offsets[nb_protos]=packet_offset;//to have init of payload
	//filling payload
	char** payload = get_field_value_from_fieldlist(fields, "payload");
	if(payload!=NULL){
		int* pay_size;
		pay_size = (int*) get_field_value_from_fieldlist(fields, "payload_size");
		if(*pay_size>packet->payload_size){
			#ifdef DEBUG
			fprintf(stdout,"fill packet : payload data is larger than payload size. Truncating payload to %d bytes\n",packet->payload_size);
			#endif
			*pay_size=packet->payload_size;
		}
		char* payload_data = (*packet->data)+offset_packet;
		memcpy(payload_data,*payload,*pay_size);
		free(pay_size);
	}
	else{
		if(packet->payload_size>0){
			#ifdef DEBUG
			fprintf(stdout,"fill packet : no payload specified, letting 0s instead\n");
			#endif
		}
	}
	//calc cheksums
	unsigned char** psd_hed = malloc(sizeof(char*));
	*psd_hed=NULL;
	int* size_psd = malloc(sizeof(int));
	*size_psd=0;
	char* offset_data;
	char* offset_prev;
	for(i = packet->nb_protos-1; i>=0; i--){
		//create the fields containing info from packet
		#ifdef DEBUG
		fprintf(stdout,"checksum for protocol %s : \n",packet->protos[i]);
		#endif
		offset_data = (*packet->data)+packet->offsets[i];
		*psd_hed=NULL;
		*size_psd=0;
		if(tab[i]->need_ext_checksum==1){
			#ifdef DEBUG
			fprintf(stderr,"fill_packet :  %s protocol needs external checksum\n",tab[i]->name);
			#endif
			if(i==0){
				#ifdef DEBUG
				fprintf(stderr,"fill_packet :  error : %s protocol needs external checksum but is the first on the list\n",tab[i]->name);
				#endif
				return -1;
			}
			offset_prev = (*packet->data)+packet->offsets[i-1];
			int res_int = tab[i-1]->create_pseudo_header(&offset_prev,tab[i]->name,psd_hed,size_psd);
			if(res_int ==-1){
				#ifdef DEBUG
				fprintf(stderr,"fill_packet :  error : %s protocol needs external checksum but ext protocol %s cannot provide one\n",tab[i]->name,tab[i-1]->name);
				#endif
				return -1;
			}
			else{
				#ifdef DEBUG
				fprintf(stderr,"fill_packet :  computing checksum from this pseudo header\n");
				#endif
			}
		}
		if(checksums!=NULL){
			checksums[i] = tab[i]->compute_checksum(&offset_data,packet->size-packet->offsets[i],psd_hed,*size_psd);
		}
		else{
			tab[i]->compute_checksum(&offset_data,packet->size-packet->offsets[i],psd_hed,*size_psd);
		}
		if(tab[i]->after_checksum!=NULL){
			tab[i]->after_checksum(&offset_data);
		}
		free(*psd_hed);
		#ifdef DEBUG
		fprintf(stdout,"checksum for protocol %s filled\n",tab[i]->name);
		#endif
	}
	free(psd_hed);
	free(size_psd);
	return 0;
}

//useful to invers tag and checksum
int packet_set_payload(packet_s* packet, char* new_payload, int pay_size, int offset){
	int intern_paysize = pay_size;
	if(packet==NULL||new_payload==NULL){
		return -1;
	}
	if(pay_size> (packet->payload_size-offset)){
		intern_paysize=packet->payload_size-offset;
	}
	int new_offset = packet->size-packet->payload_size+offset;
	memcpy(&(packet->data[new_offset]), new_payload, intern_paysize);
	return 1;
}

int packet_exchange_checksum_and_payload( packet_s* packet, int proto_exch){
	if(packet==NULL){
		return -1;
	}
	if(proto_exch>=packet->nb_protos){
		return -1;
	}
	const proto_s* proto = choose_protocol(packet->protos[proto_exch]);
	char** data = packet->data+packet->offsets[proto_exch];
	unsigned short* future_check = (unsigned short*) &(packet->data[packet->size-packet->payload_size]);
	unsigned short* ex_check = proto->get_packet_field("checksum",data);
	packet_set_payload(packet, (char*) ex_check, 2, 0);//write the checksum at init of payload
	proto->set_packet_field(future_check,"checksum", data);
	free(ex_check);
	return 1;
}
