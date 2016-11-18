#include "use.h"

#include "config.h"
#include "whois.h"

#include <errno.h>      // errno
#include <stdio.h>      // fprintf
#include <stdlib.h>     // realloc
#include <string.h>     // memset, memcpy, strlen
#include <sys/types.h>  // socket, recv
#include <sys/socket.h> // socket, recv
#include <unistd.h>     // close

#ifdef USE_CACHE
#    include "containers/map.h"

static map_t * cache_ip_asn = NULL;

static void __cache_ip_asn_create() __attribute__((constructor));
static void __cache_ip_asn_free()   __attribute__((destructor));

static void str_dump(const char * s) {
    printf("%s (%p)", s, s);
}

static void __cache_ip_asn_create() {
    cache_ip_asn = map_create(
        address_dup, address_free, address_dump, address_compare,
        strdup,      free,         str_dump
    );
}

static void __cache_ip_asn_free() {
    if (cache_ip_asn) map_free(cache_ip_asn);
}

#endif

bool whois_callback_print(void * pdata, const char * line) {
    FILE * out = (FILE *) pdata;
    fprintf(out, "%s\n", line);
    return true;
}

static bool whois_callback_find_server(void * pdata, const char * line) {
    char * whois_server = (char *) pdata;
    char * whois_fqdn = strstr(line, "whois.");

    if (whois_fqdn) strcpy(whois_server, whois_fqdn);

    // FIXME: shouldn't we stop iff the line starts with "refer:" or "whois:" ?
    return !whois_fqdn;
}

static bool whois_callback_get_asn(void * pdata, const char * line) {
    bool ret = true;
    uint32_t * asn = pdata;

    // If the line starts with "origin:"
    if (strstr(line, "origin:") == line) {
        const char * asn_begin = strstr(line + 7, "AS");
        if (asn_begin) {
            sscanf(asn_begin + 2, "%u", asn);
            ret = false;
        }
    }

    return ret;
}

bool whois_query(
    const address_t * server_address,
    const address_t * queried_address,
    bool (*callback)(void *, const char *),
    void            * pdata
) {
    char * query;
    const  size_t BUFFER_SIZE = 1000;
    char   buffer[BUFFER_SIZE];
    int    sockfd, read_size; //, total_size = 0;

    struct sockaddr     * sa;
#ifdef USE_IPV4
    struct sockaddr_in    sa4;
#endif
#ifdef USE_IPV6
    struct sockaddr_in6   sa6;
#endif
    size_t socket_len;

    int    family = server_address->family;
    size_t len = 0;

    buffer[BUFFER_SIZE] = '\0';

    if (queried_address->family != AF_INET && queried_address->family != AF_INET6) {
        fprintf(stderr, "whois_query: queried address family not supported (family = %d)\n", queried_address->family);
        goto ERR_INVALID_FAMILY;
    }

    if (family != AF_INET && family != AF_INET6) {
        fprintf(stderr, "whois_query: server address family not supported (family = %d)\n", server_address->family);
        goto ERR_INVALID_FAMILY;
    }

    if ((sockfd = socket(family, SOCK_STREAM, IPPROTO_TCP)) == -1) {
        goto ERR_SOCKET;
    }

    // Prepare connection structures.
    switch (family) {
#ifdef USE_IPV4
        case AF_INET:
            sa = (struct sockaddr *) &sa4;
            socket_len = sizeof(struct sockaddr_in);
            memset(sa, 0, socket_len);
            sa4.sin_family = family;
            sa4.sin_port   = htons(43);
            memcpy(&sa4.sin_addr.s_addr, &(server_address->ip.ipv4), sizeof(ipv4_t));
            break;
#endif
#ifdef USE_IPV6
        case AF_INET6:
            sa = (struct sockaddr *) &sa6;
            socket_len = sizeof(struct sockaddr_in6);
            memset(sa, 0, socket_len);
            sa6.sin6_family = family;
            sa6.sin6_port   = htons(43);
            memcpy(&sa6.sin6_addr, &server_address->ip.ipv6, sizeof(ipv6_t));
            break;
#endif
        default:
            goto ERR_INVALID_FAMILY;
    }

    // Connect to whois remote server.
    if (connect(sockfd, sa, socket_len) < 0) {
        fprintf(stderr, "whois_query: %s\n", strerror(errno));
        goto ERR_CONNECT;
    }

    // Send the whois query: append "\r\n\0" to the queried address.
    // TODO improve address_to_string design to take a preallocated buffer
    // in parameter. Therefore we could avoid this realloc.
    address_to_string(queried_address, &query);
    len = strlen(query);
    if (!(query = realloc(query, len + 3))) {
        goto ERR_REALLOC1;
    }

    strncpy(query + len, "\r\n\0", 3);

    if (send(sockfd, query, len + 3, 0) < 0) {
        goto ERR_SEND;
    }

    // Handle the response.
    size_t offset = 0;
    while ((read_size = recv(sockfd, buffer + offset, BUFFER_SIZE, 0))) {
        // Find (line_begin, line_end) the pair of pointer delimiting the next full line.
        char * line_begin = buffer,
             * line_end   = buffer,
             * buffer_end = buffer + read_size;
        while (line_end < buffer_end) {
            for (line_end = line_begin; *line_end != '\n' && line_end < buffer_end; line_end++);

            if (line_end < buffer_end) {
                // We've found a '\n' so this is a full line.
                // Replace it by '\0' and call the callback.
                *line_end = '\0';
                if (!callback(pdata, line_begin)) break;
            } else {
                // Truncated line, copy at the beginning of the buffer.
                // This line will be completed during the next recv() iteration.
                offset = buffer_end - line_begin;
                memcpy(buffer, line_begin, offset);
                break;
            }

            // Move the 'line_begin' to next new line.
            line_begin = line_end + 1;
        }
    }

    close(sockfd);
    return true;

ERR_SEND:
ERR_REALLOC1:
ERR_CONNECT:
    close(sockfd);
ERR_SOCKET:
ERR_INVALID_FAMILY:
    return false;
}

