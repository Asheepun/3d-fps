#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <arpa/inet.h>
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
std::vector<std::vector<Player>> pastPlayersBuffer;

//int currentConnectionID = 0;

//std::vector<TriangleMesh> triangleMeshes;

//std::vector<Obstacle> obstacles;
//std::vector<Player> players;

//Vec3f playerPos = getVec3f(5.0, 3.0, 5.0);
//Vec3f playerDirection = getVec3f(0.0, 0.0, 1.0);
//Vec3f playerVelocity = getVec3f(0.0, 0.0, 0.0);

//std::vector<Connection> connections;
//std::vector<Message> messages;

//int sockfd;

void *gameLoop(void *){

	size_t frameTime = 1000000 / 60;

	while(true){

		auto frameStartTime = std::chrono::high_resolution_clock::now();
		
		//printf("game loop\n");

		//handle inputs from clients
		for(int i = 0; i < server.connections.size(); i++){
			
			Connection *connection_p = &server.connections[i];
			
			//check if client has disconnected
			if(connection_p->ticksSinceLastInput - connection_p->n_receivedInputs > TICKS_UNTIL_DISCONNECT){
				printf("disconnected client\n");

				world.players.erase(world.players.begin() + World_getPlayerIndexByConnectionID(&world, connection_p->ID));

				server.connections.erase(server.connections.begin() + i);
				i--;
				continue;

			}

			//update player based on inputs buffer
			Player *player_p = World_getPlayerPointerByConnectionID(&world, connection_p->ID);

			while(connection_p->n_handledInputs < connection_p->n_receivedInputs){

				Inputs inputs = connection_p->inputQueue[connection_p->n_handledInputs % INPUT_QUEUE_SIZE];

				player_p->direction = inputs.cameraDirection;

				if(inputs.shoot){

					//calculate shot relative to the shooters time
					int timeDifference = server.gameTime - inputs.gameTime;
					int pastPlayersBufferIndex = max(pastPlayersBuffer.size() - 1 - timeDifference, 0);

					if(player_p->weapon == WEAPON_GUN){

						Vec3f hitPosition;
						Vec3f hitNormal;
						int hitConnectionID;
						bool hit = Player_World_shoot_common(player_p, &world, pastPlayersBuffer[pastPlayersBufferIndex], &hitPosition, &hitNormal, &hitConnectionID);

						if(hit){
							Player *hitPlayer_p = World_getPlayerPointerByConnectionID(&world, hitConnectionID);
							hitPlayer_p->health -= 20;
						}
					
					}
				}
				
				Player_World_moveAndCollideBasedOnInputs_common(player_p, &world, inputs);

				connection_p->n_handledInputs++;

			}

			if(connection_p->n_receivedInputs > 0){
				connection_p->ticksSinceLastInput++;
			}

		}

		//send game state to clients
		for(int i = 0; i < server.connections.size(); i++){

			Connection *connection_p = &server.connections[i];

			ServerGameState gameState;

			for(int j = 0; j < world.players.size(); j++){
				gameState.players[j].connectionID = world.players[j].connectionID;
				gameState.players[j].pos = world.players[j].pos;
				gameState.players[j].velocity = world.players[j].velocity;
				gameState.players[j].onGround = world.players[j].onGround;
				gameState.players[j].health = world.players[j].health;
			}

			gameState.n_players = world.players.size();

			gameState.n_handledInputs = connection_p->n_handledInputs;
			gameState.gameTime = server.gameTime;

			Message message;
			message.type = MESSAGE_SERVER_GAME_STATE;
			message.connectionID = connection_p->ID;
			memcpy(message.buffer, &gameState, sizeof(ServerGameState));

			sendto(server.sockfd, &message, sizeof(Message), 0, (struct sockaddr*)&connection_p->clientAddress, connection_p->clientAddressSize);
		
		}

		//save player states in buffer
		pastPlayersBuffer.push_back(world.players);

		if(pastPlayersBuffer.size() > PAST_PLAYERS_BUFFER_SIZE){
			pastPlayersBuffer.erase(pastPlayersBuffer.begin());
		}

		server.gameTime++;

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

		server.gameTime = 0;
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

		World_addObstacle(
			&world,
			getVec3f(0.0, 0.0, 0.0),
			TERRAIN_SCALE,
			World_getTriangleMeshIndexByName(&world, "terrain"),
			0,
			0,
			getVec4f(1.0, 1.0, 1.0, 1.0)
		);

	}
	
	printf("started server\n");

	//start game loop
	pthread_t gameLoopThread;
	pthread_create(&gameLoopThread, NULL, gameLoop, NULL);

	//receive messages from clients
	while(true){

		//char buffer[BUFFER_SIZE];
		//memset(buffer, 0, BUFFER_SIZE);
		struct sockaddr_in clientAddress;
		socklen_t clientAddressSize;
		clientAddressSize = sizeof(clientAddress);

		Message message;
		recvfrom(server.sockfd, &message, sizeof(Message), 0, (struct sockaddr*)&clientAddress, &clientAddressSize);

		if(message.type == MESSAGE_CONNECTION_REQUEST){

			printf("got connection request!\n");

			Connection connection;
			connection.ID = server.currentConnectionID;
			connection.clientAddress = clientAddress;
			connection.clientAddressSize = clientAddressSize;
			connection.n_handledInputs = 0;
			connection.n_receivedInputs = 0;
			connection.ticksSinceLastInput = 0;
			server.currentConnectionID++;

			server.connections.push_back(connection);

			World_addPlayer(&world, getVec3f(5.0, 3.0, 5.0), connection.ID);

			Message message;
			message.type = MESSAGE_CONNECTION_ID;
			message.connectionID = connection.ID;
			sendto(server.sockfd, &message, sizeof(Message), 0, (struct sockaddr*)&connection.clientAddress, connection.clientAddressSize);

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
				}

			}
		}

	}

	return 0;

}
