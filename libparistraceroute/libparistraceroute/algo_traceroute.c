#include "algo.h"
#include<stdlib.h>
#include<stdio.h>
#include<pthread.h>

#define TRACEROUTE_FIELDS 2

char* personal_fields_traceroute[TRACEROUTE_FIELDS] = {"dest_addr","dest_port"};
static pthread_mutex_t numb_probes;

typedef struct perso_infos{
	char* dest_addr;
	short dest_port;
	short source_port;
	char TTL;
	int responded_probes;
	algo_s* global_infos;
	algo_opt_s* global_options;
	field_s* probes;
} perso_infos_s;


void fill_info_perso_from_fields_traceroute(field_s* fields, perso_infos_s* perso, algo_opt_s* options, algo_s* algo_tot){
	perso->responded_probes=0;
	perso->global_options=options;
	perso->global_infos=algo_tot;
	perso->TTL = 1;
	perso->dest_port = *((short*)get_field_value(fields, "dest_port"));
	if((short*)get_field_value(fields, "source_port")!=NULL){
		perso->source_port = *((short*)get_field_value(fields, "source_port"));
	}
	else{
		perso->source_port = perso->dest_port;
	}
	char* dest_addr = (char*)get_field_value(fields, "dest_addr");
	char* perso_addr = malloc((strlen(dest_addr)+1)*sizeof(char));
	memcpy(perso_addr,dest_addr,strlen(dest_addr)+1);
	perso->dest_addr=perso_addr;
	perso->probes=NULL;	
	perso->global_options=options;	
	return;
}


void init_traceroute(algo_s* options, field_s* personal_options){
	if(options==NULL||personal_options==NULL){
		#ifdef DEBUG
		fprintf(stderr,"init_traceroute : passing null options. end\n");
		#endif
		return ;
	}
	if(check_presence_list(personal_fields_traceroute, TRACEROUTE_FIELDS, personal_options)==-1){
		#ifdef DEBUG
		fprintf(stderr,"init_traceroute : missing parameters in fields for algorithm traceroute. end\n");
		#endif
		return ;
	}
	perso_infos_s* perso = malloc(sizeof(perso_infos_s));
	fill_info_perso_from_fields_traceroute(personal_options, perso, options->options, options);
	//options->options->personal_options=perso;
	int nb_probes = perso->global_options->nb_probes_per_TTL;
	int i=0;
	probe_s* pro;
	for(i=0; i<nb_probes; i++){
		pro = uniq_node_query(perso->dest_addr, perso->dest_port, perso->source_port,perso->TTL,options, perso);
		//field_s* add_field(field_s* next, char* field_name, void* new_value)
		perso->probes = add_field(perso->probes, probe_get_name(pro), pro);
	}
	return ;
}

void free_perso_infos_traceroute(perso_infos_s* opt){
	free(opt->dest_addr);
	free_all_fields(opt->probes,0);//dont free probes
	free(opt);
	return;
}

//return 1 if all probes are "no_resp", -1 otherwise
int is_probes_resp(field_s* probes, int kind_of_resp){
	if (probes==NULL){
		return -1;
	}
	int res = 1;
	field_s* elem = probes;
	//void* get_field_value(field_s* fields, char* field_name);
	while(elem!=NULL){
		probe_s* probe = (probe_s*) get_field_value(elem, get_field_name(elem));
			if(probe_get_reply_kind(probe)!=kind_of_resp){
				res=-1;
			}
		elem =get_next_field(elem);
	}
	return res;
}
void on_reply_traceroute(probe_s* probe){
	//TODO:need a mutex here?
	//case if reply = timeout
	#ifdef DEBUG
	fprintf(stdout,"on_reply_traceroute : reply started\n");
	fflush(stdout);
	#endif
	perso_infos_s* infos = (perso_infos_s*) probe->perso_opt;
	if(infos==NULL||infos->global_options==NULL||probe==NULL){
		#ifdef DEBUG
		fprintf(stderr,"on_reply_traceroute : probe or options are missing. end\n");
		fflush(stdout);
		#endif
		return;
	}
	pthread_mutex_lock(&numb_probes);
	#ifdef DEBUG
	fprintf(stdout,"on_reply_traceroute : mutex on\n");
	#endif
	//int show_probe_simple(probe_s* probe)
	infos->responded_probes=infos->responded_probes+1;
	int jump = infos->responded_probes - infos->global_options->nb_probes_per_TTL; // positive or null -> assertion true, must end, else pass
	pthread_mutex_unlock(&numb_probes);
	if(jump>=0){
		output_s* out = infos->global_options->output_kind;
		//out->show_all_probes(infos->global_options->probes);
		out->show_all_probes(infos->probes);
		if(is_probes_resp(infos->probes, NO_RESP)==1 || is_probes_resp(infos->probes, END_RESP)==1){//last IP or not ip
			free_perso_infos_traceroute(infos);
		}
		else{
			free_all_fields(infos->probes,0);//not freeing probes
			infos->probes=NULL;
			int nb_probes = infos->global_options->nb_probes_per_TTL;
			infos->responded_probes=0;
			infos->TTL = infos->TTL+1;
			int i=0;
			probe_s* pro;
			for(i=0; i<nb_probes; i++){
				pro = uniq_node_query(infos->dest_addr, infos->dest_port, infos->source_port, infos->TTL, infos->global_infos, infos);
				//field_s* add_field(field_s* next, char* field_name, void* new_value)
				infos->probes = add_field(infos->probes, probe_get_name(pro), pro);
			}
		}
	}
}



static algo_list_s traceroute_ops = {
	.next=NULL,
	.name="traceroute",
	.name_size=10,
	.personal_fields = personal_fields_traceroute,
	.nb_field = TRACEROUTE_FIELDS,
	.init=init_traceroute,
	.on_reply=on_reply_traceroute,
};




ADD_ALGO(traceroute_ops);
