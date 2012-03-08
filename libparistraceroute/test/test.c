#include<stdlib.h>
#include<stdio.h>
#include<netinet/ip.h>
#include <unistd.h>


//#include"packet.h"
//#include "field.h"
//#include "proto.h"
//#include "fieldlist.h"
//#include "probe.h"
#include "network.h"
#include"algo.h"
#include <arpa/inet.h>
#include <netinet/ip6.h>


int test_field(){
	int* zero = malloc(sizeof(int));
	*zero = 0;
	field_s* new = create_field("salut", zero);
	char** essai = malloc(5*sizeof(char));
	*essai="test\0";
	new = add_field(new,"creation", essai);
	fprintf(stdout,"here we called new on new so if prob with add and create, must appear\n");
	view_fields(new);
	field_s* res = new;
	char** deux_essai = malloc(10*sizeof(char));
	*deux_essai="aaaaaaaaa\0";
	set_field_value(res,"salut", deux_essai,0);
	fprintf(stdout,"\n\nmaintenant on doit avoir aaaa dans salut\n");
	view_fields(res);
	fprintf(stdout,"\n\nsur champ qui n'existe pas\n");
	char** deux = malloc(2*sizeof(char));
	*deux="b\0";
	set_field_value(res,"patate", deux,0);
	view_fields(res);
	char* tab1[]= {"salut","creation"};
	fprintf(stdout,"\n\npresence champs existants\n");
	check_presence_list(tab1,2,res);
	remove_field(&res,"salut",1);
	char* tab2[]= {"salut","creation"};
	fprintf(stdout,"\n\npresence 1 champs existants 1 non : suppression de 'salut'\n");
	check_presence_list(tab2,2,res);

	char* tab3[]= {"sal","patate"};
	fprintf(stdout,"\n\n absence 2 champs\n");
	check_presence_list(tab3,2,res);

	free_all_fields(res,1);
	return 0;
}

int test_fieldlist(){
	int* zero = malloc(sizeof(int));
	*zero = 0;
	field_s* new = create_field("salut", zero);
	char** essai = malloc(5*sizeof(char));
	*essai="test\0";
	field_s* res = add_field(new,"creation", essai);
	view_fields(res);
	char** deux_essai = malloc(10*sizeof(char));
	*deux_essai="aaaaaaaaa\0";
	set_field_value(res,"salut", deux_essai,0);
	fprintf(stdout,"\n\nmaintenant on doit avoir aaaa dans salut\n");
	fprintf(stdout,"\n\nfields res : \n");
	view_fields(res);

	int* zerot = malloc(sizeof(int));
	*zerot = 0;
	field_s* new2 = create_field("salut2", zerot);
	char** essait = malloc(5*sizeof(char));
	*essait="test\0";
	field_s* res2 = add_field(new2,"creation", essait);
	view_fields(res2);
	char** deux_essait = malloc(10*sizeof(char));
	*deux_essait="aaaaaaaaa\0";
	set_field_value(res2,"salut2", deux_essait,0);
	fprintf(stdout,"\n\nmaintenant on doit avoir aaaa dans salut\n");
	fprintf(stdout,"\n\nfields res2 : \n");
	view_fields(res2);

	fprintf(stdout,"on affiche la 1 avant create\n");

	view_fields(res);

	fieldlist_s* liste = create_fieldlist("ligne1",res);

	fprintf(stdout,"on affiche la 1\n");

	view_fieldlist(liste);

	fieldlist_s* liste2 = add_fieldlist(liste, "ligne2",res2);

	fprintf(stdout,"on affiche le tout\n");

	view_fieldlist(liste2);
	
	fprintf(stdout,"on imprime ligne1 apres l'avoir recup\n");
	field_s* ligne1 = get_fieldlist_value(liste, "ligne1");
	view_fields(ligne1);

	fprintf(stdout,"on recupere le contenu de salut\n");
	int* new_zero = get_field_value_from_fieldlist(liste2, "salut");
	fprintf(stdout,"on trouve : %d pour salut\n",*new_zero);
	
	fprintf(stdout,"on verifie la presence de ligne2\n");
	int res_int = check_presence_fieldlist("ligne2",liste2);
	fprintf(stdout,"on doit trouver 1 si oui  ; on trouve %d \n",res_int);
	fprintf(stdout,"on remove la liste ligne2 de fieldlist\n");
	remove_fieldlist(&liste2, "ligne2",1);
	fprintf(stdout,"on verifie la presence de ligne2 maintenant que suppr\n");
	res_int = check_presence_fieldlist("ligne2",liste2);
	fprintf(stdout,"on doit trouver -1 si non  ; on trouve %d \n",res_int);
	free_all_fieldlists(liste2,1);

	return 0;
}


