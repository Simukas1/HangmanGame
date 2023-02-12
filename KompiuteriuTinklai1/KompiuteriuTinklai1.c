#include <stdio.h>
#include <Ws2tcpip.h>
#include <winsock2.h> // fd_set

#define BUFFLEN 1024
#define MAXCLIENTS 10
#pragma comment(lib, "Ws2_32.lib")

int findemptyuser(int c_sockets[]);

int main(int argc, char* argv[])
{
	printf("Starting up the server...\n");
	WSADATA Data;
	int WSAStartupErrorType = WSAStartup(MAKEWORD(2, 2), &Data);
	if (WSAStartupErrorType) {
		printf("ERROR: WSAStartup failed with error: %d\n", WSAStartupErrorType);
		return -1;
	}
	
	//uncomment to get .exe location.
	//printf("%s\n", argv[0]);
	
	
	//if (argc != 2) {
	//	printf("ERROR: expected 2 arguments, got: %d", argc);
	//	return -1;
	//}
	unsigned int port;
	unsigned int clientaddrlen;
	SOCKET l_socket;
	SOCKET c_sockets[MAXCLIENTS];
	struct fd_set read_set; // kazkodel pavyzdyje nera struct jei bus problemu, gal todel.

	struct sockaddr_in servaddr;
	struct sockaddr_in clientaddr;

	int maxfd = 0;

	char buffer[BUFFLEN];

	port = 7956;
	//port = atoi(argv[1]);
	//if (port < 1 || port > 65535) {
	//	printf("ERROR: invalid port specified. Expected number in (1, 65535).");
	//	return -1;
	//}
	printf("Port is %i\n", port);

	if ((l_socket = socket(AF_INET, SOCK_STREAM, 0)) == INVALID_SOCKET) {
		printf("ERROR: cannot create listening socket.");
		return -1;
	}
	
	printf("Created socket successfully.\n");
	
	memset(&servaddr, 0, sizeof(servaddr));
	servaddr.sin_family = AF_INET;
	servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
	servaddr.sin_port = htons(port);
	
	if (bind(l_socket, (struct sockadrr*)&servaddr, sizeof(servaddr)) < 0) {
		printf("ERROR: cannot bind listening socket.");
		return -1;
	}
	
	printf("Binded socket successfully.\n");
	
	if (listen(l_socket, 5) < 0) {
		printf("ERROR: cannot listen on socket.");
		return -1;
	}
	
	printf("Listening on socket successfully.\n");
	
	for (int i = 0; i < MAXCLIENTS; ++i) {
		c_sockets[i] = -1;
	}
	char c = 0;
	scanf("%c", &c);
	
	while (1) {
		FD_ZERO(&read_set);
		for (int i = 0; i < MAXCLIENTS; ++i) {
			if (c_sockets != -1) {
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
		
		select(maxfd + 1, &read_set, NULL, NULL, NULL);
		
		if (FD_ISSET(l_socket, &read_set)) {
			int client_id = findemptyuser(c_sockets);
			if (client_id != -1) {
				clientaddrlen = sizeof(clientaddr);
				memset(&clientaddr, 0, clientaddrlen);
				c_sockets[client_id] = accept(l_socket, (struct sockaddr*)&clientaddr, &clientaddrlen);
				char buff[BUFFLEN];
				printf("Client %d connected from %s:%d\n", client_id, inet_ntop(AF_INET, &clientaddr, buff, BUFFLEN), ntohs(clientaddr.sin_port));
			}
		}
		
		for (int i=0; i<MAXCLIENTS; ++i) {
			if (c_sockets[i] != -1) {
				if (FD_ISSET(c_sockets[i], &read_set)) {
					memset(&buffer, 0, BUFFLEN);
					int r_len = recv(c_sockets[i], &buffer, BUFFLEN, 0);
					
					for (int j = 0; j < MAXCLIENTS; ++j) {
						if (c_sockets[j] != -1) {
							int w_len = send(c_sockets[j], buffer, r_len, 0);
							if (w_len <= 0) {
								close(c_sockets[j]);
								c_sockets[j] = -1;
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

int findemptyuser(int c_sockets[]) {
	int i;
	for (i = 0; i < MAXCLIENTS; i++) {
		if (c_sockets[i] == -1) {
			return i;
		}
	}
	return -1;
}