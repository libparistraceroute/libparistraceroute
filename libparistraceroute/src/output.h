#ifndef H_PATR_OUTPUT
#define H_PATR_OUTPUT
#include<stdio.h>
#include<stdlib.h>

#include"probe.h"

typedef enum {
    OUTPUT_INIT,
    OUTPUT_TTL_DONE,
    OUTPUT_TRACEROUTE_DONE,
    OUTPUT_RESERVED
} output_signal_t;

typedef struct output_struct{
	/** next element of list*/ struct output_struct* next;
	/**kind of output*/ char* name;
	/** size of name*/ int name_size;
	/** descriptor of file in which write (non ->stdout) */ FILE* output_file;
	/** initialize the output (eg printing infos)*/ int (*init_output)(field_s* infos);
	/** show a probe (useful for showing content when receiving) */ int (*show_probe)(probe_s* probe);
	/** to print all the probes in list (for global presentation)*/ int (*show_all_probes)(field_s* probes);
    	/** generic "signal" handler */ int (*signal)(output_signal_t signal, void *params, field_s *probes);
} output_s;


/**
 * choose the output depending on name
 * @param output_name : the output name
 * @return the corresponding output
 */
output_s* choose_output(char* output_name);


/**
 * add a output on the output list
 * @param ops : the output 
 * @return void
 */
void add_output_list (output_s *ops);

/**
 * set the printing mode of the output
 * @param output : the output to change
 * @param printing_mode : the selected mode
 * @return 1 if success, -1 otherwise
 */

int set_printing_mode(output_s* output, char printing_mode);

#define ADD_OUTPUT(MOD)	\
static void __init_ ## MOD (void) __attribute__ ((constructor));	\
static void __init_ ## MOD (void) {	\
				\
	add_output_list (&MOD);	\
}

#endif

