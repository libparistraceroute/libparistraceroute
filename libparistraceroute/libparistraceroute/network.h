#ifndef H_PATR_NETWORK
#define H_PATR_NETWORK

#include <netinet/in.h>
#include <sys/socket.h>
#include <netpacket/packet.h>
#include <net/ethernet.h> 

union sockaddr_union {
	struct sockaddr sa;
	struct sockaddr_ll sll;
	struct sockaddr_in sin;
	struct sockaddr_in6 sin6;
};
typedef union sockaddr_union sockaddr_u;

#include "algo.h"
#include "field.h"
#include "fieldlist.h"
#include "proto.h"
#include "probe.h"
#include "queue.h"
#include "sniffer.h"
#include "packet.h"

#define DEF_START_PORT	33434	/*  start for traditional udp method   */
#define DEF_UDP_PORT	53	/*  dns   */
#define DEF_TCP_PORT	80	/*  web   */
#define DEF_RAW_PROT	253	/*  for experimentation and testing, rfc3692  */



//gestion de ipv4 ou 6
//envoi et reception des paquets
//penser a l'option IP_HDRINCL pour eviter qu'il n'ajoute dson propre header IP
//le met grace a setsockopt()
//et user raw socket
//inet tout ca la
//les socket devront etre transmis a queue
//les paquets aussi et queue gerera envoi vers sockets

/**
 * prepare the queue : init the eventfd for queuing and
 * create a raw socket for sending all kind of data
 * @return the socket
 */
int create_queue();

/**
 * init the parameters for network
 * @param protocols : the protocols to get in the sniffer
 * @param nb_proto : number of protocols
 * @param callback : optional function to call when we receive a packet
 * @return field_s* to the handles, NULL if failed
 */

field_s* create_sniffer(char** protocols, int nb_proto,void (*callback) (snif_packet_s* packet, void* userparams),void* param);

/**
 * close the sniffer
 * @param handles : the handles to close
 * @return int 1 if success, -1 otherwise
 */

int close_sniffer(field_s** handles);

/**
 * close the sniffer, the sending socket, free the probes, free the list of protocol
 * @param socket the socket to close
 * @param handles : the handles to close
 * @return void
 */
void finish_network(int socket,field_s* handles);

/**
 * creates the packet, depending to options
   not working with eventfd (i e not action with it)
 * @param probe : the probe containing the informations
 * @param payload_size : the size of payload in the packet
 * @return int 1 if success, -1 otherwise
 */

packet_s* create_fill_packet_from_probe(probe_s* probe, int payload_size);

/**
 * send the packet trhought the specified socket, then frees it, and 
   put an * to the probe if no responses is got after timeout time
   not working with eventfd (i e not action with it)
 * @param packet : the packet to be sent, freed at the end
 * @param probe : the probe containing the informations
 * @param socket : the socket in which send the data
 * @param timeout : the time to wait for timeout in seconds
 * @return int 1 if success, -1 otherwise
 */

int send_packet(packet_s* packet, probe_s* probe, int socket);

/**
 * send the probe throught the network (using create_fill_packet and send_packet) PLUS writes in the eventfd
 * @param probe : the probe containing the informations
 * @param payload_size : the size of payload to add in packet
 * @param timeout : the time to wait for timeout in seconds
 * @param socket : the socket in which send the data
 * @return int 1 if success, -1 otherwise
 */
int send_probe(probe_s* probe, int payload, int socket);

/**
 * start the execution of algos addes with add_algo, then timeout and wait for replies
 * @param data : the data (in fields) that the algorithms may need (personnal)
 * @return int 1 if success, -1 otherwise
 */
int execution_algorithms(field_s* data);

/**
 * start the eventfd manager
 * @return int 1 if success, -1 otherwise
 */
int init_reply_probe_eventf();

#endif
