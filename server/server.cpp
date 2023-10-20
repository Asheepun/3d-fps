#include <stdio.h>
#include <stdlib.h>
#include <string.h>
//#include <sys/socket.h>
//#include <sys/types.h>
//#include <netinet/in.h>
//#include <arpa/inet.h>
#include <vector>
#include <chrono>
#include "time.h"
#include "unistd.h"
#include "pthread.h"
#include "math.h"

#include "../game.h"
#include "connection.h"

Server server;
World world;

Vec3f playerStartPositions[] = {
	(float)5.0, (float)3.0, (float)5.0,
	(float)(TERRAIN_SCALE - 5.0), (float)3.0, (float)(TERRAIN_SCALE - 5.0),
	(float)(TERRAIN_SCALE - 5.0), (float)3.0, (float)5.0,
	(float)5.0, (float)3.0, (float)(TERRAIN_SCALE - 5.0)
};

void *gameLoop(void *){

	size_t frameTime = 1000000 / 60;

	while(true){

		auto frameStartTime = std::chrono::high_resolution_clock::now();

		//check if a client has disconnected
		for(int i = 0; i < server.connections.size(); i++){

			Connection *connection_p = &server.connections[i];

			if(connection_p->ticksSinceLastMessage > TICKS_UNTIL_DISCONNECT){

				printf("disconnected client\n");

				for(int j = 0; j < world.players.size(); j++){
					if(world.players[j].connectionID == connection_p->ID){
						world.players.erase(world.players.begin() + j);
						j--;
						continue;
					}
				}

				server.connections.erase(server.connections.begin() + i);
				i--;
				continue;

			}

			connection_p->ticksSinceLastMessage++;
		
		}

		if(server.currentState == SERVER_STATE_LOBBY){

			//send lobby states to clients
			ServerLobbyState lobbyState;
			lobbyState.n_players = server.connections.size();
			lobbyState.n_readyPlayers = 0;
			for(int i = 0; i < server.connections.size(); i++){
				if(server.connections[i].ready){
					lobbyState.n_readyPlayers++;
				}
			}

			//printf("lobbyState: %i, %i\n", lobbyState.n_players, lobbyState.n_readyPlayers);

			for(int i = 0; i < server.connections.size(); i++){

				Connection *connection_p = &server.connections[i];

				Message message;
				message.type = MESSAGE_SERVER_LOBBY_STATE;
				message.connectionID = connection_p->ID;
				memcpy(message.buffer, &lobbyState, sizeof(ServerLobbyState));

				Socket_sendBuffer(&server.socket, &message, sizeof(Message), &connection_p->socket);
				//sendto(server.sockfd, &message, sizeof(Message), 0, (struct sockaddr*)&connection_p->clientAddress, connection_p->clientAddressSize);
			}
			
			//start the game if all players are ready
			if(lobbyState.n_players > 1
			&& lobbyState.n_readyPlayers == lobbyState.n_players){

				int seed = 1;
				setRandomSeed(seed);

				//setup world
				World_clear(&world);

				for(int i = 0; i < world.triangleMeshes.size(); i++){
					if(strcmp(world.triangleMeshes[i].name, "generated-tree") == 0){
						TriangleMesh_free(&world.triangleMeshes[i]);
						world.triangleMeshes.erase(world.triangleMeshes.begin() + i);
						i--;
						continue;
					}
				}

				//add obstacles
				World_addObstacle(
					&world,
					getVec3f(0.0, 0.0, 0.0),
					TERRAIN_SCALE,
					World_getTriangleMeshIndexByName(&world, "terrain"),
					0,
					0,
					getVec4f(1.0, 1.0, 1.0, 1.0)
				);

				//generate tree positions
				int n_trees = 3 + (int)(getRandom() * 3.0);
				std::vector<Vec3f> treePositions;

				for(int i = 0; i < n_trees; i++){

					Vec3f pos = getVec3f(
						TREE_TERRAIN_MARGIN + getRandom() * (TERRAIN_SCALE - 2.0 * TREE_TERRAIN_MARGIN),
						0.0,
						TREE_TERRAIN_MARGIN + getRandom() * (TERRAIN_SCALE - 2.0 * TREE_TERRAIN_MARGIN)
					);

					bool redo = false;
					for(int j = 0; j < treePositions.size(); j++){
						if(length(pos - treePositions[j]) < TREE_MIN_DISTANCE){
							redo = true;
						}
					}

					if(redo){
						i--;
						continue;
					}

					treePositions.push_back(pos);

				}

				for(int i = 0; i < treePositions.size(); i++){

					TriangleMesh triangleMesh;

					generateTree(getVec3f(0.0), NULL, NULL, &triangleMesh);

					world.triangleMeshes.push_back(triangleMesh);

					World_addObstacle(
						&world,
						treePositions[i],
						TREE_SCALE,
						world.triangleMeshes.size() - 1,
						0,
						0,
						getVec4f(0.7, 0.7, 0.7, 1.0)
					);
					
				}

				//add players
				for(int i = 0; i < server.connections.size(); i++){

					Connection *connection_p = &server.connections[i];

					World_addPlayer(&world, playerStartPositions[i], connection_p->ID);

				}

				//send start message to clients
				StartLevelData startLevelData;

				startLevelData.n_players = world.players.size();
				for(int i = 0; i < world.players.size(); i++){
					startLevelData.playerConnectionIDs[i] = world.players[i].connectionID;
					startLevelData.playerPositions[i] = world.players[i].pos;
				}

				startLevelData.n_trees = treePositions.size();
				memcpy(startLevelData.treePositions, &treePositions[0], startLevelData.n_trees * sizeof(Vec3f));

				for(int i = 0; i < server.connections.size(); i++){

					Connection *connection_p = &server.connections[i];

					//send start message
					Message message;
					message.type = MESSAGE_START_LEVEL;
					message.connectionID = connection_p->ID;
					memcpy(message.buffer, &startLevelData, sizeof(StartLevelData));

					//sendto(server.sockfd, &message, sizeof(Message), 0, (struct sockaddr*)&connection_p->clientAddress, connection_p->clientAddressSize);
					Socket_sendBuffer(&server.socket, &message, sizeof(Message), &connection_p->socket);
				}

				server.currentState = SERVER_STATE_LEVEL;
				
			}
		
		}else if(server.currentState == SERVER_STATE_LEVEL){

			//printf("level state: %i, %i\n", world.players.size(), server.connections.size());

			//check if game is over
			if(server.connections.size() <= 1
			|| world.players.size() <= 1){
				server.currentState = SERVER_STATE_LOBBY;

				for(int i = 0; i < server.connections.size(); i++){

					Connection *connection_p = &server.connections[i];
					connection_p->ready = false;

					Message message;
					message.type = MESSAGE_GAME_OVER;
					message.connectionID = connection_p->ID;

					//sendto(server.sockfd, &message, sizeof(Message), 0, (struct sockaddr*)&connection_p->clientAddress, connection_p->clientAddressSize);
					Socket_sendBuffer(&server.socket, &message, sizeof(Message), &connection_p->socket);
					
				}

				printf("switched to lobby state\n");
				continue;
			}

			std::vector<ShotData> shots;

			//handle inputs from clients
			for(int i = 0; i < server.connections.size(); i++){
				
				Connection *connection_p = &server.connections[i];
				
				//update player based on inputs buffer
				Player *player_p = World_getPlayerPointerByConnectionID(&world, connection_p->ID);

				if(player_p == NULL){
					continue;
				}

				while(connection_p->n_handledInputs < connection_p->n_receivedInputs){

					Inputs inputs = connection_p->inputQueue[connection_p->n_handledInputs % INPUT_QUEUE_SIZE];

					player_p->direction = inputs.cameraDirection;

					if(inputs.shoot){

						//add shot to shot data array
						ShotData shotData;
						shotData.pos = player_p->pos + getVec3f(0.0, player_p->height, 0.0);
						shotData.direction = player_p->direction;
						shotData.connectionID = connection_p->ID;
						shotData.gameTime = inputs.gameTime;
						shots.push_back(shotData);

						//calculate shot relative to the shooters time
						int timeDifference = server.gameTime - inputs.gameTime;
						int pastPlayersBufferIndex = max(world.pastPlayersBuffer.size() - 1 - timeDifference, 0);

						if(player_p->weapon == WEAPON_GUN){

							Vec3f hitPosition;
							Vec3f hitNormal;
							int hitConnectionID;
							bool hit = Player_World_shoot_common(player_p, &world, world.pastPlayersBuffer[pastPlayersBufferIndex], &hitPosition, &hitNormal, &hitConnectionID);

							if(hit){
								Player *hitPlayer_p = World_getPlayerPointerByConnectionID(&world, hitConnectionID);
								hitPlayer_p->health -= 20;
							}
						
						}
					}
					
					Player_World_moveAndCollideBasedOnInputs_common(player_p, &world, inputs);

					connection_p->n_handledInputs++;

				}

			}

			//check if players have died
			for(int i = 0; i < world.players.size(); i++){
				if(world.players[i].health <= 0){
					world.players.erase(world.players.begin() + i);
					i--;
					continue;
				}
			}

			//send game state to clients
			for(int i = 0; i < server.connections.size(); i++){

				Connection *connection_p = &server.connections[i];

				Inputs inputs = connection_p->inputQueue[connection_p->n_handledInputs % INPUT_QUEUE_SIZE];

				ServerGameState gameState;

				for(int j = 0; j < world.players.size(); j++){
					gameState.players[j].connectionID = world.players[j].connectionID;
					gameState.players[j].pos = world.players[j].pos;
					gameState.players[j].direction = world.players[j].direction;
					gameState.players[j].onGround = world.players[j].onGround;
					gameState.players[j].health = world.players[j].health;
					gameState.players[j].height = world.players[j].height;
					gameState.players[j].walkAngle = world.players[j].walkAngle;
				}

				gameState.n_players = world.players.size();

				gameState.n_handledInputs = connection_p->n_handledInputs;
				gameState.gameTime = server.gameTime;

				Message message;
				message.type = MESSAGE_SERVER_GAME_STATE;
				message.connectionID = connection_p->ID;
				memcpy(message.buffer, &gameState, sizeof(ServerGameState));

				//sendto(server.sockfd, &message, sizeof(Message), 0, (struct sockaddr*)&connection_p->clientAddress, connection_p->clientAddressSize);
				Socket_sendBuffer(&server.socket, &message, sizeof(Message), &connection_p->socket);
			
			}

			//send shots to clients
			for(int i = 0; i < shots.size(); i++){
				for(int j = 0; j < server.connections.size(); j++){

					Connection *connection_p = &server.connections[j];
					
					Message message;
					message.type = MESSAGE_SHOT;
					message.connectionID = connection_p->ID;
					memcpy(message.buffer, &shots[i], sizeof(ShotData));

					//sendto(server.sockfd, &message, sizeof(Message), 0, (struct sockaddr*)&connection_p->clientAddress, connection_p->clientAddressSize);
					Socket_sendBuffer(&server.socket, &message, sizeof(Message), &connection_p->socket);

				}
			}

			//save player states in buffer
			world.pastPlayersBuffer.push_back(world.players);

			if(world.pastPlayersBuffer.size() > PAST_PLAYERS_BUFFER_SIZE){
				world.pastPlayersBuffer.erase(world.pastPlayersBuffer.begin());
			}

			server.gameTime++;

		}

		auto frameStopTime = std::chrono::high_resolution_clock::now();

		long int totalFrameTime = (long int)(std::chrono::duration_cast<std::chrono::microseconds>(frameStopTime - frameStartTime).count());

		int lag = frameTime - totalFrameTime;

		if(lag < 0){
			lag = 0;
		}

		usleep(lag);

	}

}
 