int test_proto(char* proto){
	view_available_protocols();
	const proto_s* fct = choose_protocol(proto); //get the functions to test
	proto_view_fields(proto);
	if(fct == NULL){
		fprintf(stderr, "association for %s with functions failed. end\n", proto);
		return -1;
	}
	fprintf(stdout,"for proto %s : \n",fct->name);
	char* testchar = malloc(200);
	fct->fill_packet_header(NULL,&testchar);
	return 0;
}

typedef struct iphdr iphdr_s;

int test_bitfield(){
		iphdr_s * ip_hed= malloc(sizeof(iphdr_s));
		ip_hed->ihl = 12;
		unsigned int ip_hl; //:4
		ip_hl = ip_hed->ihl;
		fprintf(stdout,"ip_hl vaut %d\n",ip_hl);
	return 0;
}
int test_packet(){
	fprintf(stdout,"\n\n with no proto, negative size :\n");
	packet_s* pack = create_packet(-1,NULL,0);
	fprintf(stdout,"filling the packet\n");
	fill_packet(pack,NULL,NULL);
	fprintf(stdout,"\n\n with 2 protos, one is wrong :\n");
	char* protosf[2] = {"Ipv4","ud"};
	packet_s* bad_pack = create_packet(10,protosf,2);
	fprintf(stdout,"filling the packet\n");
	fill_packet(bad_pack,NULL,NULL);

	fprintf(stdout,"\n\n with 2 protos correct : ipv4 udp\n");
	char* protos[2] = {"Ipv4","udp"};
	packet_s* good_pack = create_packet(10,protos,2);
	fprintf(stdout,"filling the packet\n");
	fill_packet(good_pack,NULL,NULL);

	fprintf(stdout,"\n\n with 2 protos correct : ipv4 tcp\n");
	char* protost[2] = {"Ipv4","tcp"};
	packet_s* good_packt = create_packet(10,protost,2);
	fprintf(stdout,"filling the packet\n");
	fill_packet(good_packt,NULL,NULL);
	return 0;
}

int test_probe(){
	char* protosf[2] = {"Ipv4","udp"};
	fprintf(stdout,"creating the probe\n");
	probe_s* probe = create_probe(create_name_probe(),protosf,2,NULL,NULL);
	field_s* fields = probe_get_data(probe);
	view_fields(fields);
	return 0;
}

int test_probe_tcp(){
	char* protosf[2] = {"Ipv4","tcp"};
	fprintf(stdout,"creating the probe\n");
	probe_s* probe = create_probe(create_name_probe(),protosf,2,NULL,NULL);
	field_s* fields = probe_get_data(probe);
	view_fields(fields);
	return 0;
}

int test_network(){
	//int timeout = 2;
	init_reply_probe_eventf();
	fprintf(stdout,"starting the sniffer\n");
	char* protos_filter[1] = {"icmp"};
	//char* protos_filter[0] = {};
	field_s* handles = create_sniffer(protos_filter, 1, NULL,NULL );
	char* protosf[2] = {"Ipv4","udp"};
	fprintf(stdout,"creating the probe\n");
	//probe_s* probe = create_probe(create_name_probe(), 3, 34, 30000, 30000, "209.85.146.103", "0.0.0.0",protosf,2,NULL,NULL);
	probe_s* probe = create_probe(create_name_probe(),protosf,2,NULL,NULL);
	//probe_s* probe2 = create_probe(create_name_probe(), 2, 34, 30000, 30000, "209.85.146.103", "0.0.0.0",protosf,2,NULL,NULL);
	probe_s* probe2 = create_probe(create_name_probe(),protosf,2,NULL,NULL);
	//probe_s* probe = create_probe(1,34,10000, 10000, "127.0.0.1", "132.227.84.223",protosf,2);

	//view_packet(pack);
	fprintf(stdout,"creating the socket\n");
	int sock = create_queue();
	/*fprintf(stdout,"creating the packet associated\n");
	packet_s* pack = create_fill_packet_from_probe(probe,20);
	packet_s* pack2 = create_fill_packet_from_probe(probe2,0);
	fprintf(stdout,"sending the packet\n");
	send_packet(pack, probe, sock);
	send_packet(pack2, probe2, sock);*/
	fprintf(stdout,"creating and sending the probes\n");
	send_probe(probe, 2,sock);
	send_probe(probe2, 2,sock);
	sleep(4);
	//close_socket(sock);
	//close_sniffer(handles);
	finish_network(sock,handles);
	return 0;
}

