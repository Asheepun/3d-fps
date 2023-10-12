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
	client_p->ready = false;
	client_p->startLevel_mutexed = false;
	client_p->gameOver = false;
	client_p->receivedGameState = false;

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

			client_p->receivedGameState = true;
		
		}

		if(message.type == MESSAGE_SERVER_LOBBY_STATE){

			ServerLobbyState serverLobbyState;
			memcpy(&serverLobbyState, message.buffer, sizeof(ServerLobbyState));

			pthread_mutex_lock(&client_p->serverLobbyStateMutex);

			client_p->latestServerLobbyState_mutexed = serverLobbyState;

			pthread_mutex_unlock(&client_p->serverLobbyStateMutex);
		
		}

		if(message.type == MESSAGE_START_LEVEL){

			
			pthread_mutex_lock(&client_p->startLevelMutex);

			client_p->startLevel_mutexed = true;
			memcpy(&client_p->startLevelData_mutexed, message.buffer, sizeof(StartLevelData));

			pthread_mutex_unlock(&client_p->startLevelMutex);
			
		}

		if(message.type == MESSAGE_GAME_OVER){
			client_p->gameOver = true;
		}

		if(message.type == MESSAGE_SHOT){

			ShotData shot;
			memcpy(&shot, message.buffer, sizeof(ShotData));

			pthread_mutex_lock(&client_p->latestServerShotsMutex);
			client_p->latestServerShots_mutexed.push_back(shot);
			pthread_mutex_unlock(&client_p->latestServerShotsMutex);

		}

	}

}

#define SEND_WITH_LAG
int SEND_LAG_FRAMES = 10;
std::vector<Message> lagBuffer;

void Client_sendInputsToServer(Client *client_p, Inputs inputs){

	Message message;
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

void Client_sendReadyToServer(Client *client_p, bool ready){

	Message message;
	message.type = MESSAGE_CLIENT_READY;
	message.connectionID = client_p->connectionID;
	memcpy(message.buffer, &ready, sizeof(bool));

	sendto(client_p->sockfd, &message, sizeof(Message), 0, (struct sockaddr*)&client_p->address, client_p->addressSize);

}
