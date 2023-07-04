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

int currentConnectionID = 0;

const int INPUT_QUEUE_SIZE = 64;

struct Connection{
	struct sockaddr_in clientAddress;
	socklen_t clientAddressSize;
	int ID;
	Inputs inputQueue[INPUT_QUEUE_SIZE];
	long int n_receivedInputs;
	long int n_handledInputs;
};

struct Message{
	char type;
	int connectionID;
	char data[BUFFER_SIZE];
};

std::vector<TriangleMesh> triangleMeshes;

std::vector<Obstacle> obstacles;
std::vector<Player> players;

//Vec3f playerPos = getVec3f(5.0, 3.0, 5.0);
//Vec3f playerDirection = getVec3f(0.0, 0.0, 1.0);
//Vec3f playerVelocity = getVec3f(0.0, 0.0, 0.0);

std::vector<Connection> connections;
std::vector<Message> messages;

int sockfd;

void *gameLoop(void *){

	size_t frameTime = 1000000 / 60;

	while(true){

		auto frameStartTime = std::chrono::high_resolution_clock::now();
		
		printf("game loop\n");

		//handle inputs from clients
		for(int i = 0; i < connections.size(); i++){

			Connection *connection_p = &connections[i];

			Player *player_p;
			for(int j = 0; j < players.size(); j++){
				if(players[j].connectionID == connection_p->ID){
					player_p = &players[j];
				}
			}

			if(connection_p->n_handledInputs < connection_p->n_receivedInputs){

				//printf("handling inputs\n");
				//printf("%i, %i\n", connection_p->n_handledInputs, connection_p->n_receivedInputs);

				Inputs inputs = connection_p->inputQueue[connection_p->n_handledInputs % INPUT_QUEUE_SIZE];
				connection_p->n_handledInputs++;

				//control player based on inputs
				Vec2f cameraDirectionXZ = getVec2f(inputs.cameraDirection.x, inputs.cameraDirection.z);
				Vec2f_normalize(&cameraDirectionXZ);

				if(inputs.forwards == 1){
					player_p->velocity.x += cameraDirectionXZ.x * PLAYER_SPEED;
					player_p->velocity.z += cameraDirectionXZ.y * PLAYER_SPEED;
				}
				if(inputs.backwards == 1){
					player_p->velocity.x += -cameraDirectionXZ.x * PLAYER_SPEED;
					player_p->velocity.z += -cameraDirectionXZ.y * PLAYER_SPEED;
				}
				if(inputs.left == 1){
					Vec3f left = getCrossVec3f(inputs.cameraDirection, getVec3f(0, 1.0, 0));
					Vec3f_normalize(&left);
					player_p->velocity.x += left.x * PLAYER_SPEED;
					player_p->velocity.z += left.z * PLAYER_SPEED;
				}
				if(inputs.right == 1){
					Vec3f right = getCrossVec3f(getVec3f(0, 1.0, 0), inputs.cameraDirection);
					Vec3f_normalize(&right);
					player_p->velocity.x += right.x * PLAYER_SPEED;
					player_p->velocity.z += right.z * PLAYER_SPEED;
				}
				if(inputs.jump == 1
				&& player_p->onGround){
					player_p->velocity.y += PLAYER_JUMP_SPEED;
				}
				if(inputs.jump == 0
				&& player_p->velocity.y > 0.0){
					player_p->velocity.y *= 0.9;
				}

				//handle player physics
				player_p->velocity.y += -PLAYER_GRAVITY;

				player_p->velocity.x *= PLAYER_WALK_RESISTANCE;
				player_p->velocity.z *= PLAYER_WALK_RESISTANCE;

				player_p->lastPos = player_p->pos;
				Vec3f_add(&player_p->pos, player_p->velocity);

				player_p->onGround = false;

				//handle collision between player and obstacles
				for(int i = 0; i < obstacles.size(); i++){
					
					Obstacle *obstacle_p = &obstacles[i];
					TriangleMesh *triangleMesh_p = &triangleMeshes[obstacle_p->triangleMeshIndex];

					//printf("obstacle: %i\n", i);

					for(int j = 0; j < triangleMesh_p->n_triangles; j++){

						//printf("triangle: %i\n", j);

						Vec3f triangle1 = triangleMesh_p->triangles[j * 3 + 0];
						Vec3f triangle2 = triangleMesh_p->triangles[j * 3 + 1];
						Vec3f triangle3 = triangleMesh_p->triangles[j * 3 + 2];

						Vec3f_mulByFloat(&triangle1, obstacle_p->scale);
						Vec3f_mulByFloat(&triangle2, obstacle_p->scale);
						Vec3f_mulByFloat(&triangle3, obstacle_p->scale);

						Vec3f_add(&triangle1, obstacle_p->pos);
						Vec3f_add(&triangle2, obstacle_p->pos);
						Vec3f_add(&triangle3, obstacle_p->pos);

						Vec3f_log(triangle1);
						Vec3f_log(triangle2);
						Vec3f_log(triangle3);

						Vec3f up = getVec3f(0.0, 1.0, 0.0);

						Vec3f u = getSubVec3f(triangle2, triangle1);
						Vec3f v = getSubVec3f(triangle3, triangle1);
						Vec3f N = getCrossVec3f(u, v);
						Vec3f_normalize(&N);

						float r = 0.2;
						Vec3f playerFeetPos = player_p->pos;
						playerFeetPos.y += r;

						if(getDotVec3f(up, N) > 0.7){

							Vec3f intersectionPoint;
							bool hit = checkLineToTriangleIntersectionVec3f(player_p->pos, getAddVec3f(player_p->pos, up), triangle1, triangle2, triangle3, &intersectionPoint);

							if(hit
							&& intersectionPoint.y > player_p->pos.y
							&& intersectionPoint.y < player_p->lastPos.y + r){
								player_p->pos.y = intersectionPoint.y;
								player_p->velocity.y = 0.0;
								player_p->onGround = true;
							}

						}else{

							bool hit = checkSphereToTriangleCollision(playerFeetPos, r, triangle1, triangle2, triangle3);

							if(hit){

								float distance = r - fabs((getDotVec3f(playerFeetPos, N) - getDotVec3f(triangle1, N)) / getDotVec3f(N, N));

								Vec3f_add(&player_p->pos, getMulVec3fFloat(N, distance));
								Vec3f_add(&player_p->velocity, getMulVec3fFloat(N, distance));
								//Vec3f_add(&game.player.velocity, N);
							}
						
						}

					}
				
				}
			
			}

		}

		//playerPos.y -= PLAYER_GRAVITY;

		//send updated state to clients
		for(int i = 0; i < connections.size(); i++){

			Connection *connection_p = &connections[i];

			ServerGameState gameState;
			for(int j = 0; j < players.size(); j++){
				gameState.playerPositions[j] = players[j].pos;
				gameState.playerVelocities[j] = players[j].velocity;
				gameState.playerOnGrounds[j] = players[j].onGround;
				gameState.playerConnectionIDs[j] = players[j].connectionID;
			}
			gameState.n_players = players.size();
			//gameState.playerPos = playerPos;
			//gameState.playerVelocity = playerVelocity;
			gameState.n_handledInputs = connection_p->n_handledInputs;

			char buffer[BUFFER_SIZE];
			memset(buffer, 0, BUFFER_SIZE);

			buffer[0] = SERVER_GAME_STATE;
			memcpy(buffer + 1, &gameState, sizeof(ServerGameState));

			sendto(sockfd, buffer, BUFFER_SIZE, 0, (struct sockaddr*)&connection_p->clientAddress, connection_p->clientAddressSize);

			printf("sent game state\n");
		
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

	//generate terrain mesh
	{
		TriangleMesh triangleMesh = generateTerrainTriangleMesh(TERRAIN_WIDTH, (1 / (TERRAIN_SCALE / 2.0)));
		triangleMeshes.push_back(triangleMesh);
	}

	//generate world
	{
		Obstacle obstacle;	
		obstacle.pos = getVec3f(0.0, 0.0, 0.0);
		obstacle.scale = TERRAIN_SCALE;
		obstacle.triangleMeshIndex = 0;

		obstacles.push_back(obstacle);
	}

	printf("started server\n");

	const char *ip = "127.0.0.1";
	int port = PORT;

	struct sockaddr_in server_addr, client_addr;
	socklen_t addr_size;
	int n;

	sockfd = socket(AF_INET, SOCK_DGRAM, 0);
	if (sockfd < 0) {
		perror("[-]socket error");
		exit(1);
	}

	memset(&server_addr, '\0', sizeof(server_addr));
	server_addr.sin_family = AF_INET;
	server_addr.sin_port = htons(port);
	server_addr.sin_addr.s_addr = inet_addr(ip);

	n = bind(sockfd, (struct sockaddr*)&server_addr, sizeof(server_addr));
	if (n < 0){
		perror("[-]bind error");
		exit(1);
	}

	//start game loop
	pthread_t gameLoopThread;
	pthread_create(&gameLoopThread, NULL, gameLoop, NULL);

	//receive messages from clients
	while(true){

		char buffer[BUFFER_SIZE];
		memset(buffer, 0, BUFFER_SIZE);
		addr_size = sizeof(client_addr);
		recvfrom(sockfd, buffer, BUFFER_SIZE, 0, (struct sockaddr*)&client_addr, &addr_size);

		if(buffer[0] == CONNECTION_REQUEST){

			Connection connection;
			connection.ID = currentConnectionID;
			connection.clientAddress = client_addr;
			connection.clientAddressSize = addr_size;
			connection.n_handledInputs = 0;
			connection.n_receivedInputs = 0;
			currentConnectionID++;

			connections.push_back(connection);

			Player player;
			player.pos = getVec3f(5.0, 3.0, 5.0);
			player.direction = getVec3f(0.0, 0.0, 1.0);
			player.velocity = getVec3f(0.0, 0.0, 0.0);
			player.connectionID = connection.ID;

			players.push_back(player);

			char buffer[BUFFER_SIZE];
			memset(buffer, 0, BUFFER_SIZE);

			buffer[0] = CONNECTION_ID;
			memcpy(buffer + 1, &connection.ID, sizeof(int));

			sendto(sockfd, buffer, BUFFER_SIZE, 0, (struct sockaddr*)&connection.clientAddress, connection.clientAddressSize);

		}

		if(buffer[0] == CLIENT_INPUTS){

			int connectionID;
			memcpy(&connectionID, buffer + 1, sizeof(int));

			Inputs inputs;
			memcpy(&inputs, buffer + 1 + sizeof(int), sizeof(Inputs));

			printf("got inputs\n");
			printf("%i\n", inputs.forwards);
			printf("%i\n", inputs.backwards);
			printf("%i\n", inputs.left);
			printf("%i\n", inputs.right);
			Vec3f_log(inputs.cameraDirection);

			for(int i = 0; i < connections.size(); i++){

				Connection *connection_p = &connections[i];

				if(connection_p->ID == connectionID){

					connection_p->inputQueue[connection_p->n_receivedInputs % INPUT_QUEUE_SIZE] = inputs;
					connection_p->n_receivedInputs++;

				}

			}
			
			//printf("Got inputs from: %i\n", connectionID);

		}

	}

	/*
	bzero(buffer, 1024);
	strcpy(buffer, "Welcome to the UDP Server.");
	sendto(sockfd, buffer, 1024, 0, (struct sockaddr*)&client_addr, sizeof(client_addr));
	printf("[+]Data send: %s\n", buffer);
	*/

	return 0;

}
