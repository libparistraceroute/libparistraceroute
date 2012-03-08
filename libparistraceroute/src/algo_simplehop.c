#include "algo.h"
#include<stdlib.h>
#include<stdio.h>
#include<pthread.h>

#define SIMPLEHOP_FIELDS 3

char* personal_fields_simplehop[SIMPLEHOP_FIELDS] = {"dest_addr","dest_port","TTL"};
static pthread_mutex_t numb_probes;

typedef struct perso_infos{
	char* dest_addr;
	short dest_port;
	short source_port;
	char TTL;
	int responded_probes;
	algo_opt_s* global_options;
} perso_infos_s;


void fill_info_perso_from_fields(field_s* fields, perso_infos_s* perso, algo_opt_s* options){
	perso->responded_probes=0;
	perso->global_options=options;
	perso->TTL = *((char*)get_field_value(fields, "TTL"));
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
	perso->global_options=options;	
	return;
}


void init_simplehop(algo_s* options, field_s* personal_options){
	if(options==NULL||personal_options==NULL){
		#ifdef DEBUG
		fprintf(stderr,"init_simplehop : passing null options. end\n");
		#endif
		return ;
	}
	if(check_presence_list(personal_fields_simplehop, SIMPLEHOP_FIELDS, personal_options)==-1){
		#ifdef DEBUG
		fprintf(stderr,"init_simplehop : missing parameters in fields for algorithm. end\n");
		#endif
		return ;
	}
	perso_infos_s* perso = malloc(sizeof(perso_infos_s));
	fill_info_perso_from_fields(personal_options, perso, options->options);
	//options->options->personal_options=perso;
	node_query(perso->dest_addr,  perso->dest_port, perso->source_port,perso->TTL,options, perso);
	return ;
}

void free_perso_infos(perso_infos_s* opt){
	free(opt->dest_addr);
	free(opt);
	return;
}


void on_reply_simplehop(probe_s* probe){
	//TODO:need a mutex here?
	//case if reply = timeout
	#ifdef DEBUG
	fprintf(stdout,"on_reply_simplehop : reply started\n");
	fflush(stdout);
	#endif
	perso_infos_s* infos = (perso_infos_s*) probe->perso_opt;
	if(infos==NULL||infos->global_options==NULL||probe==NULL){
		#ifdef DEBUG
		fprintf(stderr,"on_reply_simplehop : probe or options are missing. end\n");
		fflush(stdout);
		#endif
		return;
	}
	pthread_mutex_lock(&numb_probes);
	#ifdef DEBUG
	fprintf(stdout,"on_reply_simplehop : mutex on\n");
	#endif
	infos->responded_probes=infos->responded_probes+1;
	int jump = infos->responded_probes - infos->global_options->nb_probes_per_TTL; // positive or null -> assertion true, must end, else pass
	pthread_mutex_unlock(&numb_probes);
	if(jump>=0){
		output_s* out = infos->global_options->output_kind;
		out->show_all_probes(infos->global_options->probes);
		free_perso_infos(infos);
	}
	//free options if last call!
}



static algo_list_s simplehop_ops = {
	.next=NULL,
	.name="simplehop",
	.name_size=9,
	.personal_fields = personal_fields_simplehop,
	.nb_field = SIMPLEHOP_FIELDS,
	.init=init_simplehop,
	.on_reply=on_reply_simplehop,
};




ADD_ALGO(simplehop_ops);
