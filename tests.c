/*
 * Testing procedure used to ensure the correctness of the mptcplib
 *
 * Uses a predefined testing procedure that is the following.
 * Either for IP v4 or IP v6
 *      1. Try to connect to a server
 *      2. Establish 5 additional subflows
 *           4 valid subflows
 *           1 invalid subflow (source port < 1000)
 *      3. List all subflow IDS
 *      4. List all subflow tuples
 *      5. Close every subflow excepted the last one
 *      6. Verify the remaining subflows
 *      7. Set a socket option for the last subflow
 *          IP_TOS = 28
 *      8. Get the same socket option some moments later
 *          (to check if it's still 28)
 *
 * As the running scheme is predefined, we can thus check at runtime
 * the value of all components of the different structures:
 *      syscall_res_sockopt, syscall_res_subtuple, syscall_res_subids
 *
 * This ensures that the tests have a good coverage of the library
 */

#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>

#include "mptcp.h"
#include "tests_helpers.h"

// Configurable variables

#define DISTANT_ENDPOINT "www.multipath-tcp.org"
#define LOCAL_ENDPOINT_1 "192.168.33.10"
#define LOCAL_ENDPOINT_2 "192.168.34.10"
#define LOCAL_ENDPOINT_V6 "fde4:8dba:82e1::c4"

// Definitions that set the testing methodology

int FLOW_PRIORITIES[] = {-1, 0, 1, 0, 1, 0};
int FLOW_IDS[] = {1, 2, 3, 4, 5, 6};

// Establish the connection between bind_address and connect_address for a given domain type
// Returns a file descriptor
int establish_connection(struct simpleaddr bind_addr, struct simpleaddr connect_addr, int domain) {

    int sockfd = socket(domain, SOCK_STREAM, 0);
    if (sockfd <= 0) {
        perror("[!] Test failed; socket error");
        return -1;
    }

    int reuse = 1;
    if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, (const char *) &reuse, sizeof(reuse)) < 0) {
        perror("[!] Test failed; SO_REUSEADDR error");
        return -1;
    }

    int resbind = bind(sockfd, bind_addr.addr, bind_addr.addrlen);
    if (resbind != 0) {
        perror("[!] Test failed; bind error");
        return -1;
    }

    int resconnect = connect(sockfd, connect_addr.addr, connect_addr.addrlen);
    if (resconnect != 0) {
        perror("[!] Test failed; connect error");
        return -1;
    }

    char *send_buf = "GET / HTTP/1.1\r\n\r\n";
    ssize_t send_n = send(sockfd, send_buf, strlen(send_buf), 0);
    if (send_n <= 0) {
        perror("[!] Test failed; send error");
        return -1;
    }

    char rcv_buf[512];
    ssize_t recv_n = recv(sockfd, rcv_buf, 512, 0);
    if (recv_n <= 0) {
        perror("[!] Test failed; recv error");
    }

    return sockfd;
}


void test_mptcplib_open_sub(int sockfd, struct simpleaddr local[6], struct simpleaddr distant[6]) {

    char buf[512];
    int flow;

    for (flow = 1; flow < 6; flow++) {
        // For the flow 1..5 (those allocated with mptcplib)

        struct syscall_res_subtuple opened_flow
                = mptcplib_open_sub(sockfd,
                                    local[flow].addr, local[flow].addrlen,
                                    distant[flow].addr, distant[flow].addrlen,
                                    FLOW_PRIORITIES[flow]);

        // Excepts subflow 1..4 to succeeded and subflow 5 to fail
        if(flow != 5){
            sprintf(buf, "result.errnoValue for flow #%d", flow);
            test(opened_flow.errnoValue == 0, buf);
        }
        else{
            sprintf(buf, "result.errnoValue for flow #%d", flow);
            test(opened_flow.errnoValue != 0, buf);
        }

        // And check many information for established subflows 1..4
        if (flow != 5) {
            // returned id
            sprintf(buf, "result.id for flow #%d (it should be %d)", flow, FLOW_IDS[flow]);
            test(opened_flow.id == FLOW_IDS[flow], buf);

            // local address
            sprintf(buf, "result.local correctness for flow #%d", flow);
            test(
                    simpleaddr_compare(
                            simpleaddr_make(opened_flow.local, opened_flow.local_len),
                            local[flow]
                    ) == 0,
                    buf
            );

            // remote address
            sprintf(buf, "res.remote correctness for flow #%d", flow);
            test(
                    simpleaddr_compare(
                            simpleaddr_make(opened_flow.remote, opened_flow.remote_len),
                            distant[flow]
                    ) == 0,
                    buf
            );
        }

        mptcplib_free_res_subtuple(&opened_flow);
    }
}

