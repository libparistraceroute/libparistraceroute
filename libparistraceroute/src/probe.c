#include "probe.h"
#include "field.h"
#include "fieldlist.h"

#include<stdlib.h>
#include<stdio.h>
#include<string.h>

fieldlist_s* probes_list = NULL;
fieldlist_s* complete_probes_list = NULL;


int add_probe_to_probes_list(probe_s* probe){
	if(probe == NULL){
		return -1;
	}
	if(probes_list==NULL){
		//initiate the lists
		field_s* list_of_probes = create_field(probe->name, probe);
		probes_list=create_fieldlist("probes", list_of_probes);
	}
	else{
		field_s* fields = get_fieldlist_value(probes_list, "probes");
		fields = add_field_end(fields, probe->name, probe);
		update_fieldlist_value(probes_list, "probes", fields);
	}
	return 1;
}


int remove_probe_from_probes(probe_s* probe){
	if(probe == NULL){
		return -1;
	}
	if(probes_list==NULL){
		return -1;
	}
	else{
		remove_field_from_fieldlist(&probes_list, probe->name,"probes", 0);//do not suppress the probe
	}
	return 1;
}
probe_s* get_older_probe(){
	field_s* fields = get_fieldlist_value(probes_list, "probes");
	if(fields!=NULL){
		return (probe_s*) fields->value;
	}
	else{
		return NULL;
	}
}

int add_probe_to_complete_probes(probe_s* probe){
	if(probe == NULL){
		return -1;
	}
	if(complete_probes_list==NULL){
		//initiate the lists
		field_s* list_of_probes = create_field(probe->name, probe);
		complete_probes_list=create_fieldlist("complete probes", list_of_probes);
	}
	else{
		field_s* fields = get_fieldlist_value(complete_probes_list, "complete probes");
		fields = add_field(fields, probe->name, probe);
		update_fieldlist_value(complete_probes_list, "complete probes", fields);
	}
	return 1;
}

int remove_probe_from_complete_probes(probe_s* probe){
	if(probe == NULL){
		return -1;
	}
	if(complete_probes_list==NULL){
		return -1;
	}
	else{
		remove_field_from_fieldlist(&complete_probes_list, probe->name,"complete probes", 0);//probe suppressed after
		//free_probe(probe);
	}
	return 1;
}

int is_probe_complete(probe_s* probe){
	return check_presence_field(probe->name,complete_probes_list);
}



probe_s* match_reply_with_probe(char* reply, int reply_size, unsigned short* checksums, int nb_check){
	if(reply==NULL||reply_size==0||checksums==NULL||nb_check==0){
		#ifdef DEBUG
		fprintf(stderr,"match_reply_with_probe : passing empty parameter(s)\n");
		#endif
		return NULL;
	}
	probe_s* probe = NULL;
	unsigned short* probe_check;
	int probe_nb_check;
	int i=0;
	field_s* fields = get_fieldlist_value(probes_list, "probes");
	if(fields ==NULL){
		#ifdef DEBUG
		fprintf(stderr,"match_reply_with_probe : list of probes empty\n");
		#endif
	}
	while(fields!=NULL){
		probe = (probe_s*) get_field_value(fields, get_field_name(fields));
		probe_check = probe->checksums;
		probe_nb_check=probe->nb_proto;
		if(probe_nb_check!=nb_check){
			fields=get_next_field(fields);
		}
		else{
			for(i=0;i<nb_check;i++){
				if(probe_check[i]!=checksums[i]&&checksums[i]!=0){//checksum = 0 implies ignore it
					#ifdef DEBUG
					fprintf(stdout,"match_reply_with_probe : probe checksum %d is %04x and we expected %04x.\n",i,probe_check[i],checksums[i]);
					#endif
					fields=get_next_field(fields);
					probe=NULL;
					break; //end the "for" -> continue the while
				}
			}
			if(probe==NULL){
				continue; //continue the while for next 
			}
			//end of for : all checksums corresponding -> ok
			#ifdef DEBUG
			fprintf(stdout,"match_reply_with_probe : matching probe found\n");
			#endif
			if(probe->reply_size!=0){
				//this is not supposed to happen!
				#ifdef DEBUG
				fprintf(stderr,"match_reply_with_probe : probe matching but already has a reply! something somewhere went terribly wrong.\n");
				#endif
				fields=get_next_field(fields);
				continue;//next iteration of while
			}
			if(remove_probe_from_probes(probe)==-1){
				#ifdef DEBUG
				fprintf(stderr,"match_reply_with_probe : remove of list failed\n");
				#endif
			}
			if(add_probe_to_complete_probes(probe)!=1){
				#ifdef DEBUG
				fprintf(stderr,"match_reply_with_probe : add in complete list failed\n");
				#endif
			}
			#ifdef DEBUG
			fprintf(stdout,"match_reply_with_probe : corresponding probe found. adding info to probe and adding it to response list\n");
			#endif
			probe->reply=malloc((reply_size+1)*sizeof(char));
			strncpy(probe->reply,reply,reply_size+1);
			probe->reply_size=reply_size;
			break; //break the while
		}
	}
	if(fields==NULL){
		return NULL;
	}
	return probe;
}

