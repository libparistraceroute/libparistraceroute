#include "sniffer.h"
#include "network.h"
#include "proto.h"

#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <string.h>
#include <stdlib.h>
#include <netinet/ip.h>
#include <netinet/ip_icmp.h>

typedef struct iphdr iphdr_s;
typedef struct icmphdr icmphdr_s;

void* run_sniffer_thread(void* thread);

#ifdef USEPCAP
char* create_filter(char** protocols, int nb_proto) {
        // Get the filter expression
        char expr[128];
        expr[0] = '\0';
        if (protocols != NULL&&*protocols!=NULL) {
	        int first = 1;
		int i = 0;
	        while (i < nb_proto) {
        	        // Abort if there is no enough place in the buffer
	                if (strlen(expr) + 16 >= 128) {
				#ifdef DEBUG
				fprintf(stderr,"create_filter_pcap : filter is too long. cutting it to :\n%s\n",expr);
				#endif
				return strdup(expr);
			}
	                if (protocols[i] == NULL){
				#ifdef DEBUG
				fprintf(stderr,"create_filter_pcap : protocol n°%i is empty. final string :\n%s\n",i,expr);
				#endif
				return strdup(expr);
			}
	                if (first==0) strcat(expr, " or ");
	                strncat(expr, protocols[i], 12);
	                //free(proto_name);
			first = 0;
	                i++;
	        }
	}
	#ifdef DEBUG
        fprintf(stdout,"filter expr: %s\n", expr);
	#endif
        return strdup(expr);
}

/////adapted from jordan augé
/*
void callback_pcap_loop (u_char * user, const struct pcap_pkthdr *h, const u_char * buff){
	iphdr_s *iph = (iphdr_s *) (buff + sizeof (struct ethhdr));
	//check if ipv4 or 6 TODO: other protocols? ethernet and ip lvl
	if(iph->version==4){//IPv4
	
	}
	else{
		#ifdef DEBUG
		fprintf(stderr,"callback from pcap: unknown protocol\n");
		#endif
		return;
	}
}*/




void view_snif_packet(snif_packet_s* packet){
	int i=0,j=0;
	unsigned char* data = packet->data;
	fprintf(stdout,"information about sniffed packet\n");
	fprintf(stdout,"packet of lenght %d where IP begin at the offset : %d \n",packet->length,packet->offset);
	fprintf(stdout,"\nsniffed data :\n");
	for(i=0;i<packet->length;i=i+8){
		if(packet->length-i<8){
			for(j=0;j<packet->length-i;j++){
				fprintf(stdout,"%02x ",(unsigned char) (data)[j+i]);
			}
			fprintf(stdout,"\n");
		}
		else{
			fprintf(stdout,"%02x %02x %02x %02x %02x %02x %02x %02x\n",(unsigned char) (data)[i],(unsigned char) (data)[i+1],(unsigned char) (data)[i+2],(unsigned char) (data)[i+3],(unsigned char) (data)[i+4],(unsigned char) (data)[i+5],(unsigned char) (data)[i+6],(unsigned char) (data)[i+7]);
		}
	}
}

int get_datalink_layer_size(pcap_t* handle){
        int datalink_type   = pcap_datalink(handle);
        int datalink_length = 0;
        switch (datalink_type) {
                case DLT_NULL:          // BSD loopback
                case DLT_LOOP:          // OpenBSD loopback
                        datalink_length = 4;
                        break;
                case DLT_EN10MB:        // Ethernet
                        datalink_length = 14; //??
                        break;
                case DLT_RAW:           // Direct IP
                        datalink_length = 0;
                        break;
                case DLT_IEEE802_11:    // IEEE802.11 wireless lan
					//seems that size varies depending on kind of encryption
                case DLT_PPP:           // PPP
                case DLT_IEEE802_11_RADIO: // ??
                default:    
			#ifdef DEBUG
                        fprintf(stderr,"get_datakink_layer_size : Unsupporterd link layer\n");
			#endif
			return -1;
        }
	return datalink_length;
}

