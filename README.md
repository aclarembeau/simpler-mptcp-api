# simpler-mptcp-api

Library that extends and simply the usage of the multipath socket API (https://tools.ietf.org/html/draft-hesmans-mptcp-socket-00). 
By providing a set of simple functions, this library allows to open, close or edit subflows of a MPTCP connection from an applicative level. 

## Current state of developpement

For the moment, the multipath-tcp socket API that serves to make this library is still in an experimental state. 
Functionalities such as flow priority, closing modes (how parameter) and assignation of the IP field during the open_sub call aren't working yet. 
Thus, even if the project has been thorously tested (mainly unit tests and memory leak detection), it isn't recommended to use it in production. 

## How to use it? 

To use the library, you just have to copy the files "mptcplib.c" and "mptcplib.h" in your project or, you can also create a shared library. 
After that, if you have a machine with multipath-tcp enabled and the native socket API (such as you can get with this vagrant box https://github.com/hoang-tranviet/mptcp-vagrant), you would be able to use the following functions: 

```
// Open a new subflow
int mptcplib_open_sub(int sockfd, struct mptcplib_flow *tuple);

// Close a specific subflow
int mptcplib_close_sub(int sockfd, int id, int how);

// Get all the subflow ids
struct mptcplib_getsubids_result mptcplib_get_sub_ids(int sockfd);

// Get information about a specific subflow
struct mptcplib_getsubtuple_result mptcplib_get_sub_tuple(int sockfd, int id);

// Set subflow socket options
int mptcplib_set_sub_sockopt(int sockfd, int id, int level, int opt, void *val, size_t size);

// Get subflow socket options
struct mptcplib_getsubsockopt_result mptcplib_get_sub_sockopt(int sockfd, int id, int level, int opt, size_t size);
```

Those functions works thanks to different structures that can carry information to the library: 

```
struct mptcplib_flow {
    int id;                             // Subflow id
    int low_prio;                       // Subflow priority
    struct sockaddr *local, *remote;    // Local and remote endpoints
    size_t local_len, remote_len;       // Local and remote endpoints sizes
};

struct mptcplib_intarray {
    int count;
    int *values;
};

// Structure for the result of getsubtuple
struct mptcplib_getsubtuple_result {
    int errnoValue;
    struct mptcplib_flow flow ;
};

// Structure for the result of getsubids
struct mptcplib_getsubids_result {
    int errnoValue;
    struct mptcplib_intarray ids;
};

// Structure for the result of getsockopt
struct mptcplib_getsubsockopt_result {
    int errnoValue;
    void *value;
    size_t retsize;
};
```

The two first structures are used to represent respectively a TCP flow and an int array while the three last gather the result of the different functions. 

And, as the manipulation of network structures such as sockaddr and memory management can be sometimes quite complicated, the library also expose one function to create a flow from source and destination host names: 

```
// Creates a flow with given host and port and default priority
struct mptcplib_flow mptcplib_make_flow(char *source_host, unsigned short source_port, char *dest_host, unsigned short dest_port);
```

And to manage memory: 

```
void mptcplib_free_intarray(struct mptcplib_intarray arr);
void mptcplib_free_flow(struct mptcplib_flow tuple);
void mptcplib_free_getsubtockopt_result(struct mptcplib_getsubsockopt_result sockopt);
```

## Examples 

Beside from the tests.c file, which contains a procedure to ensure the quality of the code, you can find an exemple of utilisation of the library below: 

```
// establish a connection
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

// make a HTTP request
char *sndbuf = "GET / HTTP/1.0\r\n\r\n";
int n_sent = send(sockfd, sndbuf, strlen(sndbuf), 0);
if(n_sent <= 0){
          perror("send");
          return;
}

mptcplib_free_flow(flow_main);

// open some flow
struct mptcplib_flow flow1 = mptcplib_make_flow("192.168.33.10", 64002, "multipath-tcp.org", 80);
struct mptcplib_flow flow2 = mptcplib_make_flow("192.168.34.10", 64003, "multipath-tcp.org", 80);

int errSub1 = mptcplib_open_sub(sockfd, &flow1);
int errSub2 = mptcplib_open_sub(sockfd, &flow2);


mptcplib_free_flow(flow1);
mptcplib_free_flow(flow2);

// list the flows
struct mptcplib_getsubids_result subidsres = mptcplib_get_sub_ids(sockfd);
int i;
for(i = 0 ; i < subidsres.ids.count; i++){
          struct mptcplib_getsubtuple_result tupleRes = mptcplib_get_sub_tuple(sockfd, subidsres.ids.values[i]);
          printf("tuple priority: %d\n", tupleRes.flow.prio); 
          mptcplib_free_flow(tupleRes.flow);
}

// close one flow
mptcp_close_sub(subidsres.ids.values[0]); 
```
    
## Projects that are using this library 

For the moment, not so many projects are using this library. So, if you plan to use it, feel free to leave a message and it will be a pleasure to mention it here. 

simpler-mptcp-api is only used now as a basis for my own go binding of the multipath-tcp socket API: 
https://github.com/aclarembeau/go-mptcp-api

