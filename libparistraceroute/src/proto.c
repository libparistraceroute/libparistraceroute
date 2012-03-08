#include<stdlib.h>
#include<stdio.h>
#include<string.h>
#include<netinet/ip.h> //for recognition
#include "proto.h"

static proto_s *proto_base = NULL;


void view_tab(char** tab, int size){
	//fprintf(stdout, "elements of the tab : %i elements which are\n",size);
	int i =0;
	for(i=0; i<size; i++){
		fprintf(stdout,"%s, ",tab[i]);
	}
	//fprintf(stdout,"\nend of tab\n");
	return;
}

const proto_s* choose_protocol(char* proto_name){
	proto_s *current = proto_base;
	//int proto_size=strlen(proto);
	//int size;
	//while(proto_ops!=NULL && (strncasecmp(proto_ops->protocol,proto,proto_ops->proto_name_size)!=0)){
	char search = 1;
	while(search==1){
		if(current!=NULL && current->name_size!=strlen(proto_name)){
			current=current->next;
		}
		else{
			if( current!=NULL && strncasecmp(current->name,proto_name,current->name_size)!=0){
				current=current->next;
			}
			else{
				search=0;
			}
		}
	}
	if(current==NULL){
		#ifdef DEBUG
		fprintf(stderr,"choosing protocol : the specified protocol (%s) has no functions associated\n",proto_name);
		#endif 
		return NULL;
	}
	return current;
}

void view_available_protocols(){
	fprintf(stdout,"availables protocols :\n");
	proto_s* curr=proto_base;
	while(curr!=NULL){
		fprintf(stdout,"%s, ",curr->name);
		curr = curr ->next;
	}
	fprintf(stdout,"\nend.\n");
	return;
}

void add_proto_list (proto_s *ops) {
	#ifdef DEBUG
	fprintf(stdout,"add_proto_list : adding proto %s to list\n",ops->name);
	#endif
	ops->next = proto_base;
	proto_base = ops;
}


int guess_datagram_protocol(char* datagram, int data_size, char* protocol){
	//if(protocol==NULL||strcmp(protocol,'\0')==0||datagram==NULL||strcmp(datagram,'\0')==0){
	if(protocol==NULL||datagram==NULL){
		#ifdef DEBUG
		fprintf(stderr,"guess_datagram_protocol : passing a NULL or empty parameter\n");
		#endif
		return 0;
	}
	const proto_s* prot = choose_protocol(protocol);
	if(prot==NULL){
		#ifdef DEBUG
		fprintf(stderr,"guess_datagram_protocol : unknown protocol : %s \n", protocol);
		#endif
		return 0;
	}
	return prot->check_datagram(&datagram, data_size);
	
}

int proto_get_header_size(char* datagram, char* protocol){
	if(protocol==NULL||datagram==NULL){
		#ifdef DEBUG
		fprintf(stderr,"guess_header_size : passing a NULL or empty parameter\n");
		#endif
		return 0;
	}
	const proto_s* prot = choose_protocol(protocol);
	if(prot==NULL){
		#ifdef DEBUG
		fprintf(stderr,"guess_header_size : unknown protocol : %s \n", protocol);
		#endif
		return 0;
	}
	return prot->get_header_size(&datagram,NULL);
}

void* proto_get_field(char* datagram, char* protocol,char* name){
	if(protocol==NULL||datagram==NULL||name==NULL){
		#ifdef DEBUG
		fprintf(stderr,"get_field : passing a NULL or empty parameter\n");
		#endif
		return 0;
	}
	const proto_s* prot = choose_protocol(protocol);
	if(prot==NULL){
		#ifdef DEBUG
		fprintf(stderr,"get_field : unknown protocol : %s \n", protocol);
		#endif
		return 0;
	}
	return prot->get_packet_field(name, &datagram);
}

void proto_view_fields(char* protocol){
	if(protocol==NULL){
		#ifdef DEBUG
		fprintf(stderr,"proto_view_field : parameters empty \n");
		#endif
		return;
	}
	const proto_s* prot = choose_protocol(protocol);
	if(prot==NULL){
		#ifdef DEBUG
		fprintf(stderr,"proto_view_field : unknown protocol : %s \n", protocol);
		#endif
		return;
	}
	fprintf(stdout,"proto_view_field : protocol  %s has\nfields : ", protocol);
	view_tab(prot->fields, prot->number_fields);
	fprintf(stdout,"\nmandatory ones : ");
	view_tab(prot->mandatory_fields, prot->number_mandatory_fields);
	fprintf(stdout,"\n\n");
	return;
	
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
