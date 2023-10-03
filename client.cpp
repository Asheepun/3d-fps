#include "game.h"

#include <arpa/inet.h>
#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

#include "pthread.h"

void *receiveServerMessages(void *);

//int n_sentInputs = 0;
void Client_init(Client *client_p){

	pthread_mutex_init(&client_p->serverGameStateMutex, NULL);
	
	client_p->n_sentInputs = 0;

	const char *ip = "127.0.0.1";
	//const char *ip = "206.189.58.34";

	client_p->sockfd = socket(AF_INET, SOCK_DGRAM, 0);
	memset(&client_p->address, '\0', sizeof(client_p->address));
	client_p->address.sin_family = AF_INET;
	client_p->address.sin_port = htons(PORT);
	client_p->address.sin_addr.s_addr = inet_addr(ip);

	client_p->addressSize = sizeof(client_p->address);

	Message message;
	message.type = MESSAGE_CONNECTION_REQUEST;
	sendto(client_p->sockfd, &message, sizeof(Message), 0, (struct sockaddr *)&client_p->address, client_p->addressSize);
	printf("sent message to port\n");

	memset(&message, 0, sizeof(Message));
	recvfrom(client_p->sockfd, &message, sizeof(Message), 0, (struct sockaddr*)&client_p->address, &client_p->addressSize);

	client_p->connectionID = message.connectionID;

	printf("got answer and id: %i\n", message.connectionID);

	pthread_t receiverThread;
	pthread_create(&receiverThread, NULL, receiveServerMessages, client_p);

}

void *receiveServerMessages(void *clientPointer){

	Client *client_p = (Client *)clientPointer;

	while(true){

		Message message;
		memset(&message, 0, sizeof(Message));
		recvfrom(client_p->sockfd, &message, sizeof(Message), 0, (struct sockaddr*)&client_p->address, &client_p->addressSize);

		if(message.type == MESSAGE_SERVER_GAME_STATE){

			ServerGameState gameState;
			memcpy(&gameState, message.buffer, sizeof(ServerGameState));
			//printf("got game state!\n");
			//Vec3f_log(gameState.playerPositions[0]);

			pthread_mutex_lock(&client_p->serverGameStateMutex);

			client_p->latestServerGameState_mutexed = gameState;

			pthread_mutex_unlock(&client_p->serverGameStateMutex);
		
		}

	}

}

/*
void Game_initClient(Game *game_p){

	game_p->n_sentInputs = 0;

	const char *ip = "127.0.0.1";
	int port = PORT;

	sockfd = socket(AF_INET, SOCK_DGRAM, 0);
	memset(&addr, '\0', sizeof(addr));
	addr.sin_family = AF_INET;
	addr.sin_port = htons(port);
	addr.sin_addr.s_addr = inet_addr(ip);
	client_p->addr_size = sizeof(client_p->addr);

	char buffer[BUFFER_SIZE];
	memset(buffer, 0, BUFFER_SIZE);
	buffer[0] = CONNECTION_REQUEST;
	sendto(sockfd, buffer, BUFFER_SIZE, 0, (struct sockaddr*)&addr, client_p->addr_size);
	//printf("[+]Data send: %s\n", buffer);

	memset(buffer, 0, BUFFER_SIZE);
	recvfrom(sockfd, buffer, BUFFER_SIZE, 0, (struct sockaddr*)&addr, &addr_size);

	if(buffer[0] == CONNECTION_ID){

		memcpy(&game_p->connectionID, buffer + 1, sizeof(int));

		printf("connectionID: %i\n", game_p->connectionID);
	
	}

	//pthread_t receiverThread;
	//pthread_create(&receiverThread, NULL, receiveServerMessages, game_p);

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
*/

#define SEND_WITH_LAG
int SEND_LAG_FRAMES = 10;
std::vector<Message> lagBuffer;

void Client_sendInputsToServer(Client *client_p, Inputs inputs){

	Message message;
	memset(&message, 0, sizeof(Message));

	message.type = MESSAGE_CLIENT_INPUTS;
	message.connectionID = client_p->connectionID;

	memcpy(message.buffer, &inputs, sizeof(Inputs));

#ifdef SEND_WITH_LAG
	lagBuffer.push_back(message);

	if(lagBuffer.size() > SEND_LAG_FRAMES){
		sendto(client_p->sockfd, &lagBuffer[0], sizeof(Message), 0, (struct sockaddr*)&client_p->address, client_p->addressSize);
		lagBuffer.erase(lagBuffer.begin());
	}
#else
	sendto(client_p->sockfd, &message, sizeof(Message), 0, (struct sockaddr*)&client_p->address, client_p->addressSize);
#endif

	client_p->n_sentInputs++;

}
