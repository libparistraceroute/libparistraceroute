#ifndef H_PATR_PROBE
#define H_PATR_PROBE

#include"fieldlist.h"

#define WAITING_RESP 0
#define TTL_RESP 1
#define NO_RESP 2
#define END_RESP 3
#define OTHER_RESP 4

typedef struct probe_struct{
/** name of the probe*/ char* name;
/** int to identify the probe */ unsigned int id;
/** int to identify the probe */ unsigned int flow_id;
/** fields containing the data */ field_s* data;
/** protocols names */ char** protos;
/** checksum for each protocol header stocked in NETWORK ORDER*/ unsigned short* checksums;
/** size of protocols and checksums arrays */ int nb_proto;
/** time of sending the packet*/ double sending_time;
/** time of receiving the packet*/ double receiving_time;
/** reply address*/ char* reply_addr;
/** the reply */ char* reply;
/** reply size */ int reply_size;
/** kind of reply TTL_RESP = answer to ttl, NO_RESP = no response, END_RESP = response but destination is achieved, OTHER_RESP other*/int reply_kind;
/** the algorithm related to this probe and the algorithm otpions, type algo_s**/ void* algorithm;
/** personal options for the algorithm, unical to the probe*/ void* perso_opt;
} probe_s;

/**
 * create a probe from transmitted data
 * @param TTL the TTL of packet
 * @param flow the id flow for the packet 
 * @param dest_port the destination port ( 0 if none)
 * @param source_port the source port (0 if none)
 * @param dest_addr (IP?) dest address
 * @param source_addr (IP?) source address
 * @param protos the array of protocols for this probe
 * @param nb_proto the size of the array
 * @param algo : the informations about the algortihm that created this probe
 * @return probe_s the new probe
 */

probe_s* create_probe(char* name, char** protos, int nb_proto, void* algo, void* perso_opt);


/**
 * get the older probe from unresponsed fieldlist
 * @param void
 * @return the older probe
 */
probe_s* get_older_probe();

/**
 * set sending time in probe
 * @param the time
 * @param the probe
 * @return void
 */

void probe_set_sending_time(probe_s* probe, double time);

/**
 * get TTL in probe
 * @param the probe
 * @return value of TTL, 0 otherwise
 */
unsigned char probe_get_TTL(probe_s* probe);

/**
 * create a name (form probe number) for the probe. it will assure unique name if used for each probe's names
 * @param void
 * @return a char * containing a name
 */
char* create_name_probe();

/**
 * get name of probe
 * @param the probe
 * @return pointer to name, NULL otherwise
 */
char* probe_get_name(probe_s* probe);

/**
 * get kind of reply pf probe
 * @param the probe
 * @return int the value of reply kind, -1 if failed
 */
int probe_get_reply_kind(probe_s* probe);

/**
 * get sending in probe
 * @param the probe
 * @return value of sending time, -1 otherwise
 */
double probe_get_sending_time(probe_s* probe);
/**
 * set receiving time in probe
 * @param the time
 * @param the probe
 * @return void
 */
void probe_set_receiving_time( probe_s* probe, double time);

/**
 * get receiving in probe
 * @param the probe
 * @return value of sending time, -1 otherwise
 */
double probe_get_receiving_time(probe_s* probe);

/**
 * set kind of reply pf probe
 * @param the probe
 * @param reply_kind the kind of reply
 * @return void
 */
void probe_set_reply_kind(probe_s* probe, int reply_kind);

/**
 * set reply address in probe
 * @param the reply address
 * @param the probe
 * @return void
 */
void probe_set_reply_addr(probe_s* probe, char* reply_addr);

/**
 * get reply address in probe
 * @param the probe
 * @return pointer to the reply_addr_value in the probe
 */
char* probe_get_reply_addr(probe_s* probe);

/**
 * show the content of a probe
 * @param probe the probe to show
 * @return void
 */

void view_probe(probe_s* probe);

/**
 * free the space allocated to probe
 * @param probe the probe to free
 * @return void
 */

void free_probe(probe_s* probe);

/**
 * create the fields of the field structure with the infos from probe
 * @param probe the probe
 * @return field_s* the fields
 */

field_s* probe_get_data(probe_s* probe);

/**
 * get the correct probe from the checksums and assignate to it the reply
 * @param probe the reply packet
 * @param the size of reply
 * @param the checksums
 * @param the number of checksums
 * @return the found probe ?  or a int?
 */

probe_s* match_reply_with_probe(char* reply, int reply_size, unsigned short* checksums, int nb_check);


/**
 * get a certain field value from the probe data
 * @param probe the probe to read
 * @param the value to get
 * @return pointer to data value
 */
void* probe_get_data_value(probe_s* probe, char* name);

/**
 * add int field value name name in the probe data,(also malloc the value)
 * @param probe the probe to write
 * @param the name of data
 * @param the value to set
 * @return void
 */
void probe_set_data_int(probe_s* probe, char* name, int value);

/**
 * add a certain short field value named name in the probe data( also malloc the value)
 * @param probe the probe to write
 * @param the name of data
 * @param the value to set
 * @return void
 */
void probe_set_data_short(probe_s* probe, char* name, short value);

/**
 * add a certain char field value named name in the probe data( also malloc the value)
 * @param probe the probe to write
 * @param the name of data
 * @param the value to set
 * @return void
 */
void probe_set_data_char(probe_s* probe, char* name, char value);

/**
 * set a certain char* field value named name in the probe data (also malloc it)
 * @param probe the probe to write
 * @param the name of data
 * @param the value to set
 * @return void
 */
void probe_set_data_string(probe_s* probe, char* name, char* value);

/**
 * set a certain float field value named name in the probe data (also malloc it)
 * @param probe the probe to write
 * @param the name of data
 * @param the value to set
 * @return void
 */
void probe_set_data_float(probe_s* probe, char* name, float value);

/**
 * add the probe to the list of probes
 * @param probe the probe to add
 * @return 1 if success -1 otherwise
 */

int add_probe_to_probes_list(probe_s* probe);

/**
 * remove the probe from the list of probes without suppressing it
 * @param probe the probe
 * @return 1 if success -1 otherwise
 */
int remove_probe_from_probes(probe_s* probe);

/**
 * check if a probe is in the completed probe list
 * @param probe the probe to check
 * @return 1 if present, 0 otherwise
 */
int is_probe_complete(probe_s* probe);

/**
 * add the probe to the list of completed probes
 * @param probe the complete probe
 * @return 1 if success -1 otherwise
 */

int add_probe_to_complete_probes(probe_s* probe);

/**
 * remove the probe from the list of complete probe and suppress it
 * @param probe the complete probe
 * @return 1 if success -1 otherwise
 */
int remove_probe_from_complete_probes(probe_s* probe);
/**
 * free all the probes contained in the lists
 * @param void
 * @return void
 */

void free_all_probes();

/**
 * give the number of probes that actually exists
 * @param void
 * @return number of probes in the 2 lists
 */
int get_number_of_probes();

#endif
