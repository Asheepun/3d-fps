#ifndef CONNECTION_H_
#define CONNECTION_H_

#include <arpa/inet.h>
#include <sys/socket.h>
#include <unistd.h>

const int PORT = 8080;
const int BUFFER_SIZE = 1024;

const char MESSAGE_CONNECTION_REQUEST = 0;
const char MESSAGE_CONNECTION_ID = 1;
const char MESSAGE_SERVER_GAME_STATE = 10;
const char MESSAGE_CLIENT_INPUTS = 11;
const char MESSAGE_SERVER_LOBBY_STATE = 20;
const char MESSAGE_CLIENT_READY = 21;
const char MESSAGE_START_LEVEL = 22;

const int N_PLAYERS_MAX = 4;
const int N_TREES_MAX = 6;

const int INPUT_QUEUE_SIZE = 64;
const int TICKS_UNTIL_DISCONNECT = 120;

const int PAST_PLAYERS_BUFFER_SIZE = 30;

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
	int sendingNumber;
	int gameTime;
};

struct PlayerData{
	int connectionID;
	int health;
	Vec3f pos;
	Vec3f velocity;
	bool onGround;
};

struct ServerGameState{
	int n_players;
	PlayerData players[N_PLAYERS_MAX];
	int n_handledInputs;
	int gameTime;
};

struct Message{
	int connectionID;
	char type;
	char buffer[BUFFER_SIZE];
};

struct ServerLobbyState{
	int n_players;
	int n_readyPlayers;
};

struct StartLevelData{
	int seed;
	int n_players;
	int playerConnectionIDs[N_PLAYERS_MAX];
	Vec3f playerPositions[N_PLAYERS_MAX];
	int n_trees;
	Vec3f treePositions[N_TREES_MAX];
};

//SERVER SIDE
enum ServerState{
	SERVER_STATE_LOBBY,
	SERVER_STATE_LEVEL,
};

struct Connection{
	struct sockaddr_in clientAddress;
	socklen_t clientAddressSize;
	int ID;
	Inputs inputQueue[INPUT_QUEUE_SIZE];
	long int n_receivedInputs;
	long int n_handledInputs;
	long int ticksSinceLastMessage;
	bool ready;
};

struct Server{
	enum ServerState currentState;
	int sockfd;
	struct sockaddr_in address;
	socklen_t addressSize;
	std::vector<Connection> connections;
	int currentConnectionID;
	int gameTime;
};

//CLIENT SIDE
struct Client{
	int sockfd;
	struct sockaddr_in address;
	socklen_t addressSize;
	int connectionID;
	int n_sentInputs;
	std::vector<Inputs> inputsBuffer;
	ServerGameState latestServerGameState_mutexed;
	pthread_mutex_t serverGameStateMutex;
	bool ready;
	ServerLobbyState latestServerLobbyState_mutexed;
	pthread_mutex_t serverLobbyStateMutex;
	bool startLevel;
	StartLevelData startLevelData;
	pthread_mutex_t startLevelMutex;
};

#endif
