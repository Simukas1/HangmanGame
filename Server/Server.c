#include <stdio.h>
#include <Ws2tcpip.h>
#include <winsock2.h>

#define BUFFLEN 1024
#define MAXCLIENTS 10
#define USERLIVES 10

#pragma comment(lib, "Ws2_32.lib")

typedef struct GameInfo {
	BOOLEAN isConnected;
	BOOLEAN isHangman;
	char* guessingWord;
	int opponent;
	int lives;
} GameInfo;

WSADATA Data;
GameInfo gamesInfo[MAXCLIENTS];
fd_set read_set;
SOCKET l_socket = INVALID_SOCKET;
SOCKET c_sockets[MAXCLIENTS];
struct sockaddr_in servaddr;
struct sockaddr_in clientaddr;
char buffer[BUFFLEN];
unsigned int port = 12345;
unsigned int clientaddrlen;

void tryWSAStartup();
void validatePort();
void createSocket(int af, int type, int protocol);
void fillServerAddress(u_long sin_addr, u_short sin_port);
void bindSocket();
void listenSocket();
SOCKET findemptyuser(SOCKET c_sockets[]);
void setGameInfo(int index, char* guessingWord, BOOLEAN isConnected, BOOLEAN isHangman, int opponent);
void sendMessage(SOCKET socket, int flags);
void cleanup(char* message);

