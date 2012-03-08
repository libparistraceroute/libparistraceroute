#include<stdlib.h>
#include<stdio.h>
#include <unistd.h>

#include "algo.h"


/* list of the algorithm known */
algo_list_s *algo_base = NULL;

/* list of the algorithm (with the options) the user want to realise*/
field_s* algo_enum = NULL;

algo_list_s* choose_algorithm(char* algo_name){
	if(algo_name==NULL){
		return NULL;
	}
	algo_list_s* current = algo_base;
	char search = 1;
	while(search==1){
		if(current!=NULL && current->name_size!=strlen(algo_name)){
			current=current->next;
		}
		else{
			if( current!=NULL && strncasecmp(current->name,algo_name,current->name_size)!=0){
				current=current->next;
			}
			else{
				search=0;
			}
		}
	}
	if(current==NULL){
		#ifdef DEBUG
		fprintf(stderr,"choose_algorithm : algorithm %s not found in list\n",algo_name);
		#endif
		return NULL;
	}
	return current;

}

field_s* get_algo_enum(){
	return algo_enum;
}
char* create_name_algo(){
	char* res=malloc(17*(sizeof(char)));
	snprintf(res,17,"algo %d",get_number_of_fields(algo_enum));
	return res;
}

int add_algo(char* algo_name, char* output_kind, int probes_per_TTL, char** protocols, int nb_proto, int sending_socket){
	//init the network, execute the algorithm, make the output
	algo_list_s* algo_func = choose_algorithm(algo_name);
	if(algo_func==NULL){
		#ifdef DEBUG
		fprintf(stderr,"init_algo : algorithm not found\n");
		#endif
		return 0;
	}
	output_s* output=choose_output(output_kind);
	if(output==NULL){
		#ifdef DEBUG
		fprintf(stderr,"init_algo : output not found\n");
		#endif
		return 0;
	}
	algo_s* algo = malloc(sizeof(algo_s));
	algo->options=malloc(sizeof(algo_opt_s));
	algo->algorithm=algo_func;
	algo->options->output_kind=output;
	//algo->options->timeout=timeout;
	algo->options->nb_probes_per_TTL=probes_per_TTL;
	algo->options->protocols=protocols;
	algo->options->nb_proto=nb_proto;
	algo->options->sending_socket = sending_socket;
	algo->options->probes=NULL;
	algo_enum = add_field(algo_enum, create_name_algo(), algo);
	return 1;
}

void free_algo(algo_s* algo){
	free(algo->options);
	free(algo);
}

void add_algo_list (algo_list_s *ops) {
	#ifdef DEBUG
	fprintf(stdout,"add_algo_list : adding algo %s to list\n",ops->name);
	#endif
	ops->next = algo_base;
	algo_base = ops;
}

probe_s* uniq_node_query(char* dest_addr,  short dest_port, short source_port, char TTL, algo_s* opt, void* perso_opt){
	if(opt==NULL||opt->options==NULL||opt->algorithm==NULL){
		#ifdef DEBUG
		fprintf(stderr,"uniq_node_query : missing options to realise the query. end.\n");
		#endif
		return NULL;
	}
	algo_opt_s* options = opt->options;
	//send a probe
	probe_s* probe = create_probe(create_name_probe(),options->protocols,options->nb_proto, opt, perso_opt);
	probe_set_data_string(probe,"dest_addr", dest_addr);
	probe_set_data_short(probe,"dest_port", dest_port);
	probe_set_data_short(probe,"src_port", source_port);
	probe_set_data_char(probe,"TTL", TTL);
	//view_fields(probe->data);
	send_probe(probe, 0, options->sending_socket);
	options->probes = add_field(options->probes, probe->name , probe);
	return probe;
}

void node_query(char* dest_addr,  short dest_port, short source_port, char TTL, algo_s* opt, void* perso_opt){
	if(opt==NULL||opt->options==NULL||opt->algorithm==NULL){
		#ifdef DEBUG
		fprintf(stderr,"node_query : missing options to realise the query. end.\n");
		#endif
		return;
	}
	algo_opt_s* options = opt->options;
	int i=0;
	for(i=0;i< options->nb_probes_per_TTL;i++){
		probe_s* probe = create_probe(create_name_probe(),options->protocols,options->nb_proto, opt, perso_opt);
		probe_set_data_string(probe,"dest_addr", dest_addr);
		probe_set_data_short(probe,"dest_port", dest_port);
		probe_set_data_short(probe,"src_port", source_port);
		probe_set_data_char(probe,"TTL", TTL);
		//view_fields(probe->data);
		send_probe(probe, 0, options->sending_socket);
		options->probes = add_field(options->probes, probe->name , probe);
	}
	return;
}

void link_query(char* dest_addr, short dest_port, short source_port, char begin_TTL, algo_s* opt, void* perso_opt){
	node_query( dest_addr, dest_port, source_port, begin_TTL, opt, perso_opt);
	node_query( dest_addr, dest_port, source_port, begin_TTL+1, opt, perso_opt);
}

void all_next_hops_query(char* dest_addr, char TTL, algo_s* opt){

}

void path_query(char* dest_addr, algo_opt_s* options){

}

void test_load_balancing(char* dest_addr, char entry_TTL, algo_s* options){

}
