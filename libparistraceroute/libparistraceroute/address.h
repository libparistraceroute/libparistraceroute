#include <stdlib.h>
#include <stdio.h>
#include <sys/types.h>  // getaddrinfo etc.
#include <sys/socket.h>
#include <netdb.h>
#include <string.h> // memcpy


#ifndef ADDRESS_H
#define ADDRESS_H

typedef enum {
        /* Address family AF_INET */
    TYPE_SOCKADDR_IN,
        /* Address family AF_INET6 */
    TYPE_SOCKADDR_IN6,
} sockaddrtype_t;

typedef union {
    struct sockaddr     sa;
    struct sockaddr_in  sin;
    struct sockaddr_in6 sin6;
} sockaddr_any;

typedef struct {
    char           * ipaddr;
    int              af;
    sockaddr_any     addr;
    sockaddrtype_t   type;
    
}sockaddr_t;


//typedef union common_sockaddr sockaddr_any;
int address_get_by_name (const char * name, sockaddr_any * addr);

int address_set_host (char * hostname, sockaddr_any * dst_addr);


char * address_resolv(char * name);



#ifndef DEF_AF
#define DEF_AF      AF_INET
#endif

#endif 
