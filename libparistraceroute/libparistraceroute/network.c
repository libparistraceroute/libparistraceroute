#include "network.h"

#include<stdlib.h>
#include<stdio.h>
#include<netinet/ip_icmp.h>
#include<netinet/icmp6.h>
#include<netdb.h>
#include<sys/time.h>
#include<unistd.h>
#include<sys/eventfd.h>
#include<pthread.h>

//default_timeout in seconds
#define DEFAULT_TIMEOUT 30
//penser a l'option pour pas d'ajout de header >IP> option sockaddr


//eventfd to timeout reply the probes without reply
int eventfd_reply_probe = -1;
static pthread_mutex_t move_probe;
//proto_s* protocol_fct;

//connaitre un struct socket? -> network gere les socket? -> leur création


//default function to call when receiving a packet in the sniffer
/*
this function is a callback for when we receive a packet in the sniffer
in the rpesent case, it expects to find an ICMP time exceded reply encapsuling a ipv4-udp/tcp probe
ivp6 or tcp can be added
*/

/*  Just returns current time as double, with most possible precision...  */
//from traceroute by dmitri
double get_time (void) {
	struct timeval tv;
	double d;

	gettimeofday (&tv, NULL);

	d = ((double) tv.tv_usec) / 1000000. + (unsigned long) tv.tv_sec;

	return d;
}

