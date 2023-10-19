#ifndef SOCKET_H_
#define SOCKET_H_

#ifdef __linux__
#include <arpa/inet.h>
#include <sys/socket.h>
#include <unistd.h>

struct Socket{
	int handle;
	struct sockaddr_in address;
	socklen_t addressSize;
};

#endif

#ifdef _WIN32
#include "winsock.h"

struct Socket{
	SOCKET handle;
	sockaddr_in address;
	int addressSize;
	WSADATA wsa;
};
#endif

void Socket_init(Socket *, int, const char *);

void Socket_bind(Socket *);

void Socket_sendBuffer(Socket *, void *, int, Socket *);
void Socket_sendBufferToBoundSocket(Socket *, void *, int);

void Socket_receiveBuffer(Socket *, void *, int, Socket *);
void Socket_receiveBufferFromBoundSocket(Socket *, void *, int);

#endif
