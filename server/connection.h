#ifndef CONNECTION_H_
#define CONNECTION_H_

#include <arpa/inet.h>
#include <sys/socket.h>
#include <unistd.h>

const int PORT = 8080;
const int BUFFER_SIZE = 1024;

const char MESSAGE_CONNECTION_REQUEST = 0;
const char MESSAGE_CONNECTION_ID = 1;
const char MESSAGE_CLIENT_INPUTS = 10;
const char MESSAGE_SERVER_GAME_STATE = 20;

const int N_PLAYERS_MAX = 4;

const int INPUT_QUEUE_SIZE = 64;
const int TICKS_UNTIL_DISCONNECT = 120;

//COMMON
struct Inputs{
	char forwards;
	char backwards;
	char left;
	char right;
	char jump;
	char shoot;
	char crouch;
	Vec3f cameraDirection;
};

struct ServerGameState{
	int n_players;
	Vec3f playerPositions[N_PLAYERS_MAX];
	Vec3f playerVelocities[N_PLAYERS_MAX];
	bool playerOnGrounds[N_PLAYERS_MAX];
	int playerConnectionIDs[N_PLAYERS_MAX];
	int n_handledInputs;
};

struct Message{
	int connectionID;
	char type;
	char buffer[BUFFER_SIZE];
};

//SERVER SIDE
struct Connection{
	struct sockaddr_in clientAddress;
	socklen_t clientAddressSize;
	int ID;
	Inputs inputQueue[INPUT_QUEUE_SIZE];
	long int n_receivedInputs;
	long int n_handledInputs;
	long int ticksSinceLastInput;
};

struct Server{
	int sockfd;
	struct sockaddr_in address;
	socklen_t addressSize;
	std::vector<Connection> connections;
	int currentConnectionID;
};

//CLIENT SIDE
struct Client{
	int sockfd;
	struct sockaddr_in address;
	socklen_t addressSize;
	int connectionID;
	int n_sentInputs;
	int n_receivedInputs;
};

#endif
