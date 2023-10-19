#ifndef SOCKET_H_
#define SOCKET_H_

#include "winsock.h"

struct Socket{
	SOCKET handle;
	sockaddr_in address;
	int addressSize;
	WSADATA wsa;
};

void Socket_init(Socket *, int, const char *);

void Socket_bind(Socket *);

void Socket_sendBuffer(Socket *, void *, int, Socket *);
void Socket_sendBufferToBoundSocket(Socket *, void *, int);

void Socket_receiveBuffer(Socket *, void *, int, Socket *);
void Socket_receiveBufferFromBoundSocket(Socket *, void *, int);

#endif
