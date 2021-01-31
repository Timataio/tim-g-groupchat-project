#define TYPE_USERNAME 1
#define TYPE_MESSAGE 2
#define TYPE_EXIT 99
#define TYPE_REGISTRATION 100
#define TYPE_LOGIN 101
#define TYPE_GROUPNAMES 119
#define TYPE_JOINGROUP 120
#define TYPE_LEAVEGROUP 121
#define TYPE_CHANGEGROUP 122
#define TYPE_MSGGROUP 130
#define TYPE_OK 200
#define TYPE_FAIL 203
#define TYPE_GETGROUPMSG 210
#define TYPE_CREATEGROUP 220

#define TRUE 1
#define FALSE 0

char *getString(int socket);
void sendMessageType(int type, int socket);
void trim(char *str);
/*
// client to server
typedef struct CLIENTUSERNAME{
	int type;				//type = 1
	int nBytes;
	char username[24];
} ClntSendUsernameP;

typedef struct CLIENTMSG {
	int type;				//type = 2
	int nBytes;
	char message[127];
} ClntSendMsgP;


typedef struct CLIENTEXIT {
	int type;				// type = 99
} ClntSendExitP;

typedef struct CLIENTREGISTRATION {
   int type;            	// type = 100
   int nBytesUserName;           // nBytesIUserName = N + 1, N < 24
   char username[24];   	// '\0' is included. 
   int nBytesPassword1;           // nBytesPassword = N + 1, N < 24
   char password1[24];   	// '\0' is included.
   int nBytesPassword2;
   char password2[24];
   int groupID;
} ClntSendRegistrationP;

typedef struct CLIENTLOGIN {
   int type;            	// type = 101
   int nBytesUserName;           // nBytesUserName = N + 1, N < 24
   char username[24];   	// '\0' is included. 
   int nBytesPassword;           // nBytesPassword = N + 1, N < 24
   char password[24];   	// '\0' is included. 
} ClntSendLoginP;

typedef struct CLIENTJOINGROUP {
   int type;            	// type = 120
   int nBytesGName
   char *groupName
   server sends OK if group exists, CREATENEWGROUP if group does not, and FAIL if user is already in group
   Client needs to send OK or FAIL back for a new group
} ClntMsgGroupP;

typedef struct CLIENTLEAVEGROUP {
   int type;            	// type = 121
   int nBytesGName
   char *groupName
   server sends OK if group exists or FAIL if user is not in group
} ClntMsgGroupP;

typedef struct CLIENTMSGGROUP {
   int type;            	// type = 130
   int nBytesGName
   char *groupName
   int nBytesMsg;           	// nBytesMsg = N + 1, N < 127
   char *message;
} ClntMsgGroupP;


typedef struct SERVERGROUPMESSAGE {
   int type;            	// type = 210
   int groupID;
   int nBytesMsg           	// nBytesMsg = N + 1, N < 127
   char message[127]
} SrvrSendGroupMsg;
*/