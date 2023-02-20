#include <stdio.h>
#include <io.h>
#include <WS2tcpip.h>
#include <winsock2.h>

#define BUFFLEN 1024

#pragma comment(lib, "Ws2_32.lib")

void cleanup(char* message);

int main(int argc, char* argv[])
{
	unsigned int port = 12345;
	int s_socket;
	struct addrinfo* hp;
	struct sockaddr_in servaddr;
	fd_set read_set;

	char recvbuffer[BUFFLEN];
	char sendbuffer[BUFFLEN];

	WSADATA Data;
	int WSAStartupErrorType = WSAStartup(MAKEWORD(2, 2), &Data);
	if (WSAStartupErrorType) {
		cleanup("ERROR: WSAStartup failed with error: %d", WSAStartupErrorType);
		return -1;
	}

connection_start:

	printf("Client is in: %s\n", argv[0]);

	//if (argc != 3) {
	//	printf("Expected 3 arguments, got %d", argc);
	//	return -1;
	//}

	getaddrinfo("127.0.0.1", NULL, NULL, &hp);

	//port = atoi(argv[2]);

	if (port < 1 || port > 65535) {
		cleanup("ERROR: invalid port specified");
		return -1;
	}

	if ((s_socket = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
		cleanup("ERROR: can't create socket");
		return -1;
	}

	memset(&servaddr, 0, sizeof(servaddr));
	servaddr.sin_family = AF_INET;
	servaddr.sin_port = htons(port);

	if (inet_pton(AF_INET, "127.0.0.1", &servaddr.sin_addr) <= 0) {
		cleanup("ERROR: Invalid remote IP adress.");
		return -1;
	}

	if (connect(s_socket, (struct sockaddr*)&servaddr, sizeof(servaddr)) < 0) {
		cleanup("ERROR: can't connect to server");
		return -1;
	}

	printf("Connected to server successfully.\n");

	int i;

	while (1) {
		FD_ZERO(&read_set);
		FD_SET(s_socket, &read_set);
		FD_SET(0, &read_set);

		select(s_socket + 1, &read_set, NULL, NULL, NULL);

		if (FD_ISSET(s_socket, &read_set)) {
			memset(&recvbuffer, 0, BUFFLEN);
			i = recv(s_socket, &recvbuffer, BUFFLEN, 0);
			printf("%s\n", recvbuffer);
		}
		if (recvbuffer[0] == '!') {
			printf("Do you want to connect to the server again? (y/n)");
			fgets(sendbuffer, BUFFLEN, stdin);
			if (sendbuffer[0] == 'y')  goto connection_start;
			else exit(0);
		}
		if (recvbuffer[0] != '-') {
			fgets(sendbuffer, BUFFLEN, stdin);
			send(s_socket, sendbuffer, strlen(sendbuffer), 0);
			memset(&sendbuffer, 0, BUFFLEN);
		}
	}
}

void cleanup(char* message) {
	printf("%s\n", message);
	WSACleanup();
}