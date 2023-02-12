#include <stdio.h>
#include <Ws2tcpip.h>
#include <winsock2.h>

#define BUFFLEN 1024
#define MAXCLIENTS 10
#pragma comment(lib, "Ws2_32.lib")

int findemptyuser(int c_sockets[]);
void cleanup(char* message);

int main(int argc, char* argv[])
{
	printf("Starting up the server...\n");
	WSADATA Data;
	int WSAStartupErrorType = WSAStartup(MAKEWORD(2, 2), &Data);
	if (WSAStartupErrorType) {
		cleanup("ERROR: WSAStartup failed with error: %d\n", WSAGetLastError());
		return -1;
	}
	
	printf("Server is in: %s\n", argv[0]);


	//if (argc != 2) {
	//	printf("ERROR: expected 2 arguments, got: %d", argc);
	//	return -1;
	//}
	unsigned int port = 12345;
	unsigned int clientaddrlen;
	int l_socket;
	int c_sockets[MAXCLIENTS];
	fd_set read_set;

	struct sockaddr_in servaddr;
	struct sockaddr_in clientaddr;

	int maxfd = 0;

	char buffer[BUFFLEN];

	//port = atoi(argv[1]);
	if (port < 1 || port > 65535) {
		cleanup("ERROR: invalid port specified. Expected number in (1, 65535).");
		return -1;
	}
	printf("Port is %i\n", port);

	if ((l_socket = socket(AF_INET, SOCK_STREAM, 0)) == INVALID_SOCKET) {
		cleanup("ERROR: cannot create listening socket.");
		return -1;
	}

	printf("Created socket successfully.\n");

	memset(&servaddr, 0, sizeof(servaddr));
	servaddr.sin_family = AF_INET;
	servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
	servaddr.sin_port = htons(port);

	if (bind(l_socket, (struct sockadrr*)&servaddr, sizeof(servaddr)) < 0) {
		cleanup("ERROR: cannot bind listening socket.");
		return -1;
	}

	printf("Binded socket successfully.\n");

	if (listen(l_socket, 5) < 0) {
		cleanup("ERROR: cannot listen on socket.");
		return -1;
	}

	printf("Listening on socket successfully.\n");

	for (int i = 0; i < MAXCLIENTS; ++i) {
		c_sockets[i] = -1;
	}

	while (1) {
		FD_ZERO(&read_set);
		for (int i = 0; i < MAXCLIENTS; ++i) {
			if (c_sockets[i] != -1) {
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
				printf("Client %d connected from %s:%d\n", client_id, inet_ntop(AF_INET, &clientaddr, buffer, BUFFLEN), ntohs(clientaddr.sin_port));
				send(c_sockets[client_id], "Labas, ", sizeof("LABAS :)"), 0);
			}
		}

		for (int i = 0; i < MAXCLIENTS; ++i) {
			if (c_sockets[i] != -1) {
				if (FD_ISSET(c_sockets[i], &read_set)) {
					memset(&buffer, 0, BUFFLEN);
					int r_len = recv(c_sockets[i], &buffer, BUFFLEN, 0);

					for (int j = 0; j < MAXCLIENTS; ++j) {
						if (c_sockets[j] != -1) {
							int w_len = send(c_sockets[j], buffer, r_len, 0);
							if (w_len <= 0) {
								closesocket(c_sockets[j]);
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
	for (int i = 0; i < MAXCLIENTS; i++) {
		if (c_sockets[i] == -1) {
			return i;
		}
	}
	return -1;
}

void cleanup(char* message) {
	printf("%s\n", message);
	WSACleanup();
}