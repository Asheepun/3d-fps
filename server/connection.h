#ifndef CONNECTION_H_
#define CONNECTION_H_

const int PORT = 8080;
const int BUFFER_SIZE = 1024;

const char CONNECTION_REQUEST = 0;
const char CONNECTION_ID = 1;
const char CLIENT_INPUTS = 10;
const char SERVER_GAME_STATE = 20;

const int N_PLAYERS_MAX = 4;

struct Inputs{
	char forwards;
	char backwards;
	char left;
	char right;
	char jump;
	char shoot;
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

#endif
