#include "gc_serverdata.h"
#include "gc_shared.h"
#include <semaphore.h>
#include <stdio.h>

#define TRUE 1
#define FALSE 0

// Implemented by Tim G.
User *createUser(int userId, char *username, int socket) {
	User *user = malloc(sizeof(User));
	
	user->userId = userId;
	user->name = username;
	user->socket = socket;
	user->active = INACTIVE;
	user->currentGroup = NULL;
	user->next = NULL;
	
	return user;
}

//Implemented by Luke P.
GroupUser *createGroupUser(User *user, GroupUser *nextUser) {
	GroupUser *gUser = malloc(sizeof(GroupUser));
        
	gUser->user = user;
	gUser->nextUser = nextUser;

	return gUser;
}

//Implemented by Tim G.
Message *createMessage(int id, int senderId, char *message) {
	Message *msg = malloc(sizeof(Message));
	
	long currentTime = 0;
	time(&currentTime);
	
	msg->id = id;
	msg->senderId = senderId;
	msg->message = message;
	msg->time = currentTime;
	msg->next = NULL;
	
	return msg;
}

//Implemented by Luke P.
Group *createGroup(int groupId, char *name, char *password) {
	Group *group = malloc(sizeof(Group));
	
	group->firstUser = NULL;
	group->groupId = groupId;
	group->name = name;
	group->password = password;
	group->nMessages = 0;
	group->next = NULL;
	
	sem_init(&(group->lock), 0, 1);
	sem_init(&(group->writerLock), 0, 1);
	group->nReaders = 0;
	
	return group;
}

//Implemented by Tim G.
Header *initializeHeader(int serverSocket) {
	Header *header = malloc(sizeof(header));
	
	header->firstUser = NULL;
	header->nUsers = 0;
	sem_init(&(header->user_lock), 0, 1);
	sem_init(&(header->user_writerLock), 0, 1);
	header->user_nReaders = 0;
	
	header->firstGroup = NULL;
	header->nGroups = 0;
	sem_init(&(header->group_lock), 0, 1);
	sem_init(&(header->group_writerLock), 0, 1);
	header->group_nReaders = 0;
	
	header->serverSocket = serverSocket;
	
	return header;
}

//Implemented by Tim G.
SrvrArgs *initializeSrvrArgs(int socket, Header *header) {
	SrvrArgs* srvrArgs = malloc(sizeof(SrvrArgs));
	srvrArgs->socket = socket;
	srvrArgs->header = header;
	return srvrArgs;
}

//Implemented by Tim G.
void addUser(Header *header, User *user) {
	sem_wait(&(header->user_writerLock));
	
	user->next = header->firstUser;
	header->firstUser = user;
	header->nUsers++;
	
	sem_post(&(header->user_writerLock));
}

//Implemented by Luke P.
void addGroup(Header *header, Group *group) {
	sem_wait(&(header->group_writerLock));

	group->next = header->firstGroup;
	header->firstGroup = group;
	header->nGroups++;
	
	sem_post(&(header->group_writerLock));
}

//Implemented by Luke P.
void addGroupUser(User *user, Group *group) {
	GroupUser *groupUser = malloc(sizeof(GroupUser));
	groupUser->user = user;
	
	sem_wait(&(group->writerLock));

	groupUser->nextUser = group->firstUser;
	group->firstUser = groupUser;
	group->nUsers++;

	sem_post(&(group->writerLock));
}

//Implemented by Tim G.
void addMessage(Group *group, Message *msg) {
	sem_wait(&(group->writerLock));
	
	if (group->nMessages == 0) {
		group->firstMessage = msg;
		group->lastMessage = msg;
	}
	
	else {
		Message *lastMessage = group->lastMessage;
		lastMessage->next = msg;
		group->lastMessage = msg;
	}
	group->nMessages++;
	
	sem_post(&(group->writerLock));
}

void deleteGroupUser(User *user, Group *group) {
	sem_wait(&(group->writerLock));
	
	if (group->nUsers == 1) {
		free(group->firstUser);
		group->firstUser = NULL;
	}
	
	else {
		GroupUser *groupUser = group->firstUser;
		while (groupUser->nextUser->user->userId != user->userId) {
			groupUser = groupUser->nextUser;
		}
		GroupUser *nextUser = groupUser->nextUser->nextUser;
		free(groupUser->nextUser);
		groupUser->nextUser = nextUser;
	}
	group->nUsers--;
	
	sem_post(&(group->writerLock));
}

