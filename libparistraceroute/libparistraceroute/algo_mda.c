#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>
#include <errno.h>

#include "algo.h"
#include "probe.h"

/* inet_ntoa */
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
/* /inet_ntoa XXX this functionality should be provided by the lib */

//#define MDA_DEBUG 1

#define FIELDS_MDA 6

void nop(void) {return;}

typedef enum {
    MDA_TYPE_UNKNOWN,
    MDA_TYPE_FAILED,
    MDA_TYPE_NONE,
    MDA_TYPE_PPLB,
    MDA_TYPE_PFLB,
    MDA_TYPE_PDLB
} mda_lb_type_t;
/*
 * Inspired from previous paristraceroute + scamper implementations
 */

/*
 * NOTES:
 *
 *  - probe array should be managed by the library, we know we can use probe
 *  addresses safely in our algorithm.
 *
 */

typedef enum {
    MDA_NOT_SET,
    MDA_INIT,
    MDA_NEXT_TTL,
    MDA_NEXTHOPS,
    MDA_POPULATE_PREV_HOP,
    MDA_ENUMERATE_DONE,
    MDA_CLASSIF,
    MDA_CLASSIF_DONE,
    MDA_TERMINATE
} mda_state_t;

/* These are mandatory fields */
/* XXX We should mimic protocol here */
/* XXX difference between existing fields and newly defined fields ? */
/* We could provide a pointer to the structure */
char* personal_fields_mda[FIELDS_MDA] = {
    "confidence",   /* default: 95, values=95,99 */
    "min_ttl",      /* default: 1 */
    "max_ttl",      /* default: 32 */
    "per_dest",     /* detect per destination load balancers */
    "max_missing",  /* */
    "dest_addr"     /* mandatory */
    /* NULL */
};

#define MDA_DFT_CONFIDENCE 95
#define MDA_DFT_MIN_TTL 1
#define MDA_DFT_MAX_TTL 32
#define MDA_DFT_PER_DEST 0
#define MDA_DFT_MAX_MISSING 5 /* ? */

#define MAX_INTERFACES 10 /* XXX ugly */
#define MAX_PROBES 100
#define MAX_PROBEMAPS 100

typedef struct mda_interface_s {
    char *address; //unsigned int address;
    mda_lb_type_t type;
    unsigned int nb_interfaces_expected;
    struct mda_interface_s *next_hops[MAX_INTERFACES];
    unsigned nb_next_hops;
    unsigned int nb_probes_needed;
    unsigned int nb_probes_available;
    unsigned int probe_max_id;
    probe_s *probes[MAX_PROBES]; /* XXX ugly */
} mda_interface_t;

typedef struct mda_probemap_s {
    /*
    unsigned char ttl;
    unsigned int nb_sent;
    unsigned int nb_recv;
    */
    unsigned int nb_interfaces;
    unsigned int nb_probes;
    unsigned int max_probes;
    probe_s *probes[MAX_PROBES]; /* XXX ugly */
    mda_interface_t *interfaces[MAX_INTERFACES]; /* XXX ugly, should be dynamic array */
    
    int last_sent_id;
} mda_probemap_t;

/*
 * MDA own data
 */
typedef struct mda_data_s {

    pthread_mutex_t lock;
    /* parameters */
    unsigned char confidence;
    unsigned char min_ttl;
    unsigned char max_ttl;
    unsigned char per_dest;
    char *dest_addr;
    unsigned short dest_port;
//    unsigned int target; /* redundant ? */
//    unsigned int target_prefix;
    unsigned int id_initial;
    unsigned int max_missing;
    unsigned int missing;
    unsigned char dest_reached;
    unsigned int flow_id;
    /* XXX should not be here */

    /* algorithm */
	algo_s* algo;

    /* state */
    mda_state_t state;              /* state for the exploration FSM */
    unsigned int showprobes;
    unsigned int state_classif; /* id of the interface we will classify */
    mda_interface_t *classif_expected_interface;
    unsigned int next_tag;          /* next tag to be sent */
    unsigned char cur_ttl;          /* currently explored ttl */
    unsigned int nb_tosend;         /* number of probes to send for the current ttl */
    unsigned int prev_nb_tosend;    /* number of probes to send for the current ttl */
    unsigned int nb_sent;           /* number of probes sent for the current ttl */
    unsigned int nb_recv;           /* number of probes received for the current ttl */
    unsigned int nb_interfaces_expected;
    unsigned int current_id;        /* id for sending probes */
    unsigned int prev_interface_id;
    mda_probemap_t *probes_by_ttl2[MAX_PROBEMAPS]; /* XXX ugly */
    mda_probemap_t *prev_probemap;  /* cur_ttl sufficient ? */
    mda_probemap_t *cur_probemap;;  /* cur_ttl sufficient ? */

    int last_sent_id;
    unsigned int nb_probes_in_flight;
} mda_data_t;