//TODO: remplacer par fields?
probe_s* create_probe(char* name, char** protos, int nb_proto, void* algo, void* perso_opt){
	probe_s* res= malloc(sizeof(probe_s));
	//default 
	char* stock_name=malloc((strlen(name)+1)*sizeof(char));
	memcpy(stock_name,name,strlen(name)+1);
	res->name=stock_name;
	res->data=NULL;
	res->protos=protos;
	res->nb_proto=nb_proto;
	res->checksums=malloc(nb_proto*sizeof(short));
	res->reply=NULL;
	res->reply_size=0;
	res->receiving_time=0;
	res->reply_addr=NULL;
	res->sending_time=0;
	res->reply_kind=WAITING_RESP;
	res->perso_opt=perso_opt;
	res->algorithm=algo;
	add_probe_to_probes_list(res);
	return res;

}



char* create_name_probe(){
	char* res=malloc(17*(sizeof(char)));
	snprintf(res,17,"probe%d",get_number_of_probes());
	return res;
}

void free_probe(probe_s* probe){
	if(probe!=NULL){
		free(probe->name);
		free(probe->checksums);
		free_all_fields(probe->data);
		free(probe->reply_addr);
		free(probe->reply);
        free(probe->perso_opt);
		free(probe);
		probe=NULL;
	}
}


void free_probe_void(void* probe_void){
	probe_s* probe = probe_void;
	free_probe(probe);
}

void* probe_get_data_value(probe_s* probe, char* name){
	if(check_presence(name,probe->data)==-1){
		#ifdef DEBUG
		fprintf(stderr," get_data_value : %s field not found in data\n", name);
		#endif
		return NULL;
	}
	return get_field_value(probe->data, name);
}

void probe_set_data_int(probe_s* probe, char* name, int value){
	int* res = malloc(sizeof(int));
	*res=value;
	probe->data=add_field(probe->data, name, res);
	
}

void probe_set_data_short(probe_s* probe, char* name, short value){
	short* res = malloc(sizeof(short));
	*res=value;
	probe->data=add_field(probe->data, name, res);
}

void probe_set_data_char(probe_s* probe, char* name, char value){
	char* res = malloc(sizeof(char));
	*res=value;
	probe->data=add_field(probe->data, name, res);
}

void probe_set_data_string(probe_s* probe, char* name, char* value){
	char* res = malloc((strlen(value)+1)*sizeof(int));
	strncpy(res, value, strlen(value)+1);
	probe->data=add_field(probe->data, name, res);
}

void probe_set_data_float(probe_s* probe, char* name, float value){
	float* res = malloc(sizeof(float));
	*res=value;
	probe->data=add_field(probe->data, name, res);
}


int probe_get_reply_kind(probe_s* probe){
	if(probe!=NULL){
		return probe->reply_kind;
	}
	return -1;
}

char* probe_get_name(probe_s* probe){
	if(probe!=NULL){
		return probe->name;
	}
	return NULL;
}