int init_sniffer(field_s** handles,char** protocols, int nb_proto,void (*callback) (snif_packet_s* packet, void* userparams), void* params){
	*handles=NULL;
	char errbuf[PCAP_ERRBUF_SIZE];
	//*nb_handles=0;
	pcap_if_t *ifs;
	int res = pcap_findalldevs(&ifs, errbuf);
        if (res == -1) {
		#ifdef DEBUG
		fprintf(stderr,"init_sniffer_pcap_interfaces : Cannot get all interfaces: %s\n", (char*)errbuf);
		#endif
		return -1;
        }

        // Check wich interface correspond to the right source address: 'src'
        pcap_if_t* i = ifs;
	if(ifs==NULL){
		#ifdef DEBUG
		fprintf(stderr,"init_sniffer_pcap_interfaces : No interfaces found. end.\n");
		#endif
                return -1;
	}
        while (i != NULL) {
		#ifdef DEBUG
		#endif
		pcap_t* handle;//TODO
		//handle open in non-promiscuous mode
		handle = pcap_open_live(i->name, BUFSIZ, 0, 10, errbuf);
		if (handle == NULL) {
			#ifdef DEBUG
			fprintf(stderr, "init_sniffer_pcap_interfaces : Couldn't open device %s: %s\n", i->name, errbuf);
			#endif
		}
		else{
			//(*nb_handles)++;
			//handles=realloc(handles,(*nb_handles)*sizeof(void*));
			//handles[(*nb_handles)-1]=handle;
			struct bpf_program fp;
			bpf_u_int32 mask;
			bpf_u_int32 net;
			if (pcap_lookupnet(i->name, &net, &mask, errbuf) == -1) {
				#ifdef DEBUG
				fprintf(stderr, "init_sniffer_pcap : Can't get netmask for device %s\n",i->name);
				#endif
				net = 0;
				mask = 0;
			}
			//compiling filter
			char* filter_exp = create_filter(protocols, nb_proto);
			if(pcap_compile(handle, &fp, filter_exp, 0, net)==-1){
				#ifdef DEBUG
				fprintf(stderr, "init_sniffer_pcap : Couldn't parse filter %s for device %s: %s. closing device\n", filter_exp, i->name, pcap_geterr(handle));
				#endif
				pcap_close(handle);
				i = i->next;
				continue;
			}
			if (pcap_setfilter(handle, &fp)==-1){
				#ifdef DEBUG
				fprintf(stderr, "init_sniffer_pcap : Couldn't install filter %s for device %s: %s. closing device\n", filter_exp, i->name, pcap_geterr(handle));
				#endif
				pcap_close(handle);
				i = i->next;
				continue;
			}
			int layer_size=get_datalink_layer_size(handle);
			if(layer_size==-1){
				#ifdef DEBUG
				fprintf(stderr, "init_sniffer_pcap : unknown kind of layer for device %s. closing device\n", i->name);
				#endif
				pcap_close(handle);
				i = i->next;
				continue;	
			}
			struct sniffer_thread_t* lthread = malloc(sizeof(struct sniffer_thread_t));
			memset(lthread, 0, sizeof(struct sniffer_thread_t));
			lthread->datalink_length = layer_size;
			lthread->pcap_hd=handle;
			#ifdef BSDSOCKET
			// Get the pcap file descriptor
			lthread->pcap_fd = pcap_get_selectable_fd(handle);
			if (lthread->pcap_fd == -1) {
				#ifdef DEBUG
				fprintf(stderr,"init_sniffer : Cannot get the pcap file descriptor\n");
				#endif
				free(lthread);
				i = i->next;
				continue;
			}
			#endif
			char* name = malloc((strlen(i->name)+1)*sizeof(char));
			strcpy(name,i->name);
			lthread->devname=name;
			/*if(callback!=NULL){*/
				lthread->callback = callback;
			/*}
			else{
				lthread->callback = default_callback;
			}*/
			lthread->params = params;//on met le packet en param?
			lthread->thread_running = 1;
			#ifdef DEBUG
			fprintf(stdout,"thread[%s]: start\n", lthread->devname);
			#endif
			pthread_create(&lthread->thread, NULL, (void*)&run_sniffer_thread, lthread);
			lthread->thread_created = 1;
			//char* iname="\0";
			//sprintf(iname,"thread %s",i->name);
			*handles=add_field(*handles,i->name,lthread);

		}
	        i = i->next;
	}
        pcap_freealldevs(ifs);
	return 1;
/*
	// Iterator
	int i;

	// Create the queue that will contains informations on each thread
	handle->threads = queue_init();

	// Prepare a thread for each device
	for (i = 0; i < set_length(devset); i++) {
		char* devname;
		int  len;
		set_get(devset, i, (void**)(&devname), &len);
		sniffer_thread_init(handle, devname, protocols, ipssrc);
		info("thread[%s]: prepared", devname);
	}

	// Close sets
	set_close(devset);

	return 1;
*/
}

