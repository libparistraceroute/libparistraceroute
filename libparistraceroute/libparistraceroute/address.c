#include <stdlib.h>
#include <stdio.h>
#include <sys/types.h>  // getaddrinfo etc.
#include <sys/socket.h>
#include <netdb.h>
#include <string.h> // memcpy
#include <arpa/inet.h>

#include "address.h"


#define AI_IDN 0x0040

/******************************************************************************
 * Static variables 
 ******************************************************************************/

static int    af = 0; // ? address family
//static char * dst_name = NULL;

/******************************************************************************
 * Helper functions
 ******************************************************************************/

// CPPFLAGS += -D_GNU_SOURCE

int address_get_by_name(const char * name, sockaddr_any * addr) {
    int    ret;
    struct addrinfo hints, *ai, *res = NULL;

    memset (&hints, 0, sizeof (hints));
    hints.ai_family = af;
    hints.ai_flags = AI_IDN;

    ret = getaddrinfo(name, NULL, &hints, &res);
    if (ret) {
        fprintf(stderr, "%s: %s\n", name, gai_strerror (ret));
        return -1;
    }

    for (ai = res; ai; ai = ai->ai_next) {
        if (ai->ai_family == af)  break;
        /*  when af not specified, choose DEF_AF if present   */
        if (!af && ai->ai_family == DEF_AF)
            break;
    }
    if (!ai)  ai = res; /*  anything...  */

    if (ai->ai_addrlen > sizeof (*addr))
        return -1;  /*  paranoia   */
    memcpy (addr, ai->ai_addr, ai->ai_addrlen);

    freeaddrinfo (res);

    return 0;
}

int address_set_host (const char * hostname, sockaddr_any * dst_addr)
{

    if (address_get_by_name (hostname, dst_addr) < 0)
        return -1;

  //  dst_name = hostname;

    /*  i.e., guess it by the addr in cmdline...  */
    if (!af)  af = dst_addr->sa.sa_family;

    return 0;
}

static char addr_to_string_buf[INET6_ADDRSTRLEN];

char * address_to_string(const sockaddr_any * addr) 
{
    getnameinfo (&addr->sa, sizeof (*addr), addr_to_string_buf, sizeof (addr_to_string_buf), 0, 0, NI_NUMERICHOST);
    return addr_to_string_buf;
}

char * address_resolv(const char * name) {
    sockaddr_any      res;
    struct hostent  * hp;
    int               af_type;
    
    if(strchr(name, '.') != NULL) {
        af_type = AF_INET;
    }
    else {
        af_type = AF_INET6;
    }
    if (!inet_pton(af_type, name, &res)) {
        printf("can't parse IP address %s", name);
    }
    hp = gethostbyaddr((const void *)&res, sizeof(res), af_type);
    //printf("ip: %s -> name : %s \n ", name, hp->h_name ? hp);
    return hp ? hp->h_name : " ";
}
