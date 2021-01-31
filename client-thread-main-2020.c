/*
** a simple web browser
** compile: gcc -o client client-thread-main-2020.c client-thread-2020.c
** run: client HOST HTTPPORT webpage
**
** the server is supposed to start on port 4000 and
** make sure the server has been started before this.
**
** Basic steps for client to communicate with a servr
** 1: get a socket
** 2: connect to the server using ip and port
** 3: read and/or write as many times as needed
** 4: close the socket.
*/

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <netdb.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include "client-thread-2020.h"

// you must change the port to your team's assigned port. 
#define BUFFERSIZE 256 

void compose_http_request(char *http_request, char *filename);
void web_browser(int web_server_socket, char *http_request);

int main(int argc, char *argv[])
{
    int web_server_socket;  
    char http_request[BUFFERSIZE];

    if (argc != 4) {
        printf("usage: client HOST HTTPORT webpage\n");
        exit(1);
    }

    if ((web_server_socket = get_server_connection(argv[1], argv[2])) == -1) {
       printf("connection error\n");
       exit(1);
    }

    compose_http_request(http_request, argv[3]);

    web_browser(web_server_socket, http_request);

    close(web_server_socket);
}

void  compose_http_request(char *http_request, char *filename) {
    strcpy(http_request, "GET /");
    strcpy(&http_request[5], filename);
    strcpy(&http_request[5+strlen(filename)], " ");
}

void web_browser(int http_conn, char *http_request) {
    int numbytes = 0;
    char buf[256];
    // step 4.1: send the HTTP request
    send(http_conn, http_request, strlen(http_request), 0);

    // step 4.2: receive message from server
    while ((numbytes=recv(http_conn, buf, sizeof(buf),  0)) > 0) {
       // step 4.3: the received may not end with a '\0' 
       buf[numbytes] = '\0';
       printf("%s",buf);
    }
    if ( numbytes < 0)  {
       perror("recv");
       exit(1);
    }

}