int main(int argc, char* argv[])
{
	printf("Starting up the server...\n");
	tryWSAStartup();

	for (int i = 0; i < MAXCLIENTS; ++i) {
		setGameInfo(i, NULL, FALSE, FALSE, -1);
	}

	//if (argc != 2) {
	//	printf("ERROR: expected 2 arguments, got: %d", argc);
	//	return -1;
	//}

	//port = atoi(argv[1]);
	validatePort();
	createSocket(AF_INET, SOCK_STREAM, 0);
	fillServerAddress(htonl(INADDR_ANY), htons(port));
	bindSocket();
	listenSocket();

	for (int i = 0; i < MAXCLIENTS; ++i) {
		c_sockets[i] = -1;
	}

	SOCKET maxfd = 0;

	while (1) {
		FD_ZERO(&read_set);
		for (int i = 0; i < MAXCLIENTS; ++i) {
			if (c_sockets[i] != INVALID_SOCKET) {
				FD_SET(c_sockets[i], &read_set);
				if (c_sockets[i] > maxfd) {
					maxfd = c_sockets[i];
				}
			}
		}

		FD_SET(l_socket, &read_set);
		if (l_socket > maxfd) {
			maxfd = l_socket;
		}

		select((SOCKET)(maxfd + 1), &read_set, NULL, NULL, NULL);

		if (FD_ISSET(l_socket, &read_set)) {
			SOCKET client_id = findemptyuser(c_sockets);
			if (client_id != INVALID_SOCKET) {
				clientaddrlen = sizeof(clientaddr);
				memset(&clientaddr, 0, clientaddrlen);
				c_sockets[client_id] = accept(l_socket, (struct sockaddr*)&clientaddr, &clientaddrlen);
				printf("Client %llu connected from %s:%d\n", client_id, inet_ntop(AF_INET, &clientaddr, buffer, BUFFLEN), ntohs(clientaddr.sin_port));
				snprintf(buffer, sizeof(buffer), "Hello, you are connected. Your Id is: %llu.\nIvesk savo oponento id, arba -1, jeigu tu jo lauki", client_id);
				sendMessage(c_sockets[client_id], 0);
			}
		}

		for (int i = 0; i < MAXCLIENTS; ++i) {
			if (c_sockets[i] != INVALID_SOCKET) {
				if (FD_ISSET(c_sockets[i], &read_set)) {
					memset(&buffer, 0, BUFFLEN);
					int r_len = recv(c_sockets[i], buffer, BUFFLEN, 0);
					if (!strcmp("-1\n", buffer)) {
						snprintf(buffer, sizeof(buffer), "-Waiting for your friend...");
						sendMessage(c_sockets[i], 0);
						break;
					}
					for (int j = 0; j < MAXCLIENTS; ++j) {
						if ((isdigit(buffer[0]) || isdigit(1)) && atoi(buffer) == j) {
							if (gamesInfo[i].isConnected == FALSE) {
								snprintf(buffer, sizeof(buffer), "-You have connected to your friend. Please wait, while he Enters the starting word\n");
								setGameInfo(i, NULL, TRUE, TRUE, j);
								setGameInfo(j, NULL, TRUE, FALSE, i);
								sendMessage(c_sockets[i], 0);
								snprintf(buffer, sizeof(buffer), "Your friend is connected. You can start playing. Enter the starting word please:");
								sendMessage(c_sockets[j], 0);
							}
							else {
								snprintf(buffer, sizeof(buffer), "Opps, your friend is allready playing with someone.\n");
								sendMessage(c_sockets[j], 0);
							}
						}
						else if (gamesInfo[i].opponent == j) {
							if (gamesInfo[i].isHangman == TRUE) {
								if (gamesInfo[i].guessingWord == NULL) {
									int sz = 0;
									for (; buffer[sz] != '\n'; sz++);
									gamesInfo[i].guessingWord = (char*)malloc(sizeof(char) * sz);
									strcpy(gamesInfo[i].guessingWord, buffer);
									gamesInfo[j].guessingWord = (char*)malloc(sizeof(char) * (sz + 1));
									memset(gamesInfo[j].guessingWord, '.', sz);
									gamesInfo[j].guessingWord[sz] = '\0';
									gamesInfo[i].guessingWord[sz] = '\0';
									snprintf(buffer, sizeof(buffer), "Game started. Your word is: %s", gamesInfo[j].guessingWord);
								}
							}
							else {
								BOOL rado = FALSE;
								for (int k = 0; k < strlen(gamesInfo[j].guessingWord); ++k) {
									if (buffer[0] == gamesInfo[j].guessingWord[k]) {
										gamesInfo[i].guessingWord[k] = buffer[0];
										rado = TRUE;
									}
								}
								if (!rado) gamesInfo[i].lives--;
								if (!gamesInfo[i].lives) {
									snprintf(buffer, sizeof(buffer), "-You won!");
									int w_len = send(c_sockets[j], buffer, sizeof(buffer), 0);
									closesocket(c_sockets[j]);
									c_sockets[j] = -1;
									snprintf(buffer, sizeof(buffer), "Oops, you lost. The word was: %s", gamesInfo[j].guessingWord);
									w_len = send(c_sockets[i], buffer, sizeof(buffer), 0);
									closesocket(c_sockets[i]);
									c_sockets[j] = -1;
									gamesInfo[i].guessingWord = NULL;
									gamesInfo[i].isConnected = FALSE;
									gamesInfo[i].isHangman = FALSE;
									gamesInfo[i].lives = 10;
									gamesInfo[i].opponent = -1;
									gamesInfo[j].guessingWord = NULL;
									gamesInfo[j].isConnected = FALSE;
									gamesInfo[j].isHangman = FALSE;
									gamesInfo[j].lives = 10;
									gamesInfo[j].opponent = -1;
									break;
								}
								if (!strcmp(gamesInfo[i].guessingWord, gamesInfo[j].guessingWord)) {
									snprintf(buffer, sizeof(buffer), "-Oops, you lost. Your friend answered the word!");
									int w_len = send(c_sockets[j], buffer, sizeof(buffer), 0);
									closesocket(c_sockets[j]);
									c_sockets[j] = -1;
									snprintf(buffer, sizeof(buffer), "-Congratulations! you won! The word was: %s", gamesInfo[j].guessingWord);
									w_len = send(c_sockets[i], buffer, sizeof(buffer), 0);
									closesocket(c_sockets[i]);
									c_sockets[i] = -1;
									gamesInfo[i].guessingWord = NULL;
									gamesInfo[i].isConnected = FALSE;
									gamesInfo[i].isHangman = FALSE;
									gamesInfo[i].lives = 10;
									gamesInfo[i].opponent = -1;
									gamesInfo[j].guessingWord = NULL;
									gamesInfo[j].isConnected = FALSE;
									gamesInfo[j].isHangman = FALSE;
									gamesInfo[j].lives = 10;
									gamesInfo[j].opponent = -1;
									break;
								}
								snprintf(buffer, sizeof(buffer), "-Your friend guessed: %c and he has %d lives", buffer[0], gamesInfo[i].lives);
								int w_len = send(c_sockets[j], buffer, sizeof(buffer), 0);
								if (w_len <= 0) {
									closesocket(c_sockets[j]);
									c_sockets[j] = -1;
								}
								snprintf(buffer, sizeof(buffer), "Your word now is: '%s' and you have %d lives!", gamesInfo[i].guessingWord, gamesInfo[i].lives);
								w_len = send(c_sockets[i], buffer, sizeof(buffer), 0);
								if (w_len <= 0) {
									closesocket(c_sockets[i]);
									c_sockets[i] = -1;
								}
							}
						}
					}
				}
			}
		}
	}
	WSACleanup();
	return 0;
}