typedef struct probe_data_s {
    mda_interface_t* classif_interf;
    mda_interface_t *interf;
    char *host_address_raw;
    mda_data_t *data; /* should not have to be saved into the probe */
} probe_data_t;

mda_interface_t *create_interface(char *addr)//unsigned int addr)
{
    mda_interface_t *interface;
    //mda_interface_t empty[MAX_INTERFACES] = {NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL};
    int i;

    interface = (mda_interface_t*)malloc(sizeof(mda_interface_t));
    if (!interface)
        /* XXX error */
        return NULL;


    interface->nb_next_hops = 0;
    interface->type = MDA_TYPE_UNKNOWN;
    interface->nb_interfaces_expected = 0;
    if (addr)
        interface->address = strdup(addr);
    else
        interface->address = NULL;
    for (i=0; i < MAX_PROBES; i++)
        interface->probes[i] = NULL;

    interface->probe_max_id = 0;
    interface->nb_probes_available = 0;
    interface->nb_probes_needed = 0;
    return interface;
}

void free_interface(mda_interface_t *interface)
{
    int i;
    if (interface) {
        if (interface->address) {
            free(interface->address);
            interface->address = NULL;
        }
        for (i=0; i < MAX_PROBES; i++)
            if (interface->probes[i]) {
                free_probe(interface->probes[i]);
            }
        free(interface);
        interface = NULL;
    }
}

mda_probemap_t* create_mda_probemap(void)
{
    mda_probemap_t *mda_probemap;
    int i;

    mda_probemap = (mda_probemap_t*)malloc(sizeof(mda_probemap_t));
    if (!mda_probemap)
        /* XXX errors */
        return NULL;

    mda_probemap->nb_interfaces = 0;
    mda_probemap->last_sent_id = -1;
    mda_probemap->nb_probes = 0;
    mda_probemap->max_probes = 0;
    for (i=0; i < MAX_PROBES; i++)
        mda_probemap->probes[i] = NULL;
    for (i=0; i < MAX_INTERFACES; i++)
        mda_probemap->interfaces[i] = NULL;
    return mda_probemap;
}

void free_mda_probemap(mda_probemap_t *mda_probemap)
{
    int i;
    if (mda_probemap) {
        for (i=0; i < MAX_PROBES; i++)
            if (mda_probemap->probes[i])
                free_probe(mda_probemap->probes[i]);
        for (i=0; i < MAX_INTERFACES; i++)
            if (mda_probemap->interfaces[i])
                free_interface(mda_probemap->interfaces[i]);
        free(mda_probemap);
        mda_probemap = NULL;
    }
}

