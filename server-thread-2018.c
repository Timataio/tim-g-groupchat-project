/*
** A simple HTTP Server
   This program runs together with client-thread.c as one system.

   The server, this program, runs as a simple HTTP server, serving
   the HTTP GET method to get webpages. 

   This clinet specifies the host and port of the HTTP server and
   the webpage it wants to access. 

   To demo the whole system, you must:
   1. compile the program: cc -lpthread -o server server-thread.c
   2. run the program: server 41000 &
   3. compile the cliient-thread.c: cc -o client client-thread.c
   4. run the client on another machine: client HOST 41000 webpage
         replacing HOST and HTTPPORT with the values you defined in
         the server.

   FOR STUDNETS:
     If you want to try this demo, you MUST use the port # assigned
     to you by your instructor. Since each http port can be associated
     to only one process (or one server), if another student tries to 
     run the server with the default port, it will be rejected. 
*/


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <netdb.h>
#include <pthread.h>

// you must uncomment the next two lines and make changes
#define HOST "freebsd1.cs.scranton.edu" // the hostname of the HTTP server
#define BACKLOG 10                     // how many pending connections queue will hold

void *get_in_addr(struct sockaddr * sa);             // get internet address
int get_server_socket(char *hostname, char *port);   // get a server socket
int start_server(int serv_socket, int backlog);      // start server's listening
int accept_client(int serv_sock);                    // accept a connection from client
void start_subserver(int reply_sock_fd);             // start subserver as a thread
void *subserver(void *reply_sock_fd_ptr);            // subserver - subserver
void print_ip( struct addrinfo *ai);                 // print IP info from getaddrinfo()

int main(int argc, char *argv[])
{
    int http_sock_fd;			// http server socket
    int reply_sock_fd;  		// client connection 
    int yes;

    if (argc != 2) {
       printf("Run: program port#\n");
       return 1;
    }
    // steps 1-2: get a socket and bind to ip address and port
    http_sock_fd = get_server_socket(HOST, argv[1]);

    // step 3: get ready to accept connections
    if (start_server(http_sock_fd, BACKLOG) == -1) {
       printf("start server error\n");
       exit(1);
    }

    while(1) {  // accept() client connections
        // step 4: accept a client connection
        if ((reply_sock_fd = accept_client(http_sock_fd)) == -1) {
            continue;
        }

        // steps 5, 6, 7: read from and write to sockets, close socket
        start_subserver(reply_sock_fd);
    }
} 

int get_server_socket(char *hostname, char *port) {
    struct addrinfo hints, *servinfo, *p;
    int status;
    int server_socket;
    int yes = 1;

    memset(&hints, 0, sizeof hints);
    hints.ai_family = PF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE;

    if ((status = getaddrinfo(hostname, port, &hints, &servinfo)) != 0) {
       printf("getaddrinfo: %s\n", gai_strerror(status));
       exit(1);
    }

    for (p = servinfo; p != NULL; p = p ->ai_next) {
       // step 1: create a socket
       if ((server_socket = socket(p->ai_family, p->ai_socktype,
                           p->ai_protocol)) == -1) {
           printf("socket socket \n");
           continue;
       }
       // if the port is not released yet, reuse it.
       if (setsockopt(server_socket, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int)) == -1) {
         printf("socket option\n");
         continue;
       }

       // step 2: bind socket to an IP addr and port
       if (bind(server_socket, p->ai_addr, p->ai_addrlen) == -1) {
           printf("socket bind \n");
           continue;
       }
       break;
    }
    print_ip(servinfo);
    freeaddrinfo(servinfo);   // servinfo structure is no longer needed. free it.

    return server_socket;
}

int start_server(int serv_socket, int backlog) {
    int status = 0;
    if ((status = listen(serv_socket, backlog)) == -1) {
        printf("socket listen error\n");
    }
    return status;
}

