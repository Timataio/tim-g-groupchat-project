/*
** a groupchat web client
** compile: gcc -lpthread -o client gc_client.c
** run: ./client HOST PORT
**
** the server is supposed to start on port 17100 and
** make sure the server has been started before this.
**
** Basic steps for client to communicate with a servr
** 1: get a socket
** 2: connect to the server using ip and port
** 3: read and/or write as many times as needed
** 4: close the socket.
*/

#include <arpa/inet.h>
#include <errno.h>
#include <netdb.h>
#include <netinet/in.h>
#include <pthread.h>
#include <semaphore.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

#include "gc_shared.h"

#define BUFFERSIZE 256

typedef struct RCVARGS {
	int socket;
	sem_t lock;
} RcvArgs;

int get_server_connection(char *hostname, char *port);
void *userThread(void *arg);
void getInput(int socket, sem_t *lock);
int executeCommand(int socket, char *msg, sem_t *lock);
void handle_join_group(int socket);
void handle_leave_group(int socket);
void handle_change_group(int socket);
void exit_client(int socket);
void *recieveThread(void *connection);
void sendPassword(int socket, int NEW);
void getActiveUsers(int socket, char *groupName);
void getGroupNames(int socket);

//Implemented by Alexa Baldon
int main(int argc, char *argv[]) {
    int connection;  
 
    if (argc != 3) {
        printf("-- usage: client HOST PORT\n");
        exit(1);
    }

    // steps 1, 2: get a connection to server
    if ((connection = get_server_connection(argv[1], argv[2])) == -1) {
       printf("-- connection error\n");
       exit(1);
    }

    
    pthread_t user;
    pthread_create(&user, NULL, userThread, &connection);
    pthread_join(user, NULL);

    return 0;
}

int get_server_connection(char *hostname, char *port) {
    int serverfd;
    struct addrinfo hints, *servinfo, *p;
    int status;

    memset(&hints, 0, sizeof hints);
    hints.ai_family = PF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;

   if ((status = getaddrinfo(hostname, port, &hints, &servinfo)) != 0) {
       printf("getaddrinfo: %s\n", gai_strerror(status));
       return -1;
    }

    // print_ip(servinfo);
    for (p = servinfo; p != NULL; p = p ->ai_next) {
       // create a socket
       if ((serverfd = socket(p->ai_family, p->ai_socktype,
                           p->ai_protocol)) == -1) {
           printf("socket socket \n");
           continue;
       }

       // connect to the server
       if ((status = connect(serverfd, p->ai_addr, p->ai_addrlen)) == -1) {
           close(serverfd);
           printf("socket connect \n");
           continue;
       }
       break;
    }

    freeaddrinfo(servinfo);
   
    if (status != -1) return serverfd;
    else return -1;
}

// Handles user registration and login as well as
// the sending of messages.
// Implimented by Shane Novitsky

void * userThread(void *arg) {
    int connection = *((int*)arg);
    
    // ClntUserStructP
    int userBytes = 24 * sizeof(char);
    char username[userBytes];
    int passBytes = 24 * sizeof(char);
    char password[passBytes];
    
    char registered[3];
      
   // step 3: see if the user is registered
      printf("-- are you a registered user? (y/n)\n");
      fgets(registered, userBytes, stdin);
      trim(registered);
    
   if (strcmp(registered, "y") == 0) { // Handle user login
	  // Send type
	  sendMessageType(TYPE_LOGIN, connection);
	  
      printf("-- enter your username: \n");
      fgets(username, userBytes, stdin);
      trim(username);

      // Send username
      send(connection, &userBytes, sizeof(int), 0);
      send(connection, username, userBytes, 0);
	  
	  int type;
	  recv(connection, &type, sizeof(int), 0);
	  if (type == TYPE_FAIL) {
		  printf("Error: User does not exist.\n");
		  return NULL;
	  }
	  
	  sendPassword(connection, FALSE);
   }
   
   else { // Handle user registration 
         
      printf("-- enter your username: \n");
      fgets(username, userBytes, stdin);
      trim(username);

      // Send type
      sendMessageType(TYPE_REGISTRATION, connection);

      // Send username
      send(connection, &userBytes, sizeof(int), 0);
      send(connection, username, userBytes, 0);
	  
	  int type;
	  recv(connection, &type, sizeof(int), 0);
	  if (type == TYPE_FAIL) {
		  printf("Error: User already exists.\n");
		  return NULL;
	  }

      sendPassword(connection, TRUE);

      // Join group
	  char *groupInput = malloc(sizeof(char) * 21);
      printf("-- join a group! Here are the current groups\n");
      handle_join_group(connection);
   }

   // Start reciever thread
   pthread_t reciever;
   
   RcvArgs *rcvArgs = malloc(sizeof(RcvArgs));
   sem_t *lock = &(rcvArgs->lock);
   
   rcvArgs->socket = connection;
   sem_init(lock, 0, 1);
   
   pthread_create(&reciever, NULL, recieveThread, (void*)rcvArgs);

   getInput(connection, lock);
   return NULL;
}

void getInput(int socket, sem_t *lock) {
	printf("Enter #$%% to use commands.\nSelect your active group using the change command to get started.\n-- enter messages:\n");
	
	int msgSize = 128;
	int nBytes = sizeof(char) * msgSize;
	char msg[msgSize];
	int status = TRUE;
	
	while (status) {
		fgets(msg, nBytes, stdin);
		trim(msg);
		if (strcmp("#$%", msg) == 0) {
			printf("Command menu entered. Enter help for options.\n");
			fgets(msg, nBytes, stdin);
			trim(msg);
			status = executeCommand(socket, msg, lock);
		}
		else {
			sem_wait(lock);
			
			sendMessageType(TYPE_MSGGROUP, socket);
			int type;
			recv(socket, &type, sizeof(int), 0);
			
			if (type == TYPE_OK) {
				send(socket, &nBytes, sizeof(nBytes), 0);
				send(socket, msg, nBytes, 0);
				
			}
			
			else {
				printf("Error: You have not chosen your active group.\nPlease choose your active group by typing in #$%% followed by change.\n");
			}
			
			sem_post(lock);
		}
	}
}

int executeCommand(int socket, char *msg, sem_t *lock) {
	if (!strcmp(msg, "logout")) {
		exit_client(socket);
		return FALSE;
	}
	else {
		if (!strcmp(msg, "help")) {
			printf("Available Commands\nhelp: gives this command list\njoin: joins a group. You will be prompted for the group name\nleave: leaves a group. You will be prompted for the group name\nchange: changes the group you are currently sending messages to.\nlogout: exits the client. You will be able to log back in using your password\n");
		}
		
		else {
			sem_wait(lock);
			if (!strcmp(msg, "join")) {
				sendMessageType(TYPE_JOINGROUP, socket);
				handle_join_group(socket);
			}
		
			else if (!strcmp(msg, "leave")) {
				sendMessageType(TYPE_LEAVEGROUP, socket);
				handle_leave_group(socket);
			}
			
			else if (!strcmp(msg, "change")) {
				sendMessageType(TYPE_CHANGEGROUP, socket);
				handle_change_group(socket);
			}
			sem_post(lock);
		}
		printf("-- enter messages:\n");
		return TRUE;
	}
	

}

void handle_join_group(int socket) {
	getGroupNames(socket);
	printf("Enter one of the groups above to join it, or enter a different name to create a new group.\n");
	
	int nBytes = 128;
	char *groupName = malloc(nBytes);
	fgets(groupName, nBytes, stdin);
	trim(groupName);
	
	nBytes = sizeof(char) * (strlen(groupName) + 1);
	send(socket, &nBytes, sizeof(int), 0);
	send(socket, groupName, nBytes, 0);
	
	int type;
	recv(socket, &type, sizeof(int), 0);
	
	if (type == TYPE_FAIL) {
		printf("Error: You are already in that group.\n");
	}
	else if (type == TYPE_CREATEGROUP) {
		char *newGroup = malloc(sizeof(char) * 3);
		printf("%s does not exist yet. Would you like to create it? (y/n): ", groupName);
		fgets(newGroup, sizeof(char) * 3, stdin);
		trim(newGroup);
		
		if (strcmp(newGroup, "y") == 0) {
			sendMessageType(TYPE_OK, socket);
			sendPassword(socket, TRUE);
			printf("%s created and joined.\n", groupName);
		}
		else {
			sendMessageType(TYPE_FAIL, socket);
		}
	}
	
	else if (type == TYPE_OK) {
		sendPassword(socket, FALSE);
		printf("%s joined successfully.\n", groupName);
	}
}

void handle_leave_group(int socket) {
	getGroupNames(socket);
	printf("Enter group to leave.\n");
	
	int nBytes = 128;
	char *groupName = malloc(nBytes);
	fgets(groupName, nBytes, stdin);
	trim(groupName);
	
	nBytes = sizeof(char) * (strlen(groupName) + 1);
	send(socket, &nBytes, sizeof(int), 0);
	send(socket, groupName, nBytes, 0);
	
	int type;
	recv(socket, &type, sizeof(int), 0);
	
	if (type == TYPE_FAIL) {
		printf("Error: You are not in that group.\n");
	}

	else if (type == TYPE_OK) {
		printf("You successfully left %s.\n", groupName);
	}
}

void handle_change_group(int socket) {
	getGroupNames(socket);
	printf("Select the group above that your messages will go to.\n");
	
	int nBytes = 128;
	char *groupName = malloc(nBytes);
	fgets(groupName, nBytes, stdin);
	trim(groupName);
	
	nBytes = sizeof(char) * (strlen(groupName) + 1);
	send(socket, &nBytes, sizeof(int), 0);
	send(socket, groupName, nBytes, 0);
	
	int type;
	recv(socket, &type, sizeof(int), 0);

	if (type == TYPE_OK) {
		printf("Active group changed to %s.\n", groupName);
	}
	
	else {
		printf("Error: You are either not a member of that group or that group doesn't exist.\n");
	}
}

void exit_client(int socket) {
	sendMessageType(TYPE_EXIT, socket);
	close(socket);
}

// Recieves messages from the server
// Implimented by Shane Novitsky
void * recieveThread(void *info) {
   int connect = ((RcvArgs*)info)->socket;
   sem_t *lock = &(((RcvArgs*)info)->lock);
   
   int type = TYPE_GETGROUPMSG;
   int numBytes;
   
   sem_wait(lock);
   
   errno = 0;
   recv(connect, &type, sizeof(int), MSG_DONTWAIT);
   
   while(type != TYPE_EXIT) {
	  if (errno == 0) {
		  if (type == TYPE_GETGROUPMSG) {
			  recv(connect, &numBytes, sizeof(int), 0); // get message size
			  char msg[numBytes];
			  recv(connect, msg, numBytes, 0); // get group message struct
			  printf("%s\n", msg);
		  }
	  }
	  
	  sem_post(lock);
	  
	  sleep(1);
	  
	  sem_wait(lock);
	  
	  errno = 0;
	  recv(connect, &type, sizeof(int), MSG_DONTWAIT);
   }
   sem_post(lock);
   sem_destroy(lock);
}

void sendPassword(int socket, int NEW) {
	int passBytes = 128;
	char password[passBytes];
	int type;
	if (NEW) {
		printf("-- enter password: \n");
		fgets(password, passBytes, stdin);
		trim(password);
	  
		send(socket, &passBytes, sizeof(int), 0);
		send(socket, password, passBytes, 0);
		
		printf("-- confirm password: \n");
        fgets(password, passBytes, stdin);
        trim(password);
		  
		send(socket, &passBytes, sizeof(int), 0);
		send(socket, password, passBytes, 0);
		
		recv(socket, &type, sizeof(int), 0);
		while (type != TYPE_OK){
			printf("Error: Two passwords did not match.\nPlease reenter password:\n");
			fgets(password, passBytes, stdin);
			trim(password);
		  
			send(socket, &passBytes, sizeof(int), 0);
			send(socket, password, passBytes, 0);

			printf("-- confirm password: \n");
			fgets(password, passBytes, stdin);
			trim(password);
		  
			send(socket, &passBytes, sizeof(int), 0);
			send(socket, password, passBytes, 0);
	      
			recv(socket, &type, sizeof(int), 0);
		}
	}
	
	else {
		printf("-- enter password:\n");
		fgets(password, passBytes, stdin);
		trim(password);
  
		// Send password
		send(socket, &passBytes, sizeof(int), 0);
		send(socket, password, passBytes, 0);
	
		recv(socket, &type, sizeof(int), 0);

		while(type != TYPE_OK) { 
			printf("Incorrect try again.\n");
		 
			printf("-- enter password:\n");
			fgets(password, passBytes, stdin);
			trim(password);
      
			// Send password
			send(socket, &passBytes, sizeof(int), 0);
			send(socket, password, passBytes, 0);
        
			recv(socket, &type, sizeof(int), 0);
      }
	}
}

void getActiveUsers(int socket, char *groupName) {
	printf("Users in %s\n", groupName);
	int nBytes;
	char *name;
	recv(socket, &nBytes, sizeof(int), 0);
	while (nBytes != 0) {
		name = malloc(nBytes);
		recv(socket, name, nBytes, 0);
		printf("%s\n", name);
		free(name);
	}
}

void getGroupNames(int socket) {
	printf("Current Groups\n");
	int nBytes;
	char *name;
	recv(socket, &nBytes, sizeof(int), 0);
	while (nBytes != 0) {
		name = malloc(nBytes);
		recv(socket, name, nBytes, 0);
		printf("%s\n", name);
		free(name);
		recv(socket, &nBytes, sizeof(int), 0);
	}
}