// borrowed from scamper
static int k(mda_data_t *data, int n)
{
    /*
     * number of probes (k) to send to rule out a load-balancer having n hops;
     * 95% confidence level first from 823-augustin-e2emon.pdf, then extended
     * with gmp-based code.
     * 99% confidence derived with gmp-based code.
     */
    static const int k[][2] = {
        {   0,   0 }, {   0,   0 }, {   6,   8 }, {  11,  15 }, {  16,  21 },
        {  21,  28 }, {  27,  36 }, {  33,  43 }, {  38,  51 }, {  44,  58 },
        {  51,  66 }, {  57,  74 }, {  63,  82 }, {  70,  90 }, {  76,  98 },
        {  83, 106 }, {  90, 115 }, {  96, 123 }, { 103, 132 }, { 110, 140 },
        { 117, 149 }, { 124, 157 }, { 131, 166 }, { 138, 175 }, { 145, 183 },
        { 152, 192 }, { 159, 201 }, { 167, 210 }, { 174, 219 }, { 181, 228 },
        { 189, 237 }, { 196, 246 }, { 203, 255 }, { 211, 264 }, { 218, 273 },
        { 226, 282 }, { 233, 291 }, { 241, 300 }, { 248, 309 }, { 256, 319 },
        { 264, 328 }, { 271, 337 }, { 279, 347 }, { 287, 356 }, { 294, 365 },
        { 302, 375 }, { 310, 384 }, { 318, 393 }, { 326, 403 }, { 333, 412 },
        { 341, 422 }, { 349, 431 }, { 357, 441 }, { 365, 450 }, { 373, 460 },
        { 381, 470 }, { 389, 479 }, { 397, 489 }, { 405, 499 }, { 413, 508 },
        { 421, 518 }, { 429, 528 }, { 437, 537 }, { 445, 547 }, { 453, 557 },
        { 462, 566 }, { 470, 576 }, { 478, 586 }, { 486, 596 }, { 494, 606 },
        { 502, 616 }, { 511, 625 }, { 519, 635 }, { 527, 645 }, { 535, 655 },
        { 544, 665 }, { 552, 675 }, { 560, 685 }, { 569, 695 }, { 577, 705 },
        { 585, 715 }, { 594, 725 }, { 602, 735 }, { 610, 745 }, { 619, 755 },
        { 627, 765 }, { 635, 775 }, { 644, 785 }, { 652, 795 }, { 661, 805 },
        { 669, 815 }, { 678, 825 }, { 686, 835 }, { 695, 845 }, { 703, 855 },
        { 712, 866 }, { 720, 876 }, { 729, 886 }, { 737, 896 }, { 746, 906 },
    };

    #define TRACELB_CONFIDENCE_MAX_N 99
    #define TRACELB_CONFIDENCE_NLIMIT(v) \
    ((v) <= TRACELB_CONFIDENCE_MAX_N ? (v) : TRACELB_CONFIDENCE_MAX_N)

    return k[n][(data->confidence==95)?0:1];
}

struct output_ttl_params {
    int ttl;
};

int signal(output_signal_t signal, void *params, mda_data_t *data) 
{
    /* field_s* probes = data->algo->options->probes; */
    mda_probemap_t *probemap;
    int i, j, k, ttl;

    switch (signal) {
        case OUTPUT_INIT:
            break;
        case OUTPUT_TTL_DONE:
            /* We print all probes with a given ttl */
            
            ttl = ((struct output_ttl_params*)params)->ttl;
            probemap = data->probes_by_ttl2[ttl];

            /* ttl */
            printf("%d\t", ttl);

            printf("P(X,X,X,X)\t");

            for (i = 0; i < probemap->nb_interfaces; i++) {
                /* address */
                printf("%s ", probemap->interfaces[i]->address);

                /* type */
                switch (probemap->interfaces[i]->type) {
                    case MDA_TYPE_PPLB: printf("< "); break;
                    case MDA_TYPE_PFLB: printf("= "); break;
                    case MDA_TYPE_PDLB: printf("[ "); break;
                    default: break;
                }

                if (data->showprobes == 1) {
                    int first = 1;
                    printf("[");
                    for (j = 0; j <= probemap->max_probes; j++) {
                        if (!probemap->probes[j])
                            continue;
                        probe_data_t *probe_data = (probe_data_t*)probemap->probes[j]->perso_opt;
                        if (probe_data->interf == probemap->interfaces[i]) {
                            if (first == 0)
                                printf(",");
                            printf("%d", j);
                            first = 0;
                        }
                    }
                    printf("]");
                }

                switch (probemap->interfaces[i]->type) {
                    case MDA_TYPE_PPLB: data->showprobes = 0; break;
                    case MDA_TYPE_PFLB: data->showprobes = 1; break;
                    default: break;
                }
                printf(" ");
            }
            printf("\n");
            break;

        case OUTPUT_TRACEROUTE_DONE:

            printf("\nTraceroute complete:\n");
            for (i = 1; i < MAX_PROBEMAPS; i++) {
                probemap = data->probes_by_ttl2[i];
                if (!probemap)
                    break;
                for (j = 0; j < probemap->nb_interfaces; j++) {
                    for (k = 0; k < probemap->interfaces[j]->nb_next_hops; k++) {
                        printf("%s -> %s\n", probemap->interfaces[j]->address, probemap->interfaces[j]->next_hops[k]->address);
                    }
                }
            }
            break;

        default:
            break;
    }
    return 1;
}