void default_callback (snif_packet_s* packet, void* userparams){
	#ifdef DEBUG
	fprintf(stderr,"sniffer callback : callback launched\n");
	#endif
	int reply_kind=OTHER_RESP;
	double rec_time = get_time();
	//default callback
	//remove the encapsuling data to reach a possible ICMP then associate it with a probe
	if(userparams!=NULL){
		#ifdef DEBUG
		fprintf(stdout,"sniffer callback : warning : user-given parameters not used\n");
		#endif
	}
	char* data = (char*)(packet->data+packet->offset);
	int actual_offset = packet->offset;
	//check encapsuling protocol
	if(guess_datagram_protocol(data, packet->length-actual_offset, "ipv4")==1){//IPv4-> encapsuling our icmp  datagram
		char* response_ip = (char*) proto_get_field(data, "ipv4","source_addr");
		if(response_ip!=NULL){
			#ifdef DEBUG
			fprintf(stdout,"sniffer callback : responding ip is %s \n",response_ip);
			#endif
		}
		//ICMP : code 11 (1 byte),type 0 (1 byte), checksum (2 bytes), (4 bytes) empty, max 68 bites of the failed packet including IP head (from 20 to  60) and 8 underlying bytes
		actual_offset = actual_offset+proto_get_header_size(data, "ipv4");
		char* icmp = (char*)(packet->data + actual_offset);
		if(guess_datagram_protocol(icmp, packet->length-actual_offset, "icmp")!=1){
			free(response_ip);
			#ifdef DEBUG
			fprintf(stderr,"sniffer callback : the packet is not an ICMP response\n");
			#endif
			return;
		}
		struct icmphdr* icmph = (struct icmphdr*) icmp;
		//view_snif_packet(packet);
        switch (icmph->type) {
            case ICMP_TIME_EXCEEDED:
                if (icmph->code == ICMP_EXC_TTL)
			        reply_kind=TTL_RESP;
                break;
            case ICMP_UNREACH:
                if (icmph->code == ICMP_UNREACH_PORT)
                    reply_kind = END_RESP;
                break;
            default:
                response_ip = realloc(response_ip, 29*sizeof(char));
                snprintf(response_ip, 29,"icmp error type %d code %d",icmph->type, icmph->code);
#ifdef DEBUG
                fprintf(stderr,"sniffer callback :the packet is not an ICMP time exceeded for TTL response\n");
#endif
        }
        /*
		if(icmph->type!=ICMP_TIME_EXCEEDED||icmph->code!=ICMP_EXC_TTL){
			//free(response_ip);
			response_ip = realloc(response_ip, 29*sizeof(char));
			snprintf(response_ip, 29,"icmp error type %d code %d",icmph->type, icmph->code);
			#ifdef DEBUG
			fprintf(stderr,"sniffer callback :the packet is not an ICMP time exceeded for TTL response\n");
			#endif
		}
		else{
			reply_kind=TTL_RESP;
		}
        */
		actual_offset=actual_offset+proto_get_header_size(icmp, "icmp");
		char* real_packet = (char*)(packet->data + actual_offset) ; //TODO  does icmp contain it even if not ttl ex
		//parse the beginning of packet to associate to probe
		if(guess_datagram_protocol(real_packet, packet->length-actual_offset,"ipv4")==1){
			unsigned short checksums[2];//2 for ipv4 and encapsuled protocol
			#ifdef DEBUG
			fprintf(stdout,"sniffer callback : Ipv4 packet reply\n");
			#endif
			//get the protocol? 8 bytes of data left
			u_int8_t* prot = (u_int8_t*) proto_get_field(real_packet, "ipv4","next_proto");
			int proto = *prot;
			free(prot);
			//useless because TTL change implies checksum change in IP
			//u_int16_t* check = (u_int16_t*) get_field(real_packet, "ipv4","checksum");
			//checksums[0] = *((unsigned short*)check);
			//free(check);
			//#ifdef DEBUG
			//fprintf(stdout,"sniffer callback : checksum for ipv4 is %04x \n", checksums[0]);
			//#endif
			checksums[0]=0;
			struct protoent *ent = getprotobynumber(proto);
			char* protocol=ent->p_name;
			#ifdef DEBUG
			fprintf(stdout,"sniffer callback : Ipv4 probe encapsules %s \n",protocol);
			#endif
			char* under_proto = real_packet +proto_get_header_size(real_packet, "ipv4"); 
			unsigned short* udp_check =  proto_get_field(under_proto, protocol,"checksum");
			checksums[1] =ntohs(*(udp_check));
			free(udp_check);
			#ifdef DEBUG
			fprintf(stdout,"sniffer callback : checksum for %s is %04x \n",protocol, checksums[1]);
			#endif
			//match those checksums with probes'
			//need size of packet
			pthread_mutex_lock(&move_probe);
			probe_s* res = match_reply_with_probe((char*)packet->data,packet->offset, checksums, 2);
			pthread_mutex_unlock(&move_probe);
			if(res==NULL){
				#ifdef DEBUG
				fprintf(stderr,"sniffer callback : no probe found corresponding to received reply \n");
				#endif
				free(response_ip);
				return;
			}
			
			if(strcmp(response_ip,(char*) probe_get_data_value(res,"dest_addr"))==0){//responding a char* ??
				reply_kind=END_RESP;
			}
			//remove this probe from the eventfd reply -> decrease the number
			ssize_t s;
			uint64_t u;
			if(eventfd_reply_probe!=-1){
				s = read(eventfd_reply_probe, &u, sizeof(uint64_t));
				if (s != sizeof(uint64_t)){
					#ifdef DEBUG
					fprintf(stderr,"default_callback : error while trying to read eventfd for reply \n");
					#endif
				}
				else{
					#ifdef DEBUG
					fprintf(stderr,"default_callback : reading succeded\n");
					#endif
				}
			}
			else{
				#ifdef DEBUG
				fprintf(stderr,"default_callback : eventfd for reply is not active\n");
				#endif
			}
			probe_set_receiving_time( res, rec_time);
			//ip_response is malloc'ed
			probe_set_reply_addr( res, response_ip);
			free(response_ip);
			probe_set_reply_kind(res, reply_kind);
			//use the probe ?
			algo_s* algo = (algo_s*) res->algorithm;
			if(algo!=NULL && algo->algorithm != NULL){
				#ifdef DEBUG
				fprintf(stderr,"default_callback : finished, calling for on_reply\n");
				#endif
				algo->algorithm->on_reply(res);
				#ifdef DEBUG
				fprintf(stderr,"default_callback : on_reply done\n");
				#endif
				return;
			}
			else{
				#ifdef DEBUG
				fprintf(stderr,"sniffer callback : no algorithm found. stopping after reply treatment\n");
				#endif
			}
			return;
			
		}
		if(guess_datagram_protocol(real_packet, packet->length-actual_offset,"ipv6")==1){ //TODO : is that even possible ?
			#ifdef DEBUG
			fprintf(stdout,"sniffer callback : packet answers is ipv6. incredible.\n");
			#endif
			return;
		}
		else{
			#ifdef DEBUG
			fprintf(stderr,"sniffer callback : packet layer unrecognized\n");
			#endif
			return;
		}
		return;
		
		
	}
	if(guess_datagram_protocol(data,  packet->length-actual_offset, "ipv6")==1){//IPv6
		#ifdef DEBUG
		fprintf(stderr,"sniffer callback : response packet layer is IPv6\n");
		#endif
		//header_size = get_header_size(data, "ipv6"); //IPv6 header lenght
		//ICMP : code 3 (1 byte),type 0 (1 byte), checksum (2 bytes), (4 bytes) empty, max 1272 bites of the failed packet including IP head and prob the underlying
		//TODO
		return;
	}
	else{
		#ifdef DEBUG
		fprintf(stderr,"guess_encapsuling_protocol : unsupported or unknown encapsuling protocol\n");
		#endif
		return;
	}

}

