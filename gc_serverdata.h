#define INACTIVE 0
#define ACTIVE 1

#include <semaphore.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <time.h>

typedef struct USER {
   int userId;
   char *name;
   char *password;
   int socket;
   int active;
   struct GROUP *currentGroup;
   struct USER *next;
} User;

typedef struct GROUPUSER {
   User *user;
   struct GROUPUSER *nextUser;
} GroupUser;

typedef struct MESSAGE {
	int id;
	int senderId;
	char *message;
	long time;
	struct MESSAGE *next;
} Message;

typedef struct GROUP {
	int groupId;
	char *name;
	char *password;
	GroupUser *firstUser;
	int nUsers;
	Message *firstMessage;
	Message *lastMessage;
	int nMessages;
	struct GROUP *next;
	sem_t lock;
	sem_t writerLock;
	int nReaders;
} Group;

typedef struct HEADER {
   User *firstUser;
   int nUsers;
   sem_t user_lock;
   sem_t user_writerLock;
   int user_nReaders;
   
   Group *firstGroup;
   int nGroups;
   sem_t group_lock;
   sem_t group_writerLock;
   int group_nReaders;
   
   int fd; //file descriptor
   int serverSocket;

} Header;

typedef struct SRVRARGS {
	int socket;
	Header *header;
} SrvrArgs;

//Struct Creator Methods
User *createUser(int userId, char *username, int socket);
GroupUser *createGroupUser(User *user, GroupUser *nextUser);
Message *createMessage(int id, int senderId, char *message);
Group *createGroup(int groupId, char *name, char *password);
Header *initializeHeader(int serverSocket);
SrvrArgs *initializeSrvrArgs(int socket, Header *header);

//Header Helper methods
void addUser(Header *header, User *user);
void addGroup(Header *header, Group *group);
void addGroupUser(User *user, Group *group);
void addMessage(Group *group, Message *msg);
void deleteGroupUser(User *user, Group *group);
User *findUser(Header *header, char *username);
int hasUser(Group *group, char *username);
Group *findGroup(Header *header, char *groupName);
void printUsers(Header *header);
void printGroupUsers(GroupUser *groupUser);
void printGroupMessages(Message *message);
void printGroups(Header *header, int includeUsers);
void printGroup(Group *group, int includeUsers);
void destroyHeader(Header *header);
void destroyUsers(User *user);
void destroyGroups(Group *group);
void destroyGroupUsers(GroupUser *groupUser);
void destroyMessages(Message *message);

//Communication methods
int getMessageType(int socket);
char *getPassword(char *password, int socket, int NEW);
void sendGroupMessage(User *sender, char *message);
void sendGroupUsers(Group *group, int socket);
void sendGroupNames(Header *header, User *user, int isMember);
void broadcastMessage(Header *header, char *message);
void broadcastExit(Header *header);

Header *readState(Header *header, int fd);
void writeState(Header *header);

//Misc. Helper methods
void reader_lock(sem_t *lock, sem_t *writerLock, int *nReaders);
void reader_unlock(sem_t *lock, sem_t *writerLock, int *nReaders);
char *formatMessage(char *groupName, char *username, char *message);
void trim(char *str);