unsigned char probe_get_TTL(probe_s* probe){
	if(probe!=NULL){
		unsigned char* res = (unsigned char*) probe_get_data_value(probe, "TTL");
		if(res!=NULL){
			return *res;
		}
	}
	return 0;
}


void probe_set_sending_time(probe_s* probe, double time){
	if(probe!=NULL){
		//probe->sending_time =  get_time (void);
		probe->sending_time =  time;
	}
	return ;
}

double probe_get_sending_time(probe_s* probe){
	if(probe!=NULL){
		return probe->sending_time;
	}
	return -1;
}


void probe_set_receiving_time(probe_s* probe, double time){
	if(probe!=NULL){
		//probe->receiving_time =  get_time (void);
		probe->receiving_time =  time;
	}
	return ;
}

double probe_get_receiving_time(probe_s* probe){
	if(probe!=NULL){
		return probe->receiving_time;
	}
	return -1;
}

void probe_set_reply_addr(probe_s* probe, char* reply_addr){
	if(probe!=NULL&&reply_addr!=NULL){
		//probe->receiving_time =  get_time (void);
		int lenght = strlen(reply_addr);
		probe->reply_addr=malloc(lenght+1*sizeof(char));
		memcpy(probe->reply_addr,reply_addr,lenght+1);
	}
	return ;
}

void probe_set_reply_kind(probe_s* probe,  int reply_kind){
	if(probe==NULL){
		return;
	}
	if(reply_kind==WAITING_RESP
	|| reply_kind==TTL_RESP 
	|| reply_kind==NO_RESP
	|| reply_kind==END_RESP
	|| reply_kind==OTHER_RESP){
		probe->reply_kind=reply_kind;
	}
	else{
		probe->reply_kind=OTHER_RESP;
	}
	return;
}

char* probe_get_reply_addr(probe_s* probe){
	if(probe!=NULL){
		return probe->reply_addr;
	}
	return NULL;
}

void view_probe(probe_s* probe){
	int i=0;
	if(probe==NULL){
		fprintf(stderr,"show_probe : passing a NULL parameter\n");
		return;
	}
	fprintf(stdout,"probe %s\n",probe->name);
	for(i=0; i<probe->nb_proto;i++){
		fprintf(stdout,"Protocol n°%d : %s\n",i+1,probe->protos[i]);
	}
	if(probe->reply_addr!=NULL){
		fprintf(stdout,"probe sent at %f\n",probe->sending_time);
		fprintf(stdout,"reply received at %f\n",probe->receiving_time);//?
		fprintf(stdout,"from address : %s\n",probe->reply_addr);
		fprintf(stdout,"reply size is : %d\n",probe->reply_size);
	}
	return;
}

void view_probe_void(void* probe){
	probe_s* prob=(probe_s*) probe;
	view_probe(prob);
}

void view_probes_lists(){
	field_s* list = get_fieldlist_value(probes_list, "probes");
	field_s* complete_list = get_fieldlist_value(complete_probes_list, "complete probes");
	view_fields_with_function(list, &view_probe_void );
	view_fields_with_function(complete_list, &view_probe_void );
	
}

unsigned short calculate_tag_from_probe(probe){
	//function to calculate a tag, it will ensure to associate a kind of flow with a path
	return 0;
}

field_s* probe_get_data(probe_s* probe){
	if(probe==NULL){
		#ifdef DEBUG
		fprintf(stderr,"create_fields_from_probe : passing a NULL probe. end\n");
		#endif
		return NULL;
	}
	return probe->data;

}

void free_all_probes(){
	free_all_fieldlists_with_function(probes_list, &free_probe_void);
	free_all_fieldlists_with_function(complete_probes_list, &free_probe_void);
}

int get_number_of_probes(){
	field_s* fields = get_fieldlist_value(probes_list, "probes");
	field_s* comp_fields = get_fieldlist_value(complete_probes_list, "complete probes");
	int res = get_number_of_fields(fields)+get_number_of_fields(comp_fields);
	return res;
}

