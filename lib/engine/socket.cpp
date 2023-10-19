#include "engine/socket.h"

#include "stdio.h"
#include "string.h"

#ifdef __linux__
void Socket_init(Socket *socket_p, int PORT, const char *ip){

	socket_p->handle = socket(AF_INET, SOCK_DGRAM, 0);

	socket_p->addressSize = sizeof(socket_p->address);

	memset(&socket_p->address, '\0', socket_p->addressSize);
	socket_p->address.sin_family = AF_INET;
	socket_p->address.sin_port = htons(PORT);
	socket_p->address.sin_addr.s_addr = inet_addr(ip);

}

#endif

#ifdef _WIN32
void Socket_init(Socket *socket_p, int PORT, const char *ip){

	if(WSAStartup(MAKEWORD(2,2), &socket_p->wsa) != 0){
		printf("Failed. Error Code : %d",WSAGetLastError());
		exit(EXIT_FAILURE);
	}

	socket_p->handle = socket(AF_INET, SOCK_DGRAM, 0);

	if(socket_p->handle == INVALID_SOCKET){
		printf("Could not create socket : %d" , WSAGetLastError());
	}

	socket_p->addressSize = sizeof(socket_p->address);

	memset(&socket_p->address, '\0', socket_p->addressSize);
	socket_p->address.sin_family = AF_INET;
	socket_p->address.sin_port = htons(PORT);
	socket_p->address.sin_addr.s_addr = inet_addr(ip);

}
#endif

void Socket_bind(Socket *socket_p){

	int n = bind(socket_p->handle, (struct sockaddr*)&socket_p->address, socket_p->addressSize);

	if (n < 0){
		perror("[-]bind error");
	}

}

void Socket_sendBuffer(Socket *socket_p, void *buffer, int bufferSize, Socket *remoteSocket_p){
	sendto(socket_p->handle, (const char *)buffer, bufferSize, 0, (struct sockaddr*)&remoteSocket_p->address, remoteSocket_p->addressSize);
}

void Socket_sendBufferToBoundSocket(Socket *socket_p, void *buffer, int bufferSize){
	sendto(socket_p->handle, (const char *)buffer, bufferSize, 0, (struct sockaddr*)&socket_p->address, socket_p->addressSize);
}

void Socket_receiveBuffer(Socket *socket_p, void *buffer, int bufferSize, Socket *remoteSocket_p){
	recvfrom(socket_p->handle, (char *)buffer, bufferSize, 0, (struct sockaddr*)&remoteSocket_p->address, &remoteSocket_p->addressSize);
}

void Socket_receiveBufferFromBoundSocket(Socket *socket_p, void *buffer, int bufferSize){
	recvfrom(socket_p->handle, (char *)buffer, bufferSize, 0, (struct sockaddr*)&socket_p->address, &socket_p->addressSize);
}