//Implemented by Luke P.
void printGroupMessages(Message *message) {
	Message *tmp = message;
	while(tmp != NULL) {
		printf("%s\n", tmp->message);
		tmp = tmp->next;
	}
}
		
//Implemented by Luke P.
void printUsers(Header *header) {
	reader_lock(&(header->user_lock), &(header->user_writerLock), &(header->user_nReaders));
	
	User *user = header->firstUser;
	printf("Num Users: %d\n", header->nUsers);
	printf("List of Users\n");
	while (user != NULL) {
		if(user->active == 0){
		   printf("%s [INACTIVE]\n", user->name);
	        }else {
		   printf("%s [ACTIVE]\n", user->name);
		}
		user = user->next;
	}
	
	reader_unlock(&(header->user_lock), &(header->user_writerLock), &(header->user_nReaders));
	
	printf("\n");
}

//Implemented by Luke P.
void printGroupUsers(GroupUser *groupUser) {
	printf("List of Users\n");
	GroupUser *tmp = groupUser;
	while(tmp != NULL) {
		printf("%s\n", tmp->user->name);
		tmp = tmp->nextUser;
	}
	printf("\n");
}

void printGroups(Header *header, int includeUsers) {
	reader_lock(&(header->group_lock), &(header->group_writerLock), &(header->group_nReaders));
	
	Group *group = header->firstGroup;
	while (group != NULL) {
		printGroup(group, includeUsers);
		group = group->next;
	}
	
	reader_unlock(&(header->group_lock), &(header->group_writerLock), &(header->group_nReaders));
}

//Implemented by Luke P.
void printGroup(Group *group, int includeUsers) {
	reader_lock(&(group->lock), &(group->writerLock), &(group->nReaders));
	
	printf("Group Name: %s\n", group->name);
	printf("Number of Users: %d\n", group->nUsers);
	if(includeUsers) {
		printGroupUsers(group->firstUser);
	}
	printf("Messages\n");
	printGroupMessages(group->firstMessage);
	
	reader_unlock(&(group->lock), &(group->writerLock), &(group->nReaders));
}

//Implemented by Luke P.
void destroyHeader(Header *header) {
	destroyUsers(header->firstUser);
	destroyGroups(header->firstGroup);
	sem_destroy(&(header->user_lock));
	sem_destroy(&(header->user_writerLock));
	sem_destroy(&(header->group_writerLock));
	sem_destroy(&(header->group_writerLock));
	free(header);	
}

//Implemented by Luke P.
void destroyUsers(User *user) {
	while(user != NULL) {
		User *tmp = user->next;
		//close(user->socket);
		free(user);
		user = tmp;
	}
}	

//Implemented by Luke P.
void destroyGroups(Group *group) {
	while(group != NULL){
		Group *tmp = group->next;
		free(group->name);
		free(group->password);
		destroyGroupUsers(group->firstUser);
		destroyMessages(group->firstMessage);
		sem_destroy(&(group->lock));
		sem_destroy(&(group->writerLock));
		free(group);
		group = tmp;
	}
}

//Implemented by Luke P.
void destroyGroupUsers(GroupUser *groupUser) {
	while(groupUser != NULL) {
		GroupUser *tmp = groupUser->nextUser;
		free(groupUser);
		groupUser = tmp;
	}
}

//Implemented by Luke P.
void destroyMessages(Message *message) {
	while(message != NULL) {
		Message *tmp = message->next;
		free(message);
		message = tmp;	
	}
}

Group *findGroup(Header *header, char *groupName) {
	reader_lock(&(header->group_lock), &(header->group_writerLock), &(header->group_nReaders));
	
	Group *group = header->firstGroup;
	while (group != NULL && (strcmp(groupName, group->name) != 0)) {
		group = group->next;
	}
	
	reader_unlock(&(header->group_lock), &(header->group_writerLock), &(header->group_nReaders));
	return group;
}

//Implemented by Luke P.
User *findUser(Header *header, char *username) {
	reader_lock(&(header->user_lock), &(header->user_writerLock), &(header->user_nReaders));

	User *user = header->firstUser;
	while(user != NULL && (strcmp(username, user->name) != 0)) {
		user = user->next;
	}
	
	reader_unlock(&(header->user_lock), &(header->user_writerLock), &(header->user_nReaders));

	return user;
}