void close_sniffer_thread ( sniffer_thread_s* lthread) {
	pthread_mutex_lock(&(lthread->lock));
	if (lthread->thread_running) {
		#ifdef DEBUG
		fprintf(stdout,"thread[%s]: stop asked\n", lthread->devname);
		#endif
		lthread->thread_running = 0;
	}
	pthread_mutex_unlock(&(lthread->lock));
	// ... then wait the stop...
	if (lthread->thread_created) {
		#ifdef DEBUG
		fprintf(stdout,"thread[%s]: join thread\n", lthread->devname);
		#endif
		pthread_join(lthread->thread, NULL);
		#ifdef DEBUG
		fprintf(stdout,"thread[%s]: stop effective\n", lthread->devname);
		#endif
	}
	if (lthread->devname != NULL) free(lthread->devname);
	if (lthread->pcap_hd != NULL) pcap_close(lthread->pcap_hd);
	#ifdef BSDSOCKET
	if (lthread->pcap_fd != 0)    close(lthread->pcap_fd);
	#elif WINSOCKET
	CloseHandle(lthread->w32event);
	#endif
	//TODO:!!!
	//if (lthread->lock != 0)       pthread_mutex_destroy(&lthread->lock);
}

int finish_sniffer(field_s** handles){
	if(handles==NULL||*handles==NULL){
		#ifdef DEBUG
		fprintf(stderr, "finish_sniffer : no handle to close\n");
		#endif
		return -1;
	}
	field_s* handles_tp = *handles;
	while(handles_tp!=NULL){
		close_sniffer_thread((sniffer_thread_s*)handles_tp->value);
		handles_tp = get_next_field(handles_tp);
	}
	free_all_fields(*handles);
/*
	// Iterator
	int i;

	// Destroy the threads structure
	struct sniffer_thread_t* thread;
	for (i = 0; i < queue_length(handle->threads); i++) {
		thread=(struct sniffer_thread_t*)queue_get(handle->threads, i);
		info("thread[%s]: destroyed", thread->devname);
		sniffer_thread_close(thread);
	}

	// Free the main structure
	queue_close(handle->threads);
*/
	return 1;
}


void* run_sniffer_thread(void* thread) {
        sniffer_thread_s* lthread = (sniffer_thread_s*)thread;

	#ifdef BSDSOCKET
        fd_set sfd;
	struct timeval tv;
	tv.tv_sec  = 0;
	tv.tv_usec = 5000;
	#endif

        pthread_mutex_lock(&(lthread->lock));
        while (lthread->thread_running) {
                pthread_mutex_unlock(&lthread->lock);
		#ifdef BSDSOCKET //ifndef WINSOCKET?
        	FD_ZERO(&sfd);
	        FD_SET(lthread->pcap_fd, &sfd);
                int res = select(lthread->pcap_fd + 1, &sfd, NULL, NULL, &tv);
                if (res < 0){
			#ifdef DEBUG
			fprintf(stderr,"run_sniffer_thread : pcap: select failed\n");
			#endif
		}
		else if (res == 0) {
		}//warn("pcap: select timeout");
		#elif WINSOCKET
		int res = WaitForSingleObject(lthread->w32event, 5000);
		if (res == WAIT_TIMEOUT) {
		}//warn("wpacap: event timeout");
		#endif
		else { // Packet received
			#ifdef DEBUG
			fprintf(stdout,"run_sniffer_thread :thread[%s]: packet received\n", lthread->devname);
			#endif
	                struct pcap_pkthdr header;
        	        u_int8_t* buff=(u_int8_t*)pcap_next(lthread->pcap_hd,&header);
	                int buflen = header.len;

        	        if (buff == NULL) buflen = 0;

                	if (buflen > 0) {
	                        snif_packet_s* packet;
        	                packet = malloc(sizeof(snif_packet_s));
                	        packet->data        = buff;
                        	packet->length      = buflen;
				packet->offset      = lthread->datalink_length;
        	                //packet->timestamp   = header.ts;
				//info("GAO");

                	        if (lthread->callback != NULL){
					lthread->callback(packet,lthread->params);
				}
	                }
			#ifdef DEBUG
			fprintf(stdout,"run_sniffer_thread :thread[%s]: packet processed\n", lthread->devname);
			#endif
		}
                pthread_mutex_lock(&(lthread->lock));
        }
        pthread_mutex_unlock(&(lthread->lock));

        return NULL;
}

#else
//raw socket

#endif



