/*
** A simple HTTP Server
   This program runs together with client-thread-main-2020.c as one system.

   The server, this program, runs as a simple HTTP server, serving
   the HTTP GET method to get webpages.

   This clinet specifies the host and port of the HTTP server and
   the webpage it wants to access.

   To demo the whole system, you must:
   1. compile the server program:
        gcc -lpthread -o server server-thread-2020.c server-thread-main-2020.c
   2. run the program: server 41000 &
   3. compile the cliient program:
        gcc -o client client-thread-2020.c client-thread-main-2020.c
   4. run the client on another machine: client HOST 41000 webpage
         replacing HOST and HTTPPORT with the host the server runs
         on and the port # the server runs at.

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
#include "gc_server-thread-2020.h"

#define HOST "freebsd1.cs.scranton.edu"
#define BACKLOG 10

void start_subserver(int reply_sock_fd);
void *subserver(void * reply_sock_fd_as_ptr);

int main(int argc, char *argv[]) {
    int http_sock_fd;			// http server socket
    int reply_sock_fd;  		// client connection 
    int yes;

    if (argc != 2) {
       printf("Run: program port#\n");
       return 1;
    }
    http_sock_fd = get_server_socket(HOST, argv[1]);

    if (start_server(http_sock_fd, BACKLOG) == -1) {
       printf("start server error\n");
       exit(1);
    }

    while(1) {
        if ((reply_sock_fd = accept_client(http_sock_fd)) == -1) {
            continue;
        }

        start_subserver(reply_sock_fd);
    }
} 

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

void *subserver(void * reply_sock_fd_as_ptr) {
    long reply_sock_fd_long = (long) reply_sock_fd_as_ptr;
    int reply_sock_fd = (int) reply_sock_fd_long;

    char *html_file;
    int html_file_fd;
    int read_count = -1;
    int BUFFERSIZE = 256;
    char buffer[BUFFERSIZE+1];

    printf("subserver ID = %ld\n", (unsigned long) pthread_self());

    read_count = recv(reply_sock_fd, buffer, BUFFERSIZE, 0);
    buffer[read_count] = '\0'; // to safe-guard the string
    printf("%s\n", buffer);

    // get the file name according to HTTP GET method protocol
    html_file = strtok(&buffer[5], " \t\n");
    printf("FILENAME: %s\n", html_file);
    html_file_fd = open(html_file, O_RDONLY);
    while ((read_count = read(html_file_fd, buffer, BUFFERSIZE))>0) {

       send(reply_sock_fd, buffer, read_count, 0);
    }
    close(reply_sock_fd);

    return NULL;
}