void test_mptcplib_get_sub_ids(int sockfd, struct simpleaddr local[6], struct simpleaddr distant[6]) {

    struct syscall_res_subids output_ids = mptcplib_get_sub_ids(sockfd);
    test(output_ids.errnoValue == 0, "mptcplib_get_sub_ids errno");
    test(output_ids.ids->sub_count == 5, "mptcplib_get_sub_ids sub_count");

    int flow;
    char buf[512];
    for (flow = 0; flow < 5; flow++) {
        // for subflows 0..4

        // check the value of the ID
        sprintf(buf, "sub_status.ids correctness for flow #%d", flow);
        test(output_ids.ids->sub_status[4 - flow].id == FLOW_IDS[flow], buf);

        if (flow >= 1) {
            // for subflows 1..4 (those opened with mptcplib)
            // check the prioriy

            sprintf(buf, "sub_status.low_prio correctness for flow #%d", flow);
            test(output_ids.ids->sub_status[4 - flow].low_prio == FLOW_PRIORITIES[flow], buf);
        }
    }
    mptcplib_free_res_subids(&output_ids);
}

void test_mptcplib_get_sub_tuple(int sockfd, struct simpleaddr local[6], struct simpleaddr distant[6]) {
    int flow;
    char buf[512];
    for (flow = 0; flow < 5; flow++) {
        // for subflows 0..4

        // get the subflow tuple
        struct syscall_res_subtuple output_tuple = mptcplib_get_sub_tuple(sockfd, FLOW_IDS[flow]);

        // check the errno result
        sprintf(buf, "tuple.errnoValue correctness for flow #%d", flow);
        test(output_tuple.errnoValue == 0, buf);

        // the local address
        sprintf(buf, "tuple.local correctness for flow #%d", flow);
        test(
                simpleaddr_compare(
                        simpleaddr_make(output_tuple.local, output_tuple.local_len),
                        local[flow]
                ) == 0,
                buf
        );

        // the remote address
        sprintf(buf, "tuple.remote correctness for flow #%d", flow);
        test(
                simpleaddr_compare(
                        simpleaddr_make(output_tuple.remote, output_tuple.remote_len),
                        distant[flow]
                ) == 0,
                buf
        );

        if (flow > 0) {
            // and for flows 1..4 (those allocated with mptcplib)
            //   also check the flow prioriry

            sprintf(buf, "tuple.low_prio correctness for flow #%d", flow);
            test(output_tuple.low_prio == FLOW_PRIORITIES[flow], buf);
        }

        mptcplib_free_res_subtuple(&output_tuple);
    }

}

void test_mptcplib_close_sub(int sockfd, struct simpleaddr local[6], struct simpleaddr distant[6]) {

    int flow;
    char buf[512];
    for (flow = 0; flow < 4; flow++) {
        // for subflows 0..3 (all subflows excepted the last one)

        // close the subflow
        int res = mptcplib_close_sub(sockfd, FLOW_IDS[flow], 0);
        sprintf(buf, "close errno correctness for flow #%d", flow);
        test(res == 0, buf);
    }
    usleep(100000);

    // also try to close an invalid subflow
    test(mptcplib_close_sub(sockfd, 100, 0), "trying to close invalid subflow");

    // finally, checks the last remaining subflow
    struct syscall_res_subids output_ids_afterclose = mptcplib_get_sub_ids(sockfd);
    test(output_ids_afterclose.ids->sub_count == 1, "counting flow remaining");
    int remaining_id = output_ids_afterclose.ids->sub_status[0].id;
    test(remaining_id == FLOW_IDS[4], "last flow ID");
    mptcplib_free_res_subids(&output_ids_afterclose);
}

void test_mptcplib_sockopt(int sockfd, struct simpleaddr local[6], struct simpleaddr distant[6]) {
    // get the last subflow
    int remaining_id = FLOW_IDS[4];

    // set IP_TOS to 28
    int val = 28;
    int set_res = mptcplib_set_sub_sockopt(sockfd, remaining_id, SOL_IP, IP_TOS, &val, sizeof(int));
    test(set_res == 0, "setsockopt success status");

    // get IP_TOS
    struct syscall_res_sockopt get_res = mptcplib_get_sub_sockopt(sockfd, remaining_id, SOL_IP, IP_TOS, sizeof(int));

    // check that it's equal to 28 (and also check data consistency)
    test(get_res.errnoValue == 0, "getsockopt success status");
    test(get_res.retsize == sizeof(int), "getsockopt value size");
    test(*((int *) get_res.value) == 28, "getsockopt value");
    mptcplib_free_res_sockopt(&get_res);
};

