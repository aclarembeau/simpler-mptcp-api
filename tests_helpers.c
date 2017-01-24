#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <stdlib.h>

#include "tests_helpers.h"

struct simpleaddr simpleaddr_make(struct sockaddr *addr, socklen_t addrlen) {
    struct simpleaddr ret = {addr, addrlen};
    return ret;
}

struct simpleaddr simpleaddr_resolve(const char *host, uint16_t port, int family_hint) {
    struct simpleaddr ret = {NULL, 0};

    struct addrinfo hints;
    struct addrinfo *result;

    memset(&hints, 0, sizeof(struct addrinfo));
    hints.ai_family = family_hint;    /* Allow IPv4 or IPv6 */
    hints.ai_socktype = SOCK_STREAM; /* Datagram socket */
    hints.ai_flags = AI_PASSIVE;    /* For wildcard IP address */
    hints.ai_protocol = 0;          /* Any protocol */
    hints.ai_canonname = NULL;
    hints.ai_addr = NULL;
    hints.ai_next = NULL;

    int s = getaddrinfo(host, NULL, &hints, &result);
    if (s == 0) {
        // Copy the address
        ret.addr = malloc(result->ai_addrlen);
        memcpy(ret.addr, result->ai_addr, result->ai_addrlen);
        ret.addrlen = result->ai_addrlen;

        // Don't forget to fill in the port
        if (result->ai_family == AF_INET) {
            struct sockaddr_in *in = (struct sockaddr_in *) ret.addr;
            in->sin_port = htons(port);
        } else {
            struct sockaddr_in6 *in = (struct sockaddr_in6 *) ret.addr;
            in->sin6_port = htons(port);
        }

        freeaddrinfo(result);
    } else {
        printf("%s:%d Getaddrinfo failed: %s\n", host, port, gai_strerror(s));
    }

    return ret;
}

void simpleaddr_free(struct simpleaddr *myaddr) {
    if (myaddr != NULL)
        free(myaddr->addr);
}

int simpleaddr_compare(struct simpleaddr a, struct simpleaddr b) {
    if (a.addr->sa_family == b.addr->sa_family) {
        // two equals addresses must have the same family

        size_t logic_size_a = a.addr->sa_family == AF_INET ? sizeof(struct sockaddr_in) : sizeof(struct sockaddr_in6);
        size_t logic_size_b = b.addr->sa_family == AF_INET ? sizeof(struct sockaddr_in) : sizeof(struct sockaddr_in6);

        if (logic_size_a == a.addrlen && logic_size_b == b.addrlen) {
            // the len field must be consistent with the sa_family attribute

            if (a.addr->sa_family == AF_INET) {
                // finally we can perform the comparison
                // (little trick for IPv4: memcmp doesn't work because of sin_zero random padding)
                struct sockaddr_in *in_a = (struct sockaddr_in *) a.addr;
                struct sockaddr_in *in_b = (struct sockaddr_in *) b.addr;
                return in_a->sin_addr.s_addr == in_b->sin_addr.s_addr
                       && in_a->sin_port == in_b->sin_port ? 0 : -1;
            } else {
                // todo: not the cleanest way to compare to sockaddr_in6 but it works
                // (unlike memcmp over the whole address, which doesn't)
                char buf1[512];
                char buf2[512];
                inet_ntop(AF_INET6, &((struct sockaddr_in6 *)a.addr)->sin6_addr, buf1, 512);
                inet_ntop(AF_INET6, &((struct sockaddr_in6 *)b.addr)->sin6_addr, buf2, 512);

                return strcmp(buf1, buf2);
            }
        }

        printf("logic size difference :'( %d %d %d %d\n", logic_size_a, logic_size_b, a.addrlen, b.addrlen);
    }

    printf("family difference .. %d %d\n", a.addr->sa_family, b.addr->sa_family);
    return -1;
}

void test(int test, char *testname) {
    if (test) {
        printf("[ ] success: %s\n", testname);
    } else {
        printf("[X] failure: %s\n", testname);
    }
}