void tryWSAStartup() {
	/*
	* If we wan't to use Winsock DLL, we need to initiate it.
	*/
	int WSAStartupErrorType = WSAStartup(MAKEWORD(2, 2), &Data);
	if (WSAStartupErrorType) {
		cleanup("ERROR: WSAStartup failed\n");
	}
}

void validatePort() {
	if (port < 1 || port > 65535) {
		cleanup("ERROR: invalid port specified. Expected number in (1, 65535).");
	}
	printf("Port is %i\n", port);
}

/*
* af - address family (we should use IPv4)
* SOCK_STREAM - Socket type (We should use Transmission Control Protocol)
* protocol - the protocol to be used (we should set it to 0)
*/
void createSocket(int af, int type, int protocol) {
	if ((l_socket = socket(af, type, protocol)) == INVALID_SOCKET) {
		cleanup("ERROR: cannot create listening socket.");
	}
	printf("Created socket successfully.\n");
}

void fillServerAddress(u_long sin_addr, u_short sin_port) {
	memset(&servaddr, 0, sizeof(servaddr));
	servaddr.sin_family = AF_INET;
	servaddr.sin_addr.s_addr = sin_addr;
	servaddr.sin_port = sin_port;
}

void bindSocket() {
	if (bind(l_socket, (struct sockaddr*)&servaddr, sizeof(servaddr)) < 0) {
		cleanup("ERROR: cannot bind listening socket.");
	}
	printf("Binded socket successfully.\n");
}

void listenSocket() {
	if (listen(l_socket, 10) < 0) {
		cleanup("ERROR: cannot listen on socket.");
	}
	printf("Listening on socket successfully.\n");
}

SOCKET findemptyuser(SOCKET c_sockets[]) {
	for (int i = 0; i < MAXCLIENTS; i++) {
		if (c_sockets[i] == -1) {
			return i;
		}
	}
	return -1;
}

void setGameInfo(int index, char* guessingWord, BOOLEAN isConnected, BOOLEAN isHangman, int opponent) {
	if (gamesInfo[index].guessingWord == NULL) {
		if (guessingWord == NULL)
			gamesInfo[index].guessingWord = NULL;
		else {
			memset(&gamesInfo[index].guessingWord, strlen(guessingWord), sizeof(char));
			strcpy(gamesInfo[index].guessingWord, guessingWord);
		}
	}
	else
		strcpy(gamesInfo[index].guessingWord, guessingWord);
	gamesInfo[index].isConnected = isConnected;
	gamesInfo[index].isHangman = isHangman;
	gamesInfo[index].lives = USERLIVES;
	gamesInfo[index].opponent = opponent;
}

void sendMessage(SOCKET socket, int flags) {
	int w_len = send(socket, buffer, sizeof(buffer), flags);
	if (w_len <= 0) {
		closesocket(socket);
		socket = INVALID_SOCKET;
	}
}

void cleanup(char* message) {
	printf("%s\n", message);
	WSACleanup();
	exit(-1);
}