#ifndef PARIS_TRACEROUTE_H
#define PARIS_TRACEROUTE_H

union common_sockaddr {
    struct sockaddr sa;
    struct sockaddr_in sin;
    struct sockaddr_in6 sin6;
};
typedef union common_sockaddr sockaddr_any;

#ifndef DEF_AF
#define DEF_AF      AF_INET
#endif

#endif // PARIS_TRACEROUTE_H