int create_queue(){
	//create the eventfd and pass it to queue
	//TODO
	//create the socket
	int sock = init_raw_socket();
	if (sock < 0){
		perror ("create_socket");
		return -1;
	}
	return sock;
}

field_s* create_sniffer(char** protocols, int nb_proto,void (*callback) (snif_packet_s* packet, void* userparams),void* param){
	field_s** handles = malloc(sizeof(field_s*));
	void (*tmpcallback) (snif_packet_s* packet, void* userparams) = callback;
	if(tmpcallback==NULL){
		tmpcallback=&default_callback;
	}
	if(init_sniffer(handles,protocols, nb_proto,tmpcallback,param)==-1){
		#ifdef DEBUG
		fprintf(stderr,"create_sniffer : error  while initializing sniffer\n");
		#endif
		return NULL;
	}
	return *handles;
}

int close_sniffer(field_s** handles){
	return finish_sniffer(handles);
}

packet_s* create_fill_packet_from_probe(probe_s* probe, int payload_size){
	//trad information from probe to packet
	field_s* fields = probe_get_data(probe);
	fieldlist_s* fieldlist = create_fieldlist("info_probe",fields);
	#ifdef DEBUG
		fprintf(stdout,"create_fill_packet_from_probe : creating packet\n");
	#endif
	packet_s* pack = create_packet(payload_size, probe->protos, probe->nb_proto);
	#ifdef DEBUG
		fprintf(stdout,"create_fill_packet_from_probe : filling packet\n");
	#endif
	int fill_succ = fill_packet(pack, fieldlist,probe->checksums);
	if(fill_succ<0){
		#ifdef DEBUG
		fprintf(stderr,"create_fill_packet_from_probe : error while filling packet\n");
		#endif
		free_packet(pack);
		pack=NULL;
	}
	else{

	}
	//calc headers size if necessary
	remove_fieldlist(&fieldlist,"info_probe",0);
	//send packet to the network
	return pack;
}

int checksum_exchange_paristraceroute( packet_s* packet){
	int res = packet_exchange_checksum_and_payload(packet,1); //the second protocol : udp
	return res;
}

int send_packet(packet_s* packet, probe_s* probe, int socket){//TODO : socket supposed to be in packet, known at the probe level?
	if(socket<0){
		#ifdef DEBUG
		fprintf(stderr,"send_packet : bad socket\n");
		#endif
		return -1;
	}
	if(packet==NULL||probe==NULL){
		#ifdef DEBUG
		fprintf(stderr,"send_packet : empty parameters\n");
		#endif
		return -1;
	}
	field_s* fields = probe_get_data(probe);
	fieldlist_s* fieldlist = create_fieldlist("info_probe",fields);
	#ifdef DEBUG
	fprintf(stdout,"send_packet : creating sockaddr\n");
	#endif
	sockaddr_u* sockaddr = fill_sockaddr_from_fields(fieldlist);
	#ifdef DEBUG
	fprintf(stdout,"send_packet :  sockaddr created\n");
	#endif
	remove_fieldlist(&fieldlist,"info_probe",0);
	probe_set_sending_time( probe, get_time ());
	if(send_data(*packet->data, packet->size,socket, *sockaddr)<0){
		#ifdef DEBUG
		fprintf(stderr,"send_packet : error while trying to send packet\n");
		#endif
		free_packet(packet);
		return -1;
	}
	free_packet(packet);
	return 1;
}


