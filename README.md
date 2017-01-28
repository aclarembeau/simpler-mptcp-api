# simpler-mptcp-api

Additional C layer over the multipath tcp socket API that makes the utilisation simpler. 
The current implementation of the MPTCP socket API is using a mechanism that relies extensively on the linux commands `setsockopt` and `getsockopt`. It also use some complex data structures (such as zero-size C array) that makes the API non very heasy to understand for non experts. 

The goal of this library is to simplify all those call by exposing a "syscall like interface" that I found much simpler. 

## How to use the library 

To use the library, the only thing to do is to include the mptcp.c and mptcp.h files in your project and then to use the different functions available. You can also build it as a shared library and link it with your applications. 

In order to make the MPTCP functions run, you need a MPTCP-capable host with the native MPTCP socket API enabled (you can for instance find one vagrant box with all those functionalities here: https://github.com/hoang-tranviet/mptcp-vagrant). 

Then, just use the different functions and structures below: 

          // new structures ----------------------------------------------------------

          // List of subflow IDs and priorities
          struct syscall_res_subids {
              int errnoValue;
              struct mptcp_sub_ids *ids;
          };

          // Subflow tuple
          struct syscall_res_subtuple {
              int errnoValue;
              int id;                                // Subflow id
              int low_prio;                          // Subflow priority
              struct sockaddr *local, *remote;       // Local and remote endpoints
              size_t local_len, remote_len;          // Local and remote endpoints sizes
          };


          struct syscall_res_sockopt {
              int errnoValue;
              void *value;      // sockopt value returned by getsockopt
              size_t retsize;   // size of the value variable
          };

          // structure freeing functions ---------------------------------------------
          
          void mptcplib_free_res_subids(struct syscall_res_subids *ids);
          void mptcplib_free_res_subtuple(struct syscall_res_subtuple *tuple);
          void mptcplib_free_res_sockopt(struct syscall_res_sockopt *sockopt);

          // subflow manipulation ---------------------------------------------------

          // Open a new subflow
          struct syscall_res_subtuple mptcplib_open_sub(int sockfd,
                                                        struct sockaddr *sourceaddr, size_t sourcelen,
                                                        struct sockaddr *destaddr, size_t destlen,
                                                        int prio);

          // Close a specific subflow
          int mptcplib_close_sub(int sockfd, int id, int how);

          // Get all the subflow ids
          struct syscall_res_subids mptcplib_get_sub_ids(int sockfd);

          // Get information about a specific subflow
          struct syscall_res_subtuple mptcplib_get_sub_tuple(int sockfd, int id);

          // Get subflow socket options
          struct syscall_res_sockopt mptcplib_get_sub_sockopt(int sockfd, int id, int level, int opt, size_t size);

          // Set subflow socket options
          int mptcplib_set_sub_sockopt(int sockfd, int id, int level, int opt, void *val, size_t size);

The different functions are all either returning an integer value or a structure that contains an errnoValue field. If this integer is not equal to 0, it means that the execution of the function failed. 
You can have more details about the error code using the `strerror` function and the indications of https://tools.ietf.org/html/draft-hesmans-mptcp-socket-00 . 

## What projects use it? 

Currently, the library is still in a very experimental state. 
Even if it has been thorously tested (the tests.c file included a broad serie of unit tests that checks the behavior of all the different functions provided in IPv6 and IPv4), it isn't yet included in a lot of different project. 
So, if you find a good application that you want to share with me, just send me a message. It would be a pleasure for me to add it in this list. 

So, for the moment, the only code that use the library is my own go binding of the MPTCP socket API (https://github.com/aclarembeau/go-mptcp-api). The library was for me an efficient tool to avoid memory leaks when mixing IPv4 and IPv6 usage. It also simplifies a lot the transmission of the data from C to Go (because zero sized array and padded structures are very hard to handle with the cgo tool @see https://github.com/golang/go/issues/13919). 



