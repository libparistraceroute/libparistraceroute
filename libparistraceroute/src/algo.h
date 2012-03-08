#ifndef H_PATR_ALGO
#define H_PATR_ALGO

#include"output.h"
#include"network.h"
#include"field.h"


typedef struct algo_opt{
	/** output kind*/ output_s* output_kind;
	///** chosen timeout */int timeout;
	/**number of probe per TTL*/int nb_probes_per_TTL;
	/** used protocols*/ char** protocols;
	/** number of protocols*/ int nb_proto;
	/** socket to send */ int sending_socket;
	/** probes that have been sent by this algo instance*/ field_s* probes;
}algo_opt_s;

typedef struct algo_struct algo_s;

typedef struct algo_list{
	/** next struct*/ struct algo_list* next;
	/** name of algo*/ char* name;
	/** size of name*/ int name_size;
	/** fields to fill personnal info */ char** personal_fields;
	/** size of personal_fields array */ int nb_field; 
	/** fonction to initialise the algorithm, return personal options*/void (*init)(algo_s* options, field_s* personal_options);
	/** fonction to call when a probe receive a reply*/void (*on_reply)(probe_s* probe);
} algo_list_s;

struct algo_struct{
	/** the current algorithm*/ algo_list_s* algorithm;
	/** general options for this algorithm*/ algo_opt_s* options; 
};



/**
 * add a algorithm to execute at the list of algorithm
 * @param algo_name the algorithm chosen
 * @param output_kind the kind of output selected
 * @param probes_per_TTL the number of probes to send for each ttl query
 * @param timeout time before the probe is considered as unresponded
 * @param protocols : the protocols to put in the probe
 * @param nb_proto the number of protocols (for the above array)
 * @param sending_socket : the socket in which send the probe
 * @return int to indicate success (1) or fail (0)
 */
int add_algo(char* algo_name, char* output_kind, int probes_per_TTL, char** protocols, int nb_proto,int sending_socket);

/**
 * free the outputs
 * @return void
 */
void free_algos();

/**
 * create the list of algorithms with generic functions
 * @param *ops the struct of actual algo to add
 * @return void
 */
void add_algo_list (algo_list_s *ops);

/**
 * get the pointer to the init of algorithms enumeration
 * @return the enumeration
 */
field_s* get_algo_enum();

/**
 *@brief the following functions are used by the "execution" function of the algorithm
 * function to send a probe to a certain adress with a certain TTL
 * @param dest_addr the destination address
 * @param dest_port the destination port
 * @param source_port the destination port
 * @param TTL the chosen TTL
 * @param options the options
 * @param perso_opt the options of this particular algorithm
 * @return void
 */
void node_query(char* dest_addr,  short dest_port, short source_port, char TTL, algo_s* options, void* perso_opt);

/**
 * function to discover a link for a certain destination and TTL
 * @param dest_addr the destination address
 * @param dest_port the destination port
 * @param source_port the source port
 * @param entry_TTL the chosen TTL (i e looking for link between TTL and TTL+1)
 * @return a probe?
 */
void link_query(char* dest_addr, short dest_port, short source_port, char entry_TTL, algo_s* options, void* perso_opt);

/**
 * function to discover all next hop from a certain node (TTL) with a certain confidence interval
 * @param dest_addr the destination address
 * @param entry_TTL the chosen TTL (i e looking for links between TTL and TTL+1)
 * @return a probe?
 */
void all_next_hops_query(char* dest_addr, char entry_TTL, algo_s* options);

/**
 * function to discover all the path from the source to dest_addr
 * @param dest_addr the destination address
 * @return a probe?
 */
void path_query(char* dest_addr, algo_opt_s* options);
/**
 * function to try to discover if there is a load balancer
 * @param dest_addr the destination address
 * @param entry_TTL the chosen TTL (i e looking for load balancer between TTL and TTL+1)
 * @return a probe?
 */
void test_load_balancing(char* dest_addr, char entry_TTL, algo_s* options);


#define ADD_ALGO(MOD)	\
static void __init_ ## MOD (void) __attribute__ ((constructor));	\
static void __init_ ## MOD (void) {	\
				\
	add_algo_list (&MOD);	\
}

#endif