int hasUser(Group *group, char *username) {
	reader_lock(&(group->lock), &(group->writerLock), &(group->nReaders));
	
	GroupUser *groupUser = group->firstUser;
	User *user;
	int retVal = FALSE;
	while (retVal == FALSE && groupUser != NULL) {
		user = groupUser->user;
		if (strcmp(user->name, username) == 0) {
			retVal = TRUE;
		}
		groupUser = groupUser->nextUser;
	}
	
	reader_unlock(&(group->lock), &(group->writerLock), &(group->nReaders));
	
	return retVal;
}

int getMessageType(int socket) {
	int type;
	recv(socket, &type, sizeof(int), 0);
	return type;
}

char *getPassword(char *password, int socket, int NEW) {
	if (NEW) {
		char *newPassword1 = getString(socket);
		char *newPassword2 = getString(socket);

		while (strcmp(newPassword1, newPassword2) != 0){
			sendMessageType(TYPE_FAIL, socket);
			
			free(newPassword1);			
			free(newPassword2);
			
			newPassword1 = getString(socket);
			newPassword2 = getString(socket);	
		}
		sendMessageType(TYPE_OK, socket);
		return newPassword1;
	}
	
	else {
		char *newPassword = getString(socket);
		while (strcmp(password, newPassword) != 0) {
			sendMessageType(TYPE_FAIL, socket);
			
			free(newPassword);
			getString(socket);
		}
		sendMessageType(TYPE_OK, socket);
		return newPassword;
	}
}

void sendGroupMessage(User *sender, char *message) {
	int type = TYPE_GETGROUPMSG;
	int nBytes = sizeof(char) * (strlen(message) + 1);
	
	Group *group = sender->currentGroup;
	
	reader_lock(&(group->lock), &(group->writerLock), &(group->nReaders));
	
	GroupUser *groupUser = sender->currentGroup->firstUser;
	User *user; 
	while (groupUser != NULL) {
		user = groupUser->user;
		
		if (user->active && !(strcmp(user->name, sender->name) == 0)) {
			send(user->socket, &type, sizeof(int), 0);
			send(user->socket, &nBytes, sizeof(int), 0);
			send(user->socket, message, nBytes, 0);
		}
		groupUser = groupUser->nextUser;
	}
	
	reader_unlock(&(group->lock), &(group->writerLock), &(group->nReaders));
}

//Implemented by Luke P.
void broadcastMessage(Header *header, char *message) {
	int type = TYPE_GETGROUPMSG;
	int nBytes = sizeof(char) * (strlen(message) + 1);
	
	reader_lock(&(header->user_lock), &(header->user_writerLock), &(header->user_nReaders));
	
	User *tmp = header->firstUser;
	while (tmp != NULL) {
		send(tmp->socket, &type, sizeof(int), 0);
		send(tmp->socket, &nBytes, sizeof(int), 0);
		send(tmp->socket, message, nBytes, 0);
		tmp = tmp->next;
	}
	
	reader_unlock(&(header->user_lock), &(header->user_writerLock), &(header->user_nReaders));
}

void broadcastExit(Header *header) {
	int type = TYPE_EXIT;

	User *tmp = header->firstUser;
	while (tmp != NULL) {
		send(tmp->socket, &type, sizeof(int), 0);
		tmp = tmp->next;
	}
}

void sendGroupUsers(Group *group, int socket) {
	GroupUser *groupUser = group->firstUser;
	int nBytes;
	while (groupUser != NULL) {
		nBytes = sizeof(char) * (strlen(groupUser->user->name) + 1);
		send(socket, &nBytes, sizeof(int), 0);
		send(socket, groupUser->user->name, nBytes, 0);
		groupUser = groupUser->nextUser;
	}

	nBytes = 0;
	send(socket, &nBytes, sizeof(int), 0);
}

void sendGroupNames(Header *header, User *user, int isMember) {
	int socket = user->socket;
	Group *group = header->firstGroup;
	int nBytes;
	while (group != NULL) {
		if (user == NULL) {
			printf("User is NULL.\n");
		}
		else if (!isMember || hasUser(group, user->name)) {
			nBytes = sizeof(char) * (strlen(group->name) + 1);
			send(socket, &nBytes, sizeof(int), 0);
			send(socket, group->name, nBytes, 0);
		}
		group = group->next;
	}
	
	nBytes = 0;
	send(socket, &nBytes, sizeof(int), 0);
}