int send_probe(probe_s* probe, int payload_size, int socket){
	packet_s* pack = create_fill_packet_from_probe(probe, payload_size);
	if(send_packet(pack, probe, socket)==-1){
		#ifdef DEBUG
		fprintf(stderr,"send_probe : error while trying to send probe\n");
		#endif
		return -1;
	}
	//try to write in semaphore if not = -1
	if(eventfd_reply_probe!=-1){
		char* one = "1\0";
		uint64_t u = strtoull(one, NULL, 0);
		/* strtoull() allows various bases */
		ssize_t s = write(eventfd_reply_probe, &u, sizeof(uint64_t));
		if (s != sizeof(uint64_t)){
			#ifdef DEBUG
			fprintf(stderr,"send_probe : writing in probe reply eventfd failed\n");
			#endif
               }
		else{
			#ifdef DEBUG
			fprintf(stderr,"send_probe : writing %s succeded\n",one);
			#endif
		}

	}
	else{
		#ifdef DEBUG
		fprintf(stdout,"send_probe : warning : not using the probe reply eventfd\n");
		#endif
	}
	return 1;
}

int init_reply_probe_eventf(){
	eventfd_reply_probe = eventfd(0, EFD_SEMAPHORE);
	if(eventfd_reply_probe ==-1){
			#ifdef DEBUG
			fprintf(stderr,"execution_algorithms : creation of eventfd for reply failed\n");
			#endif
			return -1;
	}
	return 1;
}

int execution_algorithms(field_s* data){
	init_reply_probe_eventf();
	field_s* list = get_algo_enum();
	while(list!=NULL){
		algo_s* algorithm = (algo_s*) get_field_value(list, get_field_name(list));
		if(algorithm==NULL){
			#ifdef DEBUG
			fprintf(stderr,"execution_algorithms : recuperation of algorithm %s failed\n",get_field_name(list));
			#endif
			list=get_next_field(list);
			continue;
			
		}
		algorithm->algorithm->init(algorithm,data);
		//mutex for old probes
		list=get_next_field(list);
	}
	int timeout=DEFAULT_TIMEOUT;
	int* rec_timeout = get_field_value(data, "timeout");
	if(rec_timeout!=NULL){
		timeout = *rec_timeout;
	}
	//eventfd to treat the probes that arrive in the waiting list
	if(eventfd_reply_probe !=-1){
		ssize_t s;
		uint64_t u;
		int true_wait;
		while(1){
			//now checking probes
			//get the older probe -> at the end ?
			probe_s* probe = get_older_probe();
			if(probe==NULL){
				continue;
			}

			true_wait = timeout-((int) (get_time() - probe_get_sending_time(probe)));
			if(true_wait>0){
				sleep(true_wait);
			}
			pthread_mutex_lock(&move_probe);
			//if(get_reply_addr(probe)==NULL){
			if(!is_probe_complete(probe)){
				remove_probe_from_probes(probe);
				add_probe_to_complete_probes(probe);
				s = read(eventfd_reply_probe, &u, sizeof(uint64_t));
				if (s != sizeof(uint64_t)){
					#ifdef DEBUG
					fprintf(stderr,"execution_algorithms : error while trying to read eventfd for reply\n");
					#endif
				}
				char* res = "*\0";
				probe_set_reply_addr( probe, res);
				probe_set_reply_kind(probe, NO_RESP);
				probe_set_receiving_time(probe, probe_get_sending_time(probe));
				//made by "match_reply_with_probe" in callback
				algo_s* algo = (algo_s*) probe->algorithm;
				#ifdef DEBUG
				fprintf(stderr,"execution_algo : probe unresponded, calling for on_reply\n");
				#endif
				if(algo!=NULL && algo->algorithm != NULL){
					algo->algorithm->on_reply(probe);
					#ifdef DEBUG
					fprintf(stderr,"execution_algo : on_reply done\n");
					#endif
				}
				else{
					#ifdef DEBUG
					fprintf(stderr,"execution_algo : no algorithm found. stopping after reply treatment\n");
					#endif
				}
			}
			else{//take the next and do the same, using the same read, so readd the used token
				//u = strtoull("1", NULL, 0);
				//s = write(eventfd_reply_probe, &u, sizeof(uint64_t));
			}
			pthread_mutex_unlock(&move_probe);	
		}
	}
	else{
		#ifdef DEBUG
		fprintf(stderr,"execution_algorithms : error : eventfd for reply is not active\n");
		#endif		
	}
	return 1;
}

void finish_network(int socket,field_s* handles){
	//verify that all treatment are over?
	if(close_socket(socket)==-1){
		#ifdef DEBUG
		fprintf(stderr,"finish_network : error while trying to close sending socket\n");
		#endif
	}
	if(close_sniffer(&handles)==-1){
		#ifdef DEBUG
		fprintf(stderr,"finish_network : error while trying to free sniffing socket\n");
		#endif
	}
	free_all_probes();
}