mda_data_t* create_mda_data(field_s *fields, algo_s *algo)
{
    mda_data_t *data;
    void *ret;
    int i;


    data = (mda_data_t*)malloc(sizeof(mda_data_t));
    if (!data)
        /* XXX How to handle errors */
        return NULL;

    /* data available in algo->options ? */
    /* probes available in algo->probes */

    /* XXX this should be automatic from description */
    /* XXX when are fields unallocated ? */

    /* mandatory fields */
    data->dest_addr = strdup(get_field_value(fields, "dest_addr"));

    /* optional fields */
    ret = get_field_value(fields, "confidence");
    data->confidence = ret ? *(unsigned char*)ret : MDA_DFT_CONFIDENCE;
    ret = get_field_value(fields, "min_ttl");
    data->min_ttl = ret ? *(unsigned char*)ret : MDA_DFT_MIN_TTL;
    ret = get_field_value(fields, "max_ttl");
    data->max_ttl = ret ? *(unsigned char*)ret : MDA_DFT_MAX_TTL;
    ret = get_field_value(fields, "per_dest");
    data->per_dest = ret ? *(unsigned char*)ret : MDA_DFT_PER_DEST;
    ret = get_field_value(fields, "max_missing");
    data->max_missing = ret ? *(unsigned char*)ret : MDA_DFT_MAX_MISSING;

    data->algo = algo;

    /* initialize mda state */
    data->state = MDA_NOT_SET;
    data->cur_ttl = data->min_ttl;
    for (i=0; i<MAX_PROBEMAPS; i++)
        data->probes_by_ttl2[i] = NULL;
    data->missing = 0;
    data->max_missing = 3;
    data->dest_reached = 0;
    data->prev_nb_tosend = 0;
    data->nb_probes_in_flight = 0;
    data->last_sent_id = -1;
    data->showprobes = 0;

    if (pthread_mutex_init(&data->lock, NULL) != 0) 
        return NULL;

    return data;
}

void free_mda_data(mda_data_t* data)
{
    int i;
    if (data) {
        pthread_mutex_destroy(&data->lock);
        for (i=0; i<MAX_PROBEMAPS; i++)
            if (data->probes_by_ttl2[i])
                free_mda_probemap(data->probes_by_ttl2[i]);
        free(data->dest_addr);
        data->dest_addr = NULL;
        free(data);
        data = NULL;
    }
}

probe_data_t *create_probe_data(void)
{
    probe_data_t *data;
    data = (probe_data_t*)malloc(sizeof(probe_data_t));
    if (!data) {
        perror("Out of memory: cannot allocate probe");
        return NULL;
    }
    return data;
}

void free_probe_data(probe_data_t *data)
{
    if (data) {
        free(data);
        data = NULL;
    }
}


mda_interface_t *find_mda_interface(mda_probemap_t *probemap, char *interface) //unsigned int interface)
{
    int i;
    for(i = 0; i < probemap->nb_interfaces; i++)
        if (strcmp(probemap->interfaces[i]->address, interface) == 0)
            return probemap->interfaces[i];
    return NULL;
}

mda_interface_t *add_mda_interface(mda_probemap_t *probemap,char *addr)// unsigned int addr)
{
    mda_interface_t *interface;

    interface = find_mda_interface(probemap, addr);
    if (interface)
        return interface;

    interface = create_interface(addr);
    probemap->interfaces[probemap->nb_interfaces] = interface;
    probemap->nb_interfaces++;

#ifdef MDA_DEBUG
    //fprintf(stderr, "%d.[%d] add_interface %d %s\n", id_initial, probemap->ttl,
    //        probemap->nb_interfaces, my_inet_ntoa(addr));
#endif
    return interface;
}

