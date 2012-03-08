#ifndef H_PATR_QUEUE
#define H_PATR_QUEUE

#include <netinet/in.h>
#include "network.h" //pour connaitre struct addr?
/**struct from traceroute for addresses*/

/**
 * set the max flow rate to send (for example per second>?)
 * @param bit_per_second : the number of bits send in each "send()" call, no limit if 0
 * @return void
 */

void set_flow_rate(int bit_per_send);

/**
 * send data throught the network
 * @param data : the packet itself to send
 * @param data_size : the size of data to send
 * @param socket : the socket in which send the data
 * @param addr : the adress to which send the data
 * @return void
 */

int send_data(char* data, int data_size, int socket, sockaddr_u addr);

/**
 * create a sockaddr depending on the specified address and port
 * @param data : the fieldlist
 * @return the sockaddr filled with data
 */

sockaddr_u* fill_sockaddr_from_fields(fieldlist_s* fields);

/**
 * create a raw socket for sending all kind of data
 * @return the socket
 */
int init_raw_socket();

/**
 * close a socket
 * @param socket : the socket to close
 * @return 1 if success, -1 otherwise
 */
int close_socket(int socket);
#endif