// Tests all the functionalities of the mptcp lib for a list of 6 given local and distant addresses
void test_all(struct simpleaddr local[6], struct simpleaddr distant[6], int domain) {
    // Set up a connection using the first addresses: local[0] and distant[0]
    puts("\nsetting up...\n");
    int sockfd = establish_connection(local[0], distant[0], domain);

    // Opens subflows local[1],distant[1] ... local[5],distant[5]
    // And excepts the last subflow to fail
    puts("\ntesting mptcplib_open_sub...");
    test_mptcplib_open_sub(sockfd, local, distant);

    // Retrieves the subflow ids and check priority
    puts("\nretrieving mptcplib_get_sub_ids...");
    test_mptcplib_get_sub_ids(sockfd, local, distant);

    // And also the local and remote addresses
    puts("\ntesting mptcplib_get_sub_tuple...\n");
    test_mptcplib_get_sub_tuple(sockfd, local, distant);

    // Finally, close every subflow excepted the last one, then
    // performs some tests
    puts("\ntesting mptcplib_close_sub...\n");
    test_mptcplib_close_sub(sockfd, local, distant);

    // Finally, set a socket option on the last remaining flow and tries
    // to retrieve its value lates
    puts("\ntesting socket options...\n");
    test_mptcplib_sockopt(sockfd, local, distant);

    shutdown(sockfd, 0);
}


int main() {
    puts("==== Testing IP v4 ====");

    // what will be the different flow established
    // (need 6 flows with last one incorrect)
    struct simpleaddr LOCAL_ADDR[] = {
            simpleaddr_resolve(LOCAL_ENDPOINT_1, 64001, AF_INET),
            simpleaddr_resolve(LOCAL_ENDPOINT_1, 64100, AF_INET),
            simpleaddr_resolve(LOCAL_ENDPOINT_1, 64200, AF_INET),
            simpleaddr_resolve(LOCAL_ENDPOINT_2, 64300, AF_INET),
            simpleaddr_resolve(LOCAL_ENDPOINT_2, 64400, AF_INET),
            simpleaddr_resolve(LOCAL_ENDPOINT_2, 10, AF_INET)};

    struct simpleaddr REMOTE_ADDRS[] = {
            simpleaddr_resolve(DISTANT_ENDPOINT, 80, AF_INET),
            simpleaddr_resolve(DISTANT_ENDPOINT, 80, AF_INET),
            simpleaddr_resolve(DISTANT_ENDPOINT, 80, AF_INET),
            simpleaddr_resolve(DISTANT_ENDPOINT, 80, AF_INET),
            simpleaddr_resolve(DISTANT_ENDPOINT, 80, AF_INET),
            simpleaddr_resolve(DISTANT_ENDPOINT, 80, AF_INET)};

    test_all(LOCAL_ADDR, REMOTE_ADDRS, AF_INET);
    int flow;

    for (flow = 0; flow < 6; flow++) {
        simpleaddr_free(&LOCAL_ADDR[flow]);
        simpleaddr_free(&REMOTE_ADDRS[flow]);
    }

    puts("==== Testing IP v6 ====");

    // what will be the different flow established
    // (need 6 flows with last one incorrect)
    struct simpleaddr LOCAL_ADDR6[] = {
            simpleaddr_resolve(LOCAL_ENDPOINT_V6, 64001, AF_INET6),
            simpleaddr_resolve(LOCAL_ENDPOINT_V6, 64100, AF_INET6),
            simpleaddr_resolve(LOCAL_ENDPOINT_V6, 64200, AF_INET6),
            simpleaddr_resolve(LOCAL_ENDPOINT_V6, 64300, AF_INET6),
            simpleaddr_resolve(LOCAL_ENDPOINT_V6, 64400, AF_INET6),
            simpleaddr_resolve(LOCAL_ENDPOINT_V6, 10, AF_INET6)};

    struct simpleaddr REMOTE_ADDRS6[] = {
            simpleaddr_resolve(DISTANT_ENDPOINT, 80, AF_INET6),
            simpleaddr_resolve(DISTANT_ENDPOINT, 80, AF_INET6),
            simpleaddr_resolve(DISTANT_ENDPOINT, 80, AF_INET6),
            simpleaddr_resolve(DISTANT_ENDPOINT, 80, AF_INET6),
            simpleaddr_resolve(DISTANT_ENDPOINT, 80, AF_INET6),
            simpleaddr_resolve(DISTANT_ENDPOINT, 80, AF_INET6)};

    test_all(LOCAL_ADDR6, REMOTE_ADDRS6, AF_INET6);
    for (flow = 0; flow < 6; flow++) {
        simpleaddr_free(&LOCAL_ADDR6[flow]);
        simpleaddr_free(&REMOTE_ADDRS6[flow]);
    }
}