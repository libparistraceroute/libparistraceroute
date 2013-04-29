#include <stdlib.h>
#include <stdio.h>
#include <sys/types.h>  // getaddrinfo etc.
#include <sys/socket.h>
#include <netdb.h>
#include <string.h> // memcpy


#ifndef ADDRESS_H
#define ADDRESS_H

/*typedef enum {
    TYPE_SOCKADDR_IN,
    TYPE_SOCKADDR_IN6,
} sockaddrtype_t;
*/
typedef union {
    struct sockaddr     sa;
    struct sockaddr_in  sin;
    struct sockaddr_in6 sin6;
} sockaddr_any;

typedef struct {
    char           * ipaddress;
    int              af;
    sockaddr_any     address;
  //  sockaddrtype_t   type;
} sockaddr_t;

//typedef union common_sockaddr sockaddr_any;
int address_get_by_name (const char * name, sockaddr_any * addr);

int address_set_host (const char * hostname, sockaddr_any * saddr);

char * address_to_string (const sockaddr_any * addr); 

char * address_resolv(const char * name);



#ifndef DEF_AF
#define DEF_AF      AF_INET
#endif

#endif 
