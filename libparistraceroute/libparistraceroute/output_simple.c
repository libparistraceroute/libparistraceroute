#include "output.h"
#include "field.h"
#include<stdlib.h>
#include<stdio.h>

/*#define DEFAULT_PROBES_PER_TTL 3
typedef struct traceroutedata_struct{
	int probes_per_TTL;
	char* dest_addr;
}tracedata_s;

tracedata_s infos;

void fill_infos_from_fields(field_s* infos){
	(int*)probes = get_field_value(infos, "probes_per_TTL");
	if(probes!=NULL){
		infos->probes_per_TTL=*probes;
	}
	else{
		infos->probes_per_TTL=DEFAULT_PROBES_PER_TTL;
	}
	(char**)addr = get_field_value(infos, "dest_addr");
	if(addr!=NULL){
		infos->dest_addr=*addr;
	}
	else{
		infos->dest_addr="";
	}
}*/

int init_output_simple(field_s* infos){
	return 1;
}

int show_probe_simple(probe_s* probe){
	if(probe!=NULL){
		double receive = probe_get_receiving_time(probe);
		double send = probe_get_sending_time(probe);
		double result=(receive-send)/2.0; //time = go and back so /2
		fprintf(stdout,"targeting %s:%d ; ",(char*)probe_get_data_value(probe,"dest_addr"),*((short*)probe_get_data_value(probe,"dest_port")));
		fprintf(stdout,"TTL %d, reply IP : %s in %f sec\n", probe_get_TTL(probe),probe_get_reply_addr(probe),result);
		return 1;

	}
	return -1;
}

int show_all_probes_simple(field_s* probes){
	field_s* curr = probes;
	int res=1;
	while(curr!=NULL){
		if(show_probe_simple((probe_s*)curr->value)!=1){
			res=-1;
		}
		curr= get_next_field(curr);
	}
	return res;
}

/* We should not have to give probe as parameters */
int simple_signal(output_signal_t signal, void *params, field_s *probes) 
{
	field_s* p;
    switch (signal) {
        case OUTPUT_INIT:
            break;
        case OUTPUT_TTL_DONE:
            /* We print all probes with a given ttl */
            printf("%d\t", *((int*)params));
            for (p=probes; p; p = get_next_field(p)) {
                probe_s *probe = (probe_s*)p->value;
    	    	double rtt = (probe_get_receiving_time(probe) - probe_get_sending_time(probe)) * 1000;
                if (probe_get_TTL(probe) == *((int*)params))
                    printf("%s (%f ms) ", probe_get_reply_addr(probe), rtt);
            }
            printf("\n");
            break;
        default:
            break;
    }
    return 1;
}

static output_s simple_ops = {
	.next=NULL,
	.name="simple",
	.name_size=6,
	.output_file=NULL,
	.init_output=init_output_simple,
	.show_probe=show_probe_simple,
	.show_all_probes=show_all_probes_simple,
    .signal=simple_signal
};


ADD_OUTPUT(simple_ops);