int accept_client(int serv_sock) {
    int reply_sock_fd = -1;
    socklen_t sin_size = sizeof(struct sockaddr_storage);
    struct sockaddr_storage client_addr;
    char client_printable_addr[INET6_ADDRSTRLEN];

    // accept a connection request from a client
    // the returned file descriptor from accept will be used
    // to communicate with this client.
    if ((reply_sock_fd = accept(serv_sock, 
       (struct sockaddr *)&client_addr, &sin_size)) == -1) {
            printf("socket accept error\n");
    }
    else {
        // here is for info only, not really needed.
        inet_ntop(client_addr.ss_family, get_in_addr((struct sockaddr *)&client_addr), 
                          client_printable_addr, sizeof client_printable_addr);
        printf("server: connection from %s at port %d\n", client_printable_addr,
                            ((struct sockaddr_in*)&client_addr)->sin_port);
    }
    return reply_sock_fd;
}

/* NOTE: the value of reply_sock_fd is passed to pthread_create() as the
         parameter, instead of the address of reply_sock_fd. This is
         necessary because reply_sock_fd is a parameter (or local varaible)
         of start_server. After start_server returns, reply_sock_id becomes
         invalid. if the new thread is not executed before start_server()
         returns, the thread would be an invalid reply_sock_fd. Passing the
         value of reply_sock_fd avoids this problem, since the valid is
         copied to the local variable of the thread. Because our server1 is
         of 64-bits and int in c is 32 bits, we need to do type conversions
         to not cause compilation warnings
*/ 
void start_subserver(int reply_sock_fd) {
   pthread_t pthread;
   long reply_sock_fd_long = reply_sock_fd;
   if (pthread_create(&pthread, NULL, subserver, (void *)reply_sock_fd_long) != 0) {
      printf("failed to start subserver\n");
   }
   else {
      printf("subserver %ld started\n", (unsigned long)pthread);
   }
}

/* this is the subserver who really communicate
   with client through the reply_sock_fd.  */
void *subserver(void * reply_sock_fd_as_ptr) {
    char *html_file;
    int html_file_fd;
    int read_count = -1;
    int BUFFERSIZE = 256;
    char buffer[BUFFERSIZE+1];

    // we cast the parameter of the function to an int, 
    // not treat it as a pointer. Read the comments for
    // start_subserver() for details.
    long reply_sock_fd_long = (long) reply_sock_fd_as_ptr;
    int reply_sock_fd = (int) reply_sock_fd_long;
    printf("subserver ID = %ld\n", (unsigned long) pthread_self());
    // step 5 receive from the client
    read_count = recv(reply_sock_fd, buffer, BUFFERSIZE, 0);
    buffer[read_count] = '\0'; // to safe-guard the string
    printf("%s\n", buffer);

    // get the file name according to HTTP GET method protocol
    html_file = strtok(&buffer[5], " \t\n");
    printf("FILENAME: %s\n", html_file);
    html_file_fd = open(html_file, O_RDONLY);
    while ((read_count = read(html_file_fd, buffer, BUFFERSIZE))>0) {

       // step 6: send message to client
       send(reply_sock_fd, buffer, read_count, 0);
    }
    // step 7: close the socket to client
    close(reply_sock_fd);

    return NULL;
 }
// ======= HELP FUNCTIONS =========== //
/* the following is a function designed for testing.
   it prints the ip address and port returned from
   getaddrinfo() function */
void print_ip( struct addrinfo *ai) {
   struct addrinfo *p;
   void *addr;
   char *ipver;
   char ipstr[INET6_ADDRSTRLEN];
   struct sockaddr_in *ipv4;
   struct sockaddr_in6 *ipv6;
   short port = 0;

   for (p = ai; p !=  NULL; p = p->ai_next) {
      if (p->ai_family == AF_INET) {
         ipv4 = (struct sockaddr_in *)p->ai_addr;
         addr = &(ipv4->sin_addr);
         port = ipv4->sin_port;
         ipver = "IPV4";
      }
      else {
         ipv6= (struct sockaddr_in6 *)p->ai_addr;
         addr = &(ipv6->sin6_addr);
         port = ipv4->sin_port;
         ipver = "IPV6";
      }
      inet_ntop(p->ai_family, addr, ipstr, sizeof ipstr);
      printf("serv ip info: %s - %s @%d\n", ipstr, ipver, ntohs(port));
   }
}

void *get_in_addr(struct sockaddr * sa) {
   if (sa->sa_family == AF_INET) {
      printf("ipv4\n");
      return &(((struct sockaddr_in *)sa)->sin_addr);
   }
   else {
      printf("ipv6\n");
      return &(((struct sockaddr_in6 *)sa)->sin6_addr);
   }
}