int main(int argc, char **argv){

	//init server
	{
		const char *ip = "127.0.0.1";
		//const char *ip = "0.0.0.0";
	
		/*
		server.sockfd = socket(AF_INET, SOCK_DGRAM, 0);
		if (server.sockfd < 0) {
			perror("[-]socket error");
			exit(1);
		}

		memset(&server.address, '\0', sizeof(server.address));
		server.address.sin_family = AF_INET;
		server.address.sin_port = htons(PORT);
		server.address.sin_addr.s_addr = inet_addr(ip);

		server.addressSize = sizeof(server.address);

		int n = bind(server.sockfd, (struct sockaddr*)&server.address, server.addressSize);
		if (n < 0){
			perror("[-]bind error");
			exit(1);
		}
		*/
		Socket_init(&server.socket, PORT, ip);
		Socket_bind(&server.socket);

		server.gameTime = 0;
		server.currentState = SERVER_STATE_LOBBY;
	}

	//init world
	{
		World_loadAssets(&world, "../");

		//generate triangle mesh
		{
			Vec3f *triangles;
			int n_triangles;

			setRandomSeed(1);
			generateTerrainTriangles(&triangles, NULL, &n_triangles);

			TriangleMesh triangleMesh;
			triangleMesh.triangles = triangles;
			triangleMesh.n_triangles = n_triangles;
			String_set(triangleMesh.name, "terrain", STRING_SIZE);

			world.triangleMeshes.push_back(triangleMesh);
		}

	}
	
	printf("started server\n");

	//start game loop
	pthread_t gameLoopThread;
	pthread_create(&gameLoopThread, NULL, gameLoop, NULL);

	//receive messages from clients
	while(true){

		//char buffer[BUFFER_SIZE];
		//memset(buffer, 0, BUFFER_SIZE);
		//struct sockaddr_in clientAddress;
		//socklen_t clientAddressSize;
		//clientAddressSize = sizeof(clientAddress);
		Socket clientSocket;
		clientSocket.addressSize = sizeof(clientSocket.address);

		Message message;
		//recvfrom(server.sockfd, &message, sizeof(Message), 0, (struct sockaddr*)&clientAddress, &clientAddressSize);
		Socket_receiveBuffer(&server.socket, &message, sizeof(Message), &clientSocket);

		if(message.type == MESSAGE_CONNECTION_REQUEST
		&& server.connection.size() < N_PLAYERS_MAX){

			printf("got connection request!\n");

			Connection connection;
			connection.ID = server.currentConnectionID;
			connection.socket = clientSocket;
			//connection.clientAddress = clientAddress;
			//connection.clientAddressSize = clientAddressSize;
			connection.n_handledInputs = 0;
			connection.n_receivedInputs = 0;
			connection.ticksSinceLastMessage = 0;
			connection.ready = false;
			server.currentConnectionID++;

			server.connections.push_back(connection);

			//World_addPlayer(&world, getVec3f(5.0, 3.0, 5.0), connection.ID);

			Message message;
			message.type = MESSAGE_CONNECTION_ID;
			message.connectionID = connection.ID;
			//sendto(server.sockfd, &message, sizeof(Message), 0, (struct sockaddr*)&connection.clientAddress, connection.clientAddressSize);
			Socket_sendBuffer(&server.socket, &message, sizeof(Message), &connection.socket);

			printf("sent id: %i\n", connection.ID);

		}else if(message.type == MESSAGE_CLIENT_INPUTS){
			Inputs inputs;
			memcpy(&inputs, message.buffer, sizeof(Inputs));
			//printf("got inputs!\n");

			//printf("%i\n", inputs.forwards);
			//printf("%i\n", inputs.backwards);
			//printf("%i\n", inputs.left);
			//printf("%i\n", inputs.right);

			for(int i = 0; i < server.connections.size(); i++){

				Connection *connection_p = &server.connections[i];

				if(connection_p->ID == message.connectionID){
					connection_p->inputQueue[connection_p->n_receivedInputs % INPUT_QUEUE_SIZE] = inputs;
					connection_p->n_receivedInputs++;
					connection_p->ticksSinceLastMessage = 0;
				}

			}
		}else if(message.type == MESSAGE_CLIENT_READY){

			bool ready;
			memcpy(&ready, message.buffer, sizeof(bool));

			for(int i = 0; i < server.connections.size(); i++){

				Connection *connection_p = &server.connections[i];

				if(connection_p->ID == message.connectionID){
					connection_p->ready = ready;
					connection_p->ticksSinceLastMessage = 0;
				}

			}
			
		}

	}

	return 0;

}