/* function very similar to add_mda_interface & find_mda_interface */
void add_next_hop_interface(mda_interface_t *interface1, mda_interface_t *interface2)
{
    int i;
#ifdef MDA_DEBUG
    fprintf(stderr, "searching interface among %d\n", interface1->nb_next_hops);
#endif
	for (i = 0; i < interface1->nb_next_hops; i++)
		if (strcmp(interface1->next_hops[i]->address, interface2->address) == 0)
			return;

#ifdef MDA_DEBUG
    fprintf(stderr, "not found. add it %d %d\n", interface1->max_next_hops, interface1->nb_next_hops);
    /* XXX How to deal with loops */
#endif

	interface1->next_hops[interface1->nb_next_hops] = interface2;
	interface1->nb_next_hops++;
}

/*
 * Terminate algo
 */
void mda_terminate(void *mda_data) {
	mda_data_t* data = (mda_data_t*)mda_data;
	free_mda_data(data);
    exit(EXIT_SUCCESS);
}


///**
// * This function is used to craft the probes until proper support for the probe interface is available
// */
//probe_s* mda_craft_probe(mda_data_t *data, char *target, unsigned short dport, unsigned char ttl) {
//    algo_s *algo;
//    probe_s *probe;
//    char **protocols = {"ipv4", "tcp", NULL};
//    struct tuple **idl = { {"udp", "csum"}, NULL };
//    struct tuple **fidl = { {"udp", "dport"}, NULL };
//
//    probe = create_probe("ipv4");
//
//    /* probe defaults should be handled by create */
//    probe->arrival_time = 0;
//
//    /* PACKET CONTENT */
//    /* Note: for performance, one could just use the packet interface, instead of the probe interface */
//
//    probe_set_protocols(probe, protocols);
//
//    /* We update the fields for the purposes of our algorithm */
//    /* probe_set("ip", "src", xxx); omitted DEFAULT used */
//    probe_set("ipv4", "dst", target);
//    probe_set("ipv4", "ttl", ttl); /* how to use constants ? */
//
//    /* Default fields should not have to be updated */
//    probe_set(probe, "ip_id", 0);
//
//    udp = create_probe("udp");
//    probe_set(udp, "sport", xxx);
//    probe_set(udp, "dport", ports[data->flow_id]);
//
//    /* This automatically sets up as the lower layer protocol */
//    probe_set(probe, "data", udp);
//
//    /* what are those fields
//    probe_set(probe, "host_address", 0);
//    probe_set(probe, "host_name", NULL);
//    */
//
//    /* Here would could just say we tag dport to carry flow_id */
//    probe_set_id_location(idl);
//    probe_set_flow_id_location(fidl);
//
//    /* STRUCTURE CONTENT */
//
//    /* Note:
//     *  - as part of the structure, probes have probe_id and flow_id
//     *  - setting this will imply some tags will be automatically changed by the
//     *  system
//     *  - cf the issue of tag, and the list of fields we tag
//     *  - TODO each protocol tells the system which fields can be tagged 
//     */
//    return probe;
//}

/*
 * count = combien de probes faut envoyer en tout
 * nb_probes = combien de probes on a envoyé
 * constant and next parameters is used to classify probes: we have a constant flow identifier
 *
 * each flow_id == a specific port
 *
 * CLASSIFY
 *  flow id does not change
 */
/* Here we have probes to send */


probe_s *mda_craft_probe(mda_data_t *data, unsigned int ttl, unsigned int flow_id) {
    unsigned short port;
    probe_s *probe;
    probe_data_t *probe_data;

    port = 30000 + flow_id; /* XXX */

    probe_data = create_probe_data();
    probe_data->data = data;
    probe_data->classif_interf = NULL;//interf;
    probe_data->interf = 0;

    probe = create_probe(create_name_probe(),data->algo->options->protocols, data->algo->options->nb_proto, data->algo, probe_data);
    probe_set_data_string(probe,"dest_addr", data->dest_addr); //my_inet_ntoa(data->target));
    probe_set_data_short(probe,"dest_port", port);
    probe_set_data_short(probe,"src_port", 33333);
    probe_set_data_char(probe,"TTL", ttl);
    probe->flow_id = flow_id;
    /* What is this for ?*/
    data->algo->options->probes = add_field(data->algo->options->probes, probe->name , probe);

    return probe;

}