// Implemented by Alexa Baldon
void writeState(Header *header){
	FILE *userlist;
	FILE *grouplist;
	FILE *usergroup;
	
	//open or create 3 csv files
	//userlist
	userlist = fopen("gc_userlist.csv", "w+");
	//grouplist
	grouplist = fopen("gc_grouplist.csv", "w+");
	//user-group
	usergroup = fopen("gc_usergroup.csv", "w+");

	User *user = header->firstUser;
	Group *group = header->firstGroup;

	// database schema
		// userlist (userId, name, password)
		// grouplist (groupId, name, password)
		// usergroup (group.name, user.name)

	while (user != NULL){
		fprintf(userlist, "%d, %s, %s",
				user->userId,
				user->name,
				user->password);
		user = user->next;
		fprintf(userlist, "\n");
	}

	while (group != NULL){
		fprintf(grouplist, "%d, %s, %s",
				group->groupId,
				group->name,
				group->password);
		group = group->next;
		fprintf(grouplist, "\n");
	}

	group = header->firstGroup;
	while (group != NULL){
		fprintf(usergroup, "%d", group->groupId);
		GroupUser *guTemp = group->firstUser;
		while (guTemp != NULL){
			fprintf(usergroup, "%s", guTemp->user->name);
		}
		fprintf(usergroup, "\n");
	}

	//close all files
	fclose(userlist);
	fclose(grouplist);
	fclose(usergroup);
}

void getCell(char *buff){
	char *token = strtok(buff, ",");
	while (token != NULL){
		token = strtok(NULL, ",");
	}
}

// Implemented by Alexa Baldon
Header *readState(Header *header, int fd) {
	/*
	FILE *userlist;
	FILE *grouplist;
	FILE *usergroup;
	
	//open or create 3 csv files
	//userlist
	userlist = fopen("gc_userlist.csv", "r");
	//grouplist
	grouplist = fopen("gc_grouplist.csv", "r");
	//user-group
	usergroup = fopen("gc_usergroup.csv", "r");

	//load groups into header
	while(!feof(grouplist)){
		char *buff;
		fscanf(grouplist, "%s", buff);
		char groupArray[6][128];
		int i;
		i=0;
		while (buff != NULL){
			char *str;
			str = getCell(buff);
			strcpy(a[i], str);
			i++;
		}
		Group *newGroup = createGroup(groupArray[0], groupArray[1], groupArray[2]);
		newGroup->nUsers = groupArray[3];
		addGroup(header, newGroup);
	}

	//load users into header
	while(!feof(userlist)){
		char *buff;
		fscanf(userlist, "%s", buff);
		char userArray[6][128];
		int i;
		i=0;
		while (buff != NULL){
			char *str;
			str = getCell(buff);
			strcpy(a[i], str);
			i++;
		}
		User *newUser = createUser(userArray[0],userArray[1],userArray[3]);
		newUser->password = userArray[2];
		newUser->active = 0;
		User *newUser = addUser(header, newUser);
	}

	//load relationships into header
	while(!feof(usergroup)){
		char *buff;
		char *thisUser;
		fscanf(usergroup, "%s", buff);
		int i;
		i=0;
		while (buff != NULL){
			char *str;
			str = getCell(buff);
			if (i=0){
				thisUser = str;
			}
			else{
				addGroupUser(findUser(header,thisUser),findGroup(header,str));
			}
			i++;
		}
	}

	//close all files
	fclose(userlist);
	fclose(grouplist);
	fclose(usergroup);
	*/
	return NULL;
}

char *formatMessage(char *groupName, char *username, char *message) {
	char *msgText = malloc(sizeof(char) * (strlen(groupName) + strlen(username) + strlen(message) + 7));
	sprintf(msgText, "[%s](%s): %s", groupName, username, message);
	return msgText;
}

void reader_lock(sem_t *lock, sem_t *writerLock, int *nReaders) {
	sem_wait(lock);
	*nReaders++;
	if (*nReaders == 1) {
		sem_wait(writerLock);
	}
	sem_post(lock);
}

void reader_unlock(sem_t *lock, sem_t *writerLock, int *nReaders) {
	sem_wait(lock);
	*nReaders--;
	if (*nReaders == 0) {
		sem_post(writerLock);
	}
	sem_post(lock);
}