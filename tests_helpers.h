#ifndef TESTS_HELPERS_H
#define TESTS_HELPERS_H

// Simple structure used to store a pair <sockaddr, socklen_t>
struct simpleaddr {
    struct sockaddr *addr;
    socklen_t addrlen;
};

// One line constructor for simpleaddr structure
struct simpleaddr simpleaddr_make(struct sockaddr *addr, socklen_t addrlen);

// Function used to create a "simpleaddr" structure from a host name,
// a port value and a IP family : AF_INET or AF_INET6
struct simpleaddr simpleaddr_resolve(const char *host, uint16_t port, int family_hint);

// Free memory from a simpleaddr structure
void simpleaddr_free(struct simpleaddr *myaddr);

// Compare two "simple addr" structures
// Returns 0 if the two structures represent the same address
int simpleaddr_compare(struct simpleaddr a, struct simpleaddr b);

// Display fancy messages depending on the result of a condition
void test(int condition, char *testname);

#endif