void mda_do(mda_data_t *data)
{
    mda_probemap_t *probemap = data->cur_probemap; /* for convenience */
    mda_interface_t *interface;
    probe_s *probe;
    probe_data_t *probe_data;
    unsigned int inflight;
    unsigned int i,j;

    pthread_mutex_lock(&data->lock);

#ifdef MDA_DEBUG
    printf("mda_do()\n");
#endif

    for(;;) {
#ifdef MDA_DEBUG
        printf("mda_do loop\n");
#endif
        
        switch (data->state) {

            case MDA_INIT:
#ifdef MDA_DEBUG
                printf("MDA_INIT\n");
#endif
                /* target can be changed while probing dest_addr, for example for PDLB*/
                //data->target = data->dest_addr; 
                //data->target_prefix = data->target & 0xffffff;

                /* create a fake first hop */
                data->prev_probemap = create_mda_probemap();
                data->prev_probemap->interfaces[0] = create_interface(NULL);
                data->prev_probemap->nb_interfaces = 1;

            case MDA_NEXT_TTL:
#ifdef MDA_DEBUG
                printf("=================================");
                printf("MDA_NEXT_TTL : %d\n", data->cur_ttl);
#endif
                /* not for init but to be reused later */
                data->nb_sent = 0; /* redundant ? */
                data->nb_recv = 0;
                data->flow_id = 0;
                data->nb_interfaces_expected = 1;
                data->nb_tosend = 6; /* XXX */
                /* end not */

                /* the list of probes per ttl */
                data->cur_probemap = create_mda_probemap();
                data->probes_by_ttl2[data->cur_ttl] = data->cur_probemap;
                probemap = data->cur_probemap;
                probemap->nb_interfaces = 0;

                data->prev_interface_id = 0;
                data->state = MDA_NEXTHOPS;
                data->nb_recv = 0;
                data->nb_sent = 0;
                data->last_sent_id = -1;
                data->flow_id = 0;

            case MDA_NEXTHOPS:
                /* Find the next hops of interfaces data->prev_probemap->interfaces[data->prev_interface_id]
                 * at hop R */
                interface = data->prev_probemap->interfaces[data->prev_interface_id];

                interface->nb_interfaces_expected = interface->nb_next_hops + 1;
                if (interface->nb_interfaces_expected < 2)
                    interface->nb_interfaces_expected = 2;

                /* Nb probes to send to current hop, going through considered
                 * interface at previous hop */
                interface->nb_probes_needed = k(data, interface->nb_interfaces_expected);
                
                /* are we done ? */
                if (data->nb_recv >= interface->nb_probes_needed) {
                    data->state = MDA_CLASSIF;
                    break;
                }

                /* If not, do we have enough probes available at previous hop */
                if (data->prev_probemap->nb_interfaces > 1) {
                    if (interface->nb_probes_available <  interface->nb_probes_needed) {
                        /* We will do it at the last probe */
                        if (data->nb_probes_in_flight > 0) {
                            pthread_mutex_unlock(&data->lock);
                            return;
                        }
                        data->state = MDA_POPULATE_PREV_HOP;
                        break;
                    } else {
                        /* Enough probes, enumerate hops at this hop, send all probes */
                        for(i = data->last_sent_id+1; i <= data->prev_probemap->max_probes; i++) {
                            /* Loop though probes that might be sent: exist at the
                             * previous hop and still not sent here */
                            if (!data->prev_probemap->probes[i])
                                continue;
                            probe_data_t *probe_data = (probe_data_t*)data->prev_probemap->probes[i]->perso_opt;
                            if (probe_data->interf == interface) {
                                data->last_sent_id = i;
                                data->nb_probes_in_flight++;
                                probe = mda_craft_probe(data, data->cur_ttl, i); //data->prev_probemap->probes[i]->flow_id);
                                data->nb_sent++;
                                send_probe(probe, 0, data->algo->options->sending_socket);
                                probemap->probes[i] = probe;
                                if (probemap->max_probes < i)
                                    probemap->max_probes = i;
                                probemap->nb_probes++;
                            }
                            /* we might have more probes than needed, break loop */
                            if (data->nb_sent >= interface->nb_probes_needed) {
                                break;
                            }
                        }
                    }
                } else {
                    /* Only one interface at previous hop */
                    for (i =  data->nb_sent; i < interface->nb_probes_needed; i++) {
                        data->nb_probes_in_flight++;
                        data->nb_sent++;
                        probe = mda_craft_probe(data, data->cur_ttl, i); // idem
                        send_probe(probe, 0, data->algo->options->sending_socket);
                        probemap->probes[i] = probe;
                        if (probemap->max_probes < i)
                            probemap->max_probes = i;
                        probemap->nb_probes++;
                    }
                }

                pthread_mutex_unlock(&data->lock);
                return;

            case MDA_POPULATE_PREV_HOP:
                interface = data->prev_probemap->interfaces[data->prev_interface_id];

                /* We need nb_prev probes available at this hop */

                /* We might come here after receiving a probe reply; if
                 * receiving a reply hasn't helped, this loop will send one more
                 * probe */

                if (interface->nb_probes_available >=  interface->nb_probes_needed) {
                    /* We have enough probes available */
                    data->state = MDA_NEXTHOPS;
                    break;
                }

                inflight = data->nb_probes_in_flight;
                for (i=interface->nb_probes_available; i < interface->nb_probes_needed - inflight; i++) {
                    /* Here we send exactly the number of missing probes, though,
                     * knowing only a fraction will arrive at this interface, we
                     * could send a bigger batch. We might generate a bit more
                     * probes, but we will be quicker */
                    data->nb_probes_in_flight++;
                    probe = mda_craft_probe(data, data->cur_ttl-1, data->flow_id);
                    send_probe(probe, 0, data->algo->options->sending_socket);
                    data->prev_probemap->max_probes++;
                    data->prev_probemap->probes[data-> prev_probemap->max_probes] = probe;
                    data->prev_probemap->nb_probes++;
                    data->flow_id++;
                }
                /* We need to wait for more answers before continueing */
                pthread_mutex_unlock(&data->lock);
                return;

            case MDA_CLASSIF:
                /* We need to pick one probe, and send 5 others  with the same flow_id, to check whether they all go to the same interface */
                interface = data->prev_probemap->interfaces[data->prev_interface_id];
                if (interface->nb_next_hops == 1) {
                    /* No need to classify */
                    data->prev_probemap->interfaces[data->prev_interface_id]->type = MDA_TYPE_NONE;
                    data->state = MDA_CLASSIF_DONE;
                    break;
                }

                for (i = 0; i < data->prev_probemap->nb_probes; i++) {
                    probe_data = (probe_data_t*)data->prev_probemap->probes[i]->perso_opt;
                    if (probe_data->interf == interface) {
                        break;
                    }
                }
                /* We have one probe for sure at position i */

                probe_data = (probe_data_t*)data->cur_probemap->probes[i]->perso_opt;
                data->classif_expected_interface = probe_data->interf;
                for (j = 0; j < 5; j++) {
                    probe = mda_craft_probe(data, data->cur_ttl, i);
                    data->nb_probes_in_flight++;
                    send_probe(probe, 0, data->algo->options->sending_socket);
                }
                pthread_mutex_unlock(&data->lock);
                return;

            case MDA_CLASSIF_DONE:

                data->prev_interface_id++;
                if (data->prev_interface_id >= data->prev_probemap->nb_interfaces) {
                    data->state = MDA_ENUMERATE_DONE;
                } else {
                    data->nb_recv = 0; /* Used to know how many replies we got for a given interface */
                    data->nb_sent = 0;
                    data->last_sent_id = -1;
                    data->flow_id = 0;
                    data->state = MDA_NEXTHOPS;
                }
                break;

            case MDA_ENUMERATE_DONE:

                /*
                data->missing = (data->nb_recv == 0) ? (data->missing + 1) : 0;

                if (data->missing >= data->max_missing) {
#ifdef MDA_DEBUG
                    printf("Too many down hops -> stop algo");
#endif
                    data->state = MDA_TERMINATE;
                    break;
                }
                */

                /* delete the fake mapprobes created at the beginning */
                if (data->cur_ttl == data->min_ttl)
                    free_mda_probemap(data->prev_probemap);

                /* Send TTL_DONE signal to output */
                //out = data->algo->options->output_kind;
                //out->show_all_probes(data->algo->options->probes);
                
                /* We should not have to give probes as parameters */
                if (data->cur_ttl > 1) {
                    struct output_ttl_params p;
                    p.ttl = data->cur_ttl-1;
                    signal(OUTPUT_TTL_DONE, &p, data);
                }
                data->prev_probemap = probemap;

                data->cur_ttl++;

                /* STOP CONDITION : destination reached and only one interface at current hop */
                if (data->dest_reached && probemap->nb_interfaces == 1) {
                    data->state = MDA_TERMINATE;
                    break;
                }
		        // output
                data->state = MDA_NEXT_TTL;
                break;

            case MDA_TERMINATE:
                {
                    struct output_ttl_params p;
                    p.ttl = data->cur_ttl-1;
                    signal(OUTPUT_TTL_DONE, &p, data);
                }
                signal(OUTPUT_TRACEROUTE_DONE, NULL, data);
#ifdef MDA_DEBUG
                printf("MDA_TERMINATE\n");
#endif
                mda_terminate((void*)data);
#ifdef MDA_DEBUG
                printf("RETURN\n");
#endif
                return;

            default:
                break;
        }
    }
    
}