bool whois_find_server(const address_t * queried_address, char * whois_server) {
    address_t iana_address;

    if (address_from_string(queried_address->family, "whois.iana.org", &iana_address)) {
        goto ERR_ADDRESS_FROM_STRING;
    }

    whois_server[0] = '\0';
    if (!whois_query(&iana_address, queried_address, whois_callback_find_server, whois_server)) {
        goto ERR_WHOIS_QUERY;
    }

    return true;

ERR_WHOIS_QUERY:
ERR_ADDRESS_FROM_STRING:
    return false;
}

bool whois(
    const address_t * queried_address,
    bool (*callback)(void *, const char *),
    void            * pdata
) {
    char      whois_server[50];
    address_t server_address;

    if (!whois_find_server(queried_address, (char *) whois_server)) {
        goto ERR_WHOIS_FIND_SERVER;
    }

    if (address_from_string(queried_address->family, whois_server, &server_address)) {
        goto ERR_ADDRESS_FROM_STRING;
    }

    return whois_query(&server_address, queried_address, callback, pdata);

ERR_ADDRESS_FROM_STRING:
ERR_WHOIS_FIND_SERVER:
    return false;
}

bool whois_get_asn(
    const address_t * queried_address,
    uint32_t        * asn,
    int               mask_cache
) {
    bool ret = true;

	// FIXME
/*

	// valgrind error in map

	address_get_size: Invalid family==6139== Invalid read of size 1
	==6139==    at 0x4E42696: address_compare (address.c:183)
	==6139==    by 0x514414F: tfind (in /lib/x86_64-linux-gnu/libc-2.22.so)
	==6139==    by 0x4E47D39: set_find (set.c:70)
	==6139==    by 0x4E477A8: map_find_impl (map.c:145)
	==6139==    by 0x4E521BB: whois_get_asn (whois.c:249)
	==6139==    by 0x400917: main (traceroute.c:99)
	==6139==  Address 0xffffffffffffffff is not stack'd, malloc'd or (recently) free'd
*/

/*
#ifdef USE_CACHE
    if (cache_ip_asn && (mask_cache & CACHE_READ)) {

        address_dump(queried_address);
		printf("\n");

        ret = map_find(cache_ip_asn, queried_address, asn);

		if (ret) printf("hit cache!");
        address_dump(queried_address);
		printf("\n");
    }

    if (!ret) {
#endif
*/
        whois(queried_address, whois_callback_get_asn, asn);
		ret = (*asn != 0);
/*
#ifdef USE_CACHE
        if (mask_cache & CACHE_WRITE) {
            map_update(cache_ip_asn, queried_address, asn_dup);
        }
    }
#endif
*/
    return ret;
}

