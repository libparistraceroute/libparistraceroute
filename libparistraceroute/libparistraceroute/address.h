#include <stdlib.h>
#include <stdio.h>
#include <sys/types.h>  // getaddrinfo etc.
#include <sys/socket.h>
#include <netdb.h>
#include <string.h> // memcpy


#ifndef ADDRESS_H
#define ADDRESS_H

union common_sockaddr {
    struct sockaddr     sa;
    struct sockaddr_in  sin;
    struct sockaddr_in6 sin6;
};
typedef union common_sockaddr sockaddr_any;

int address_get_by_name (const char * name, sockaddr_any * addr);

int address_set_host (char * hostname, sockaddr_any * dst_addr);

const char * address_2_str (const sockaddr_any *addr); 

char * address_resolv(char * name);



#ifndef DEF_AF
#define DEF_AF      AF_INET
#endif

#endif 