/*
 * Initialize algo
 *
 *  - we expect options and fields to be non NULL
 *  - we expect all _mandatory_ fields to be present (cf protocols)
 *    and have correct values
 */
void mda_init(algo_s* algo, field_s* fields)
{
    mda_data_t* data = create_mda_data(fields, algo); /* we should not need algo */

    data->state = MDA_INIT;
    mda_do(data);
}


/*
 * Reply
 *
 *  - we expect data and data->global_options to be not NULL (?)
 *
 */
void mda_on_reply(probe_s* probe)
{
    probe_data_t *probe_data = (probe_data_t*)probe->perso_opt;
	mda_data_t* data = probe_data->data;
    mda_interface_t *classif, *interface;

    pthread_mutex_lock(&data->lock);

    char *src = probe_get_reply_addr(probe); //, "source_addr");

    /* have we reached the destination ? : cf wakeup */
    if (probe_get_reply_kind(probe) == END_RESP)
        data->dest_reached = 1;

    /* Adding interfaces should be hop agnostic */

    data->nb_probes_in_flight--;

    switch (data->state) {
        case MDA_NEXTHOPS:
            /* Received a new probes for enumerating interfaces at hop h */
            /* We know it is a next hop for prev interface */
            interface = add_mda_interface(data->cur_probemap, src);
            probe_data->interf = interface;
            interface->nb_probes_available++;

            add_next_hop_interface(data->prev_probemap->interfaces[data->prev_interface_id],
                    interface);
            data->nb_recv++;
            break;

        case MDA_POPULATE_PREV_HOP:
            /* We are tying to populate prev hop and just received an answer */
            interface = add_mda_interface(data->prev_probemap, src);
            probe_data->interf = interface;
            interface->nb_probes_available++;
            break;

        case MDA_CLASSIF:
            /* need to be equal to this interface */
            interface = find_mda_interface(data->cur_probemap, src);
            classif = data->prev_probemap->interfaces[data->prev_interface_id];
            if (!interface)
                /* XXX unseen interface ! */
                classif->type = MDA_TYPE_FAILED;
            else {
                if (interface == data->classif_expected_interface) {
                    if ((data->nb_probes_in_flight == 0) && (classif->type == MDA_TYPE_UNKNOWN)) {
                        /* If we haven't set the type yet, we have a PFLB */
                        classif->type = MDA_TYPE_PFLB;
                    }
                } else {
                    classif->type = MDA_TYPE_PPLB;
                }
            }
            
            if (data->nb_probes_in_flight == 0) {
                data->state = MDA_CLASSIF_DONE;
                break;
            } else {
                pthread_mutex_unlock(&data->lock);
                return;
            }
        default:
            pthread_mutex_unlock(&data->lock);
            return;
    }
    /* If we had a reply, increase the number of available probes for
     * this interface */
    pthread_mutex_unlock(&data->lock);
    mda_do(data);
}

static algo_list_s mda_ops = {
	.next=NULL,
	.name="mda",
	.name_size=3,
	.personal_fields = personal_fields_mda,
	.nb_field = FIELDS_MDA,
	.init=mda_init,
	.on_reply=mda_on_reply,
    /*
    .terminate = mda_terminate,
    */
};

ADD_ALGO(mda_ops);