int test_sniffer(){
	return 0;
}


int test_algo_all(){
	//create the data for simplehop

	short* zero = malloc(sizeof(short));
	*zero = 32500;
	field_s* data = create_field("dest_port", zero);
	char* addr = malloc(16*sizeof(char));
	addr="1.1.1.2\0";
	data = add_field(data,"dest_addr", addr);
	char* ttl = malloc(sizeof(char));
	*ttl=1;
	data = add_field(data,"TTL", ttl);


	//test
	// not needed, done  by the exectution algorithm  : init_reply_probe_eventf();
	fprintf(stdout,"starting the sniffer\n");
	char* protos_filter[1] = {"icmp"};
	field_s* handles = create_sniffer(protos_filter, 1, NULL,NULL );
	fprintf(stdout,"creating the socket\n");
	int sock = create_queue();
	char* protosf[2] = {"Ipv4","udp"};
	//add an algo to the algo list
	fprintf(stdout,"adding a simplehop algo, a mda algo, a traceroute algo\n");
	//int add_algo(char* algo_name, char* output_kind, int probes_per_TTL, char** protocols, int nb_proto,int sending_socket);
	//add_algo("simplehop", "simple", 3, protosf, 2,sock);
	add_algo("mda", "simple", 3, protosf, 2,sock);
	//add_algo("traceroute", "simple", 3, protosf, 2,sock);
	execution_algorithms(data);

	sleep(2);
	finish_network(sock,handles);
	return 1;
}

int test_algo_tcp(){
	//create the data for simplehop

	short* zero = malloc(sizeof(short));
	*zero = 32500;
	field_s* data = create_field("dest_port", zero);
	char* addr = malloc(16*sizeof(char));
	addr="1.1.1.2\0";
	data = add_field(data,"dest_addr", addr);
	char* ttl = malloc(sizeof(char));
	*ttl=1;
	data = add_field(data,"TTL", ttl);


	//test
	// not needed, done  by the exectution algorithm  : init_reply_probe_eventf();
	fprintf(stdout,"starting the sniffer\n");
	char* protos_filter[1] = {"icmp"};
	field_s* handles = create_sniffer(protos_filter, 1, NULL,NULL );
	fprintf(stdout,"creating the socket\n");
	int sock = create_queue();
	char* protosf[2] = {"Ipv4","tcp"};
	//add an algo to the algo list
	fprintf(stdout,"adding a simplehop algo, a mda algo, a traceroute algo\n");
	//int add_algo(char* algo_name, char* output_kind, int probes_per_TTL, char** protocols, int nb_proto,int sending_socket);
	//add_algo("simplehop", "simple", 3, protosf, 2,sock);
	add_algo("mda", "simple", 3, protosf, 2,sock);
	//add_algo("traceroute", "simple", 3, protosf, 2,sock);
	execution_algorithms(data);

	sleep(2);
	finish_network(sock,handles);
	return 1;
}

int test_algo(char* algo){
	
	return 1;
}

int tests(){
	struct in6_addr ip6_src;
	//inet_pton(AF_INET6,"::/128",&ip6_src);
	inet_pton(AF_INET6,"0:0:0:0:0:0:0:0",&ip6_src);
	
	char* res=malloc(40*sizeof(char));
	memset(res,0,40);
	inet_ntop(AF_INET6,&ip6_src,res,40);
	fprintf(stdout,"%s \n", res);

	struct in6_addr ip_check;
	inet_pton(AF_INET6,"0:0:0:0:0:0:0:0",&ip_check);
	char check[40];//xxxx:xxxx:xxxx:xxxx:xxxx:xxxx:xxxx:xxxx\0
	char src[40];
	memset(check,0,40);
	memset(src,0,40);
	inet_ntop(AF_INET6,&ip6_src,src,40);
	inet_ntop(AF_INET6,&ip_check,check,40); 
	if(strcmp(src,check)==0){
		fprintf(stdout,"ok\n");
	}
	return 1;
}

int main(int argc, char** argv){
	//test_field();
	//test_packet();
	//test_fieldlist();
	//test_bitfield();
	//test_proto("IPv4");
	//test_proto("Icmp");
	//test_proto("udp");
	//test_proto("ud");
	//test_network();
	test_algo_all();
	//test_algo_tcp();
	tests();
	return 0;
}
