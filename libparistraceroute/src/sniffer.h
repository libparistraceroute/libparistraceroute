#ifndef H_PATR_SNIFFER
#define H_PATR_SNIFFER

//#include "../libcommon/libcommon.h"
#include "config.h"
#include "field.h"

#include <pthread.h>
#include <stdint.h>
#include <stddef.h>
#include <sys/types.h>

/**
  This file is based from the sniffer and the thread gestion of xavier cuvelier
 (see in trunk/libnet sniffer files)
*/



typedef struct snif_packet {
	/** Data */ u_int8_t* data;
	/** Length of the whole data array */ int length;
	/** Beginning of the IP header (not 0 if link layer present in the data) */int offset;
} snif_packet_s;

/**
 * initialize all the devices interfaces to get the handles
 * @param handles : the resulting array
 * @param nb_handles : the size of the array
 * @param protocols : the protocols to filter
 * @param nb_proto : the number of proto to filter
 * @param callback : an other callback function can be called rather than the default one. may be NULL
 * @param params : a optional parameter for the callback function may be NULL
 * @return 1 if succedded, -1 otherwise
 */
int init_sniffer(field_s** handles,char** protocols, int nb_proto,void (*callback) (snif_packet_s* packet, void* userparams), void* params);

/**
 * close the handles
 * @param handlevoid : the handles to close
 * @param nb_handles : the number of handles to close
 * @return 1 if succedded, -1 otherwise
 */
int finish_sniffer(field_s** handles);

/**
 * show the content of a received packet
 * @param packet : the packet to show
 * @return void
 */
void view_snif_packet(snif_packet_s* packet);


#ifdef USEPCAP
	#include <pcap.h>

	typedef struct sniffer_thread_t {
		/** Device name (for debugging mainly)*/ char* devname;
		/** Handler to access libpcap*/ pcap_t* pcap_hd;
	#ifdef BSDSOCKET
		/** File descriptor used by libpcap to see if data's are available*/ int pcap_fd;
	#endif
	#ifdef WINSOCKET
		/** Handle used by winpcap to see if data's are available*/ HANDLE w32event;
	#endif
		/** Length of the datalink header in bytes*/ int datalink_length;
		/** Indicates if the thread that listen for new packets is running*/ int thread_running;
		/** Indicates if the thread has been created*/ int thread_created;
		/** Thread that listens for new packets*/ pthread_t thread;
		/** Mutex for the thread*/ pthread_mutex_t lock;
		/** User's callback called when a packet is received by libpcap*/ void (*callback)(snif_packet_s* packet, void* userparams);
		/** Parameters passed by the caller and then passed to the callback*/ void* params;
	} sniffer_thread_s ;

	//int init_sniffer_pcap(void** handlesvoid, int* nb_handles,char** protocols, int nb_proto);

	//int finish_sniffer_pcap(void** handlevoids, int nb_handles);


#else // Raw sockets
	/// Handler for a raw socket listener
	typedef struct sniffer_thread_t {
		/** The file descriptor of the raw socket*/ int fd;
		/** Indicates if the thread that listen is running*/ int thread_running;
		/** Indicates if the thread has been created*/ int thread_created;
		/** The thread that listens for replies*/ pthread_t thread;
		/** Mutex used by the thread*/ pthread_mutex_t lock;
		/** Callback function called when a datagram is received on the socket*/ void (*callback)(snif_packet_s* packet, void* userparams);
		/** Parameters passed by the caller and then passed to the callback*/ void* params;
	}sniffer_thread_s ;
#endif


#endif  //H_PATR_SNIFFER
