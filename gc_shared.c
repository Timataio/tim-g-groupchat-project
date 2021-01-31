#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>

char *getString(int socket) {
	int nBytes;
	recv(socket, &nBytes, sizeof(int), 0);
	char *str = malloc(nBytes);
	recv(socket, str, nBytes, 0);
	return str;
}

void sendMessageType(int type, int socket) {
	send(socket, &type, sizeof(int), 0);
}

void trim(char *str) {
	int i = 0;
	char c = str[0];
	while (c != '\0') {
		i++;
		c = str[i];
	}
	while ((c == '\n') || (c == ' ') || (c == '\t') || (c == '\0')) {
		str[i] = '\0';
		i--;
		c = str[i];
	}
}