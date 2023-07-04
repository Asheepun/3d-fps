#include "game.h"

#include <arpa/inet.h>
#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

#include "pthread.h"

//int n_sentInputs = 0;

int sockfd;
struct sockaddr_in addr;
socklen_t addr_size;

void Game_initClient(Game *game_p){

	game_p->n_sentInputs = 0;

	const char *ip = "127.0.0.1";
	int port = PORT;

	sockfd = socket(AF_INET, SOCK_DGRAM, 0);
	memset(&addr, '\0', sizeof(addr));
	addr.sin_family = AF_INET;
	addr.sin_port = htons(port);
	addr.sin_addr.s_addr = inet_addr(ip);

	char buffer[BUFFER_SIZE];
	memset(buffer, 0, BUFFER_SIZE);
	buffer[0] = CONNECTION_REQUEST;
	sendto(sockfd, buffer, BUFFER_SIZE, 0, (struct sockaddr*)&addr, sizeof(addr));
	//printf("[+]Data send: %s\n", buffer);

	memset(buffer, 0, BUFFER_SIZE);
	addr_size = sizeof(addr);
	recvfrom(sockfd, buffer, BUFFER_SIZE, 0, (struct sockaddr*)&addr, &addr_size);

	if(buffer[0] == CONNECTION_ID){

		memcpy(&game_p->connectionID, buffer + 1, sizeof(int));

		printf("connectionID: %i\n", game_p->connectionID);
	
	}

	pthread_t receiverThread;
	pthread_create(&receiverThread, NULL, receiveServerMessages, game_p);

	//printf("[+]Data recv: %s\n", buffer);
 
}

void *receiveServerMessages(void *gamePointer){

	Game *game_p = (Game *)gamePointer;

	while(true){
		
		char buffer[BUFFER_SIZE];
		memset(buffer, 0, BUFFER_SIZE);
		addr_size = sizeof(addr);
		recvfrom(sockfd, buffer, BUFFER_SIZE, 0, (struct sockaddr*)&addr, &addr_size);

		if(buffer[0] == SERVER_GAME_STATE){

			ServerGameState gameState;
			memcpy(&gameState, buffer + 1, sizeof(ServerGameState));

			pthread_mutex_lock(&game_p->serverStateMutex);

			game_p->latestServerGameState_mutexed = gameState;

			pthread_mutex_unlock(&game_p->serverStateMutex);

			//game_p->player.pos = gameState.playerPos;
			//Vec3f_log(gameState.playerPos);
			//printf("got game state!\n");
		}

	}

}

#define SEND_WITH_LAG
int SEND_LAG_FRAMES = 10;
std::vector<char *>lagBuffers;

void Game_sendInputsToServer(Game *game_p, Inputs inputs){

	char buffer[BUFFER_SIZE];
	memset(buffer, 0, BUFFER_SIZE);

	buffer[0] = CLIENT_INPUTS;
	memcpy(buffer + 1, &game_p->connectionID, sizeof(int));
	memcpy(buffer + 1 + sizeof(int), &inputs, sizeof(Inputs));

#ifdef SEND_WITH_LAG
	char *copyBuffer = (char *)malloc(BUFFER_SIZE);
	memcpy(copyBuffer, buffer, BUFFER_SIZE);
	lagBuffers.push_back(copyBuffer);

	if(lagBuffers.size() > SEND_LAG_FRAMES){
		sendto(sockfd, lagBuffers[0], BUFFER_SIZE, 0, (struct sockaddr*)&addr, sizeof(addr));
		free(lagBuffers[0]);
		lagBuffers.erase(lagBuffers.begin());
	}
#else
	sendto(sockfd, buffer, BUFFER_SIZE, 0, (struct sockaddr*)&addr, sizeof(addr));
#endif

	game_p->n_sentInputs++;
	
}
