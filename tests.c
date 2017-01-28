/**
 * Testing suite for the mptcplib
 *
 * By CLAREMBEAU Alexis
 * 01/24/2017
 */

#include "mptcplib.h"
#include <sys/socket.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/ip.h>

void test(const char *name, int condition, char *success, char *failure){
    if(condition){
        printf("  [ ] %s : %s\n", name, success);
    }
    else{
        printf("  [X] %s : %s\n", name, failure);
    }
}

int main(int argc, char **argv){
    // initializing the connection
    int sockfd = socket(AF_INET, SOCK_STREAM, 0);

    struct mptcplib_flow flow_main = mptcplib_make_flow("192.168.33.10", 64001, "multipath-tcp.org", 80);
    int bind_res = bind(sockfd, flow_main.local, flow_main.local_len);
    if(bind_res != 0){
        perror("bind");
        return;
    }
    int connect_res = connect(sockfd, flow_main.remote, flow_main.remote_len);
    if(connect_res !=  0){
        perror("connect");
        return;
    }

    // making the request
    char *sndbuf = "GET / HTTP/1.0\r\n\r\n";
    int n_sent = send(sockfd, sndbuf, strlen(sndbuf), 0);
    if(n_sent <= 0){
        perror("send");
        return;
    }

    mptcplib_free_flow(flow_main);

    // opening some flow
    puts("opening some subflows");
    struct mptcplib_flow flow1 = mptcplib_make_flow("192.168.33.10", 64002, "multipath-tcp.org", 80);
    struct mptcplib_flow flow2 = mptcplib_make_flow("192.168.34.10", 64003, "multipath-tcp.org", 80);
    struct mptcplib_flow flow3 = mptcplib_make_flow("::", 64004, "multipath-tcp.org", 80);
    struct mptcplib_flow flow4 = mptcplib_make_flow("192.168.33.10", 10, "multipath-tcp.org", 80);
    struct mptcplib_flow flow5 = mptcplib_make_flow("192.168.33.10", 64002, "multisdsdpath-tcp.org", 80);
    flow1.low_prio = 1;
    flow2.low_prio = 1;
    flow3.low_prio = 1;
    flow4.low_prio = 1;
    flow5.low_prio = 1;

    int errSub1 = mptcplib_open_sub(sockfd, &flow1);
    int errSub2 = mptcplib_open_sub(sockfd, &flow2);
    int errSub3 = mptcplib_open_sub(sockfd, &flow3);
    int errSub4 = mptcplib_open_sub(sockfd, &flow4);
    int errSub5 = mptcplib_open_sub(sockfd, &flow5);

    test("opening flow #1", errSub1 == 0, "success", "failure");
    test("opening flow #2", errSub2 == 0, "success", "failure");
    test("opening flow #3", errSub3 == 0, "success", "failure");
    test("opening flow #4", errSub4 != 0, "failure", "success");
    test("opening flow #5", errSub5 != 0, "failure", "success");

    test("subflow #1 id set by opensub > 0", flow1.id > 0, "success", "failure");
    test("subflow #2 id set by opensub > 0", flow2.id > 0, "success", "failure");
    test("subflow #3 id set by opensub > 0", flow3.id > 0, "success", "failure");

    mptcplib_free_flow(flow1);
    mptcplib_free_flow(flow2);
    mptcplib_free_flow(flow3);
    mptcplib_free_flow(flow4);
    mptcplib_free_flow(flow5);

    // reading some data from the socket

    char buf[10];
    int n_recv = recv(sockfd, buf, 10, 0);
    if(n_recv <= 0){
        perror("recv");
        return;
    }

    // listing some subflows

    puts("listing subflows");
    struct mptcplib_getsubids_result subidsres = mptcplib_get_sub_ids(sockfd);
    test("listing subflows success", subidsres.errnoValue == 0 , "success", "failure");
    test("counting subflows", subidsres.ids->sub_count == 4, "== 4", "!= 4");

    puts("showing content of the subflows");
    int i;
    for(i = 0 ; i < subidsres.ids->sub_count; i++){
        struct mptcplib_getsubtuple_result tupleRes = mptcplib_get_sub_tuple(sockfd, subidsres.ids->sub_status[i].id);
        test("test inspection succeeded", tupleRes.errnoValue == 0, "success", "failure");
        test("tuple id >= 0", tupleRes.flow.id >= 0, ">= 0", "< 0");
        test("tuple prio == 1", tupleRes.flow.low_prio == 1, "==1", "!=1");

        mptcplib_free_flow(tupleRes.flow);
    }
    struct mptcplib_getsubtuple_result invalidTuple = mptcplib_get_sub_tuple(sockfd, 100);
    test("getting invalid tuple", invalidTuple.errnoValue != 0,"failed", "success");

    // removing some subflows and listing again to show the result

    puts("removing some subflows");

    int closedId1 = subidsres.ids->sub_status[0].id;
    int closedId2 = subidsres.ids->sub_status[1].id;
    int errClose1 = mptcplib_close_sub(sockfd, closedId1, 0);
    test("close test valid", errClose1 == 0, "success", "failure");
    int errClose2 = mptcplib_close_sub(sockfd, closedId2, 0);
    test("close test valid", errClose2 == 0, "success", "failure");
    int errClose3 = mptcplib_close_sub(sockfd, 100,0);
    test("close test invalid", errClose3 != 0, "failure", "success");

    usleep(100000);

    puts("listing again");

    struct mptcplib_getsubids_result subidsres2 = mptcplib_get_sub_ids(sockfd);
    test("listing subflow status", subidsres2.errnoValue == 0, "succeeded", "failed");

    for(i = 0 ; i< subidsres2.ids->sub_count; i++){
        int id = subidsres2.ids->sub_status[i].id;
        test("removed ID doesn't belong to result", id != closedId1 && id != closedId2, "success", "failure");
    }


    // finally, setting and reading a socket option from a subflow

    puts("testing socket options");
    int setId = subidsres2.ids->sub_status[0].id;

    int val = 28;
    int resSet = mptcplib_set_sub_sockopt(sockfd,setId, SOL_IP, IP_TOS, &val, sizeof(val));
    test("set subflow sockopt", resSet == 0, "success", "failure");

    struct mptcplib_getsubsockopt_result getRes = mptcplib_get_sub_sockopt(sockfd,setId, SOL_IP, IP_TOS, sizeof(val));
    test("get subflow sockopt", getRes.errnoValue == 0, "success", "failure");
    test("get subflow sockopt value", *((int*)getRes.value) == 28, "28", "!= 28");

    // closing the test suite
    mptcplib_free_getsubtockopt_result(getRes);
    mptcplib_free_getsubids_result(subidsres);
    mptcplib_free_getsubids_result(subidsres2);
    shutdown(sockfd, 0);
}