/*
	Program:	Group Chat Server Version II
	Author:		Tim Gallagher, Luke Prusinski, Alexa Baldon
	Date: 		11/12/2020
	Filename:	gc_server.c
	Compile:	gcc -lpthread -o gc_server gc_server.c gc_serverdata.c server-thread-2020.c
	Run:		./gc_server
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

#include "gc_serverdata.h"
#include "gc_server-thread-2020.h"
#include "gc_shared.h"

#define HOST "freebsd1.cs.scranton.edu"
#define BACKLOG 10

void *subserver(Header *header, User *user, int socket);
void *server(void *args);
void *acceptUser(void *args);
void close_server(Header *header);
void *handle_registration(Header *header, int socket);
void *handle_login(Header *header, int socket);
void *handle_group_join(Header *header, User *user, int socket);
void *handle_group_leave(Header *header, User *user, int socket);
void *handle_group_change(Header *header, User *user, int socket);
void *handle_group_create(Header *header, User *user, char *name, int socket);
void *handle_group_message(Header *header, User *user, int socket);

//Created by Tim G. and Luke P.
int main(int argc, char *argv[]) {
    int http_sock_fd;
    if (argc < 2) {
       printf("Run: host name#\n");
       return 1;
    }
	else if (argc < 3) {
	   printf("Run: prgoram port#\n");
	}
    http_sock_fd = get_server_socket(HOST, argv[1]);

    if (start_server(http_sock_fd, BACKLOG) == -1) {
       printf("start server error\n");
       exit(1);
    }
	
	Header *header = initializeHeader(http_sock_fd);
	SrvrArgs* srvrArgs = initializeSrvrArgs(http_sock_fd, header);
	pthread_t connectionThread;
	
	pthread_create(&connectionThread, NULL, server, (void*)srvrArgs);
	
	printf("reportState works differently now.\nThe server will tell you anything you'd like if you ask for it.\nType in help for a list of commands.\n");
	
	char *command = malloc(sizeof(char) * 20);
	fgets(command, 128, stdin);
	trim(command);
	
	while (strcmp(command, "exit") != 0) {
		if (strcmp(command, "status") == 0) {
			printUsers(header);
			printGroups(header, FALSE);
		}
		
		else if (strcmp(command, "group") == 0) {
			printf("Please enter group name.\n");
			char *groupName = malloc(sizeof(char) * 128);
			fgets(groupName, 128, stdin);
			trim(groupName);
			Group *group = findGroup(header, groupName);
			free(groupName);
			if (group != NULL) {
				printGroup(group, TRUE);
			}
			else {
				printf("Error: Group does not exist.\n");
			}
		}
		
		else if (strcmp(command, "broadcast") == 0) {
			char *message = malloc(sizeof(char) * 128);
			fgets(message, 128, stdin);
			trim(message);
			broadcastMessage(header, message);
		}
		
		else if (strcmp(command, "help") == 0) {
			printf("commands\nhelp: lists the possible commands\nstatus: prints a high level overview of everything, including the list of users, groups, and their messages\ngroup: prints the info about a group, including a list of its users and messages\nbroadcast [msg]: sends msg to all active users\nexit: closes the server\n");
		}
		
		fgets(command, 128, stdin);
		trim(command);
	}
	close_server(header);
	
} 

//Built on Top of Dr. Bi's code by Tim G.
void *subserver(Header *header, User *user, int socket) {
	while (user->active == ACTIVE) {
		
		int type = getMessageType(socket);
		
		if (type == TYPE_EXIT) {
			user->active = INACTIVE;
			//close(user->socket);
			
			if (user->currentGroup != NULL) {
				char *msg = malloc(sizeof(char) * (strlen(user->name) + 19));
				sprintf(msg, "%s has gone offline.", user->name);
				sendGroupMessage(user, msg);
				user->currentGroup = NULL;
				printf("%s\n", msg);
			}
			return NULL;
		}
		
		else if (type == TYPE_MSGGROUP) {
			handle_group_message(header, user, socket);
		}
		
		else if (type == TYPE_JOINGROUP) {
			handle_group_join(header, user, socket);
		}
		
		else if (type == TYPE_LEAVEGROUP) {
			handle_group_leave(header, user, socket);
		}
		
		else if (type == TYPE_CHANGEGROUP) {
			handle_group_change(header, user, socket);
		}

		sleep(2);
	}
}

//Implemented by Luke P.
//Debugged by Tim G.
void *server(void * args) {
   SrvrArgs *srvrArgs = (SrvrArgs*)args;
   int socket = srvrArgs->socket;
   Header *header = srvrArgs->header;
   while (1) {
      // accept new connection
      int clientSocket = accept_client(socket);
	  srvrArgs->socket = clientSocket;
	  if (clientSocket >= 0) {
		  pthread_t subSer;
		  pthread_create(&subSer, NULL, acceptUser, (void*)srvrArgs);
	  }
	  else {
		  sleep(1);
	  }
   }
}

//Implemented by Tim G. & Luke P.
void *acceptUser(void *args) {
    int socket = ((SrvrArgs*)args)->socket;
	Header *header = ((SrvrArgs*)args)->header;
	
	User *user;
	int type = getMessageType(socket);
	if (type == TYPE_REGISTRATION) {
		user = handle_registration(header, socket);
	}
	
	else if (type == TYPE_LOGIN) {
		user = handle_login(header, socket);
	}
	
	if (user != NULL) {
		printf("User %s accepted.\n", user->name);
		subserver(header, user, socket);
	}
	else {
		printf("User used wrong account.\n");
	}
	
	return NULL;
}

void close_server(Header *header) {
	//Broadcast message to all active users that server is closing
	char *message = malloc(sizeof(char)*25);
	message = "Server is shutting down.";
	broadcastMessage(header, message);
	broadcastExit(header);
	//writeState(header);
	
	destroyHeader(header);
	
}

// Implemented by Alexa Baldon
void *handle_registration(Header *header, int socket) {
	int nBytes;
	recv(socket, &nBytes, sizeof(int), 0);
	char *username = malloc(nBytes);
	recv(socket, username, nBytes, 0);
	
	//Checks if user already exists
	//Handle error
	if (findUser(header, username) != NULL) {
		int type = TYPE_FAIL;
		send(socket, &type, sizeof(type), 0); 
		return NULL;
	} 
	// Successful
	int type = TYPE_OK;
	send(socket, &type, sizeof(type), 0);
	User *user;
	user = createUser(header->nUsers + 1, username, socket);
	addUser(header, user);

	printf("%s is a new user: Getting password...\n", username);
	
	//Get password from user twice to verify that it is entered correctly
	//loop until the user enters the same password twice
	
	char *password = getPassword("", socket, TRUE);
	user->password = password;

	//figure out which group to join
	printf("%s has created a password: Now choosing a group...\n", username);
	
	handle_group_join(header, user, socket);
	user->active = ACTIVE;

	return user;
}

// Implemented by Alexa Baldon
void *handle_login(Header *header, int socket) {
	//Get numBytes for username
	//Get username
	int nBytes;
	recv(socket, &nBytes, sizeof(int), 0);
	char *username = malloc(nBytes);
	recv(socket, username, nBytes, 0);
	
	//Checks if user already exists
	User *user = findUser(header, username);
	
	int type;
	if (user == NULL) { //Handle error
		type = TYPE_FAIL;
		send(socket, &type, sizeof(type), 0);
		return NULL;
	} 
	
	type = TYPE_OK;
	send(socket, &type, sizeof(type), 0);

	// Get existing User from header, ask for password
	printf("%s is an existing user: Getting password...\n", username);
	
	getPassword(user->password, socket, FALSE); 
	
	// Set user to active
	user->active = ACTIVE;
	user->socket = socket;

	return user;
}

void *handle_group_join(Header *header, User *user, int socket) {
	sendGroupNames(header, user, FALSE);
	//Receives group name from user
	char *groupName = getString(socket);
	
	Group *group = findGroup(header, groupName);
	if (group == NULL) {
		handle_group_create(header, user, groupName, socket);
	}
	
	else {
		int type;
		if (hasUser(group, user->name)) {
			type = TYPE_FAIL;
			send(socket, &type, sizeof(int), 0);
			
		}
		else {
			type = TYPE_OK;
			send(socket, &type, sizeof(int), 0);
			getPassword(group->password, socket, FALSE);
			addGroupUser(user, group);
			printf("%s joined group %s.\n", user->name, group->name);
		}	
	}	
	return NULL;
}

void *handle_group_leave(Header *header, User *user, int socket) {
	sendGroupNames(header, user, TRUE);
	
	//Receives group name from user
	char *groupName = getString(socket);
	
	Group *group = findGroup(header, groupName);
	if (group == NULL || !hasUser(group, user->name)) {
		sendMessageType(TYPE_FAIL, socket);
	}
	
	else {
		deleteGroupUser(user, group);
		if (user->currentGroup = group) {
			user->currentGroup = NULL;
		}
		printf("%s left group %s.\n", user->name, group->name);
		sendMessageType(TYPE_OK, socket);
	}
	
	return NULL;
}

void *handle_group_change(Header *header, User *user, int socket) {
	sendGroupNames(header, user, TRUE);
	
	char *groupName = getString(socket);
	Group *group = findGroup(header, groupName);
	
	if (group == NULL || !hasUser(group, user->name)) {
		sendMessageType(TYPE_FAIL, socket);
	}
	else {
		user->currentGroup = group;
		printf("%s's active group is now %s.\n", user->name, group->name);
		sendMessageType(TYPE_OK, socket);
	}
	return NULL;
}

void *handle_group_create(Header *header, User *user, char *name, int socket) {
	int type = TYPE_CREATEGROUP;
	send(socket, &type, sizeof(int), 0);
	
	recv(socket, &type, sizeof(int), 0);
	if (type == TYPE_OK) {
		char *password = getPassword("", socket, TRUE);
		Group *group = createGroup(header->nGroups + 1, name, password);
		addGroup(header, group);
		addGroupUser(user, group);
		printf("%s created and joined group %s.\n", user->name, group->name);
	}
	
	else {
		printf("Incorrect type recieved. Type %d received.\n", type);
	}
}

void *handle_group_message(Header *header, User *user, int socket) {	
	if (user->currentGroup == NULL) {
		sendMessageType(TYPE_FAIL, socket);
	}
	
	else {
		sendMessageType(TYPE_OK, socket);
		char *messageContent = getString(socket);		
		char *formattedMessage = formatMessage(user->currentGroup->name, user->name, messageContent);
		free(messageContent);
		
		printf("New Message Received: %s\n", formattedMessage);
		
		Message *message = createMessage(user->currentGroup->nMessages + 1, user->userId, formattedMessage);
		addMessage(user->currentGroup, message);
		sendGroupMessage(user, formattedMessage);
	}
}

