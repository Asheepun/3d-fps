#include "engine/shaders.h"
#include "engine/renderer2d.h"
#include "engine/text.h"

#include "game.h"

#include "stdio.h"
#include "stdlib.h"
#include "math.h"
#include "time.h"
#include <cstring>
#include <vector>
#include <immintrin.h>

Vec3f treePos = { 10.0, 0.0, 15.0 };
TextureBuffer leafTransformationsTextureBuffer;

bool drawTimings = true;

float gameTime = 0.0;

Game game;

float windTime = 0.0;

const int GRASS_GRID_WIDTH = 10;
//const float GRASS_DISTANCE = 1.0;
const float GRASS_DISTANCE = 1.0;
//const float GRASS_DISTANCE = 1.5;
TextureBuffer grassPositionsTextureBuffer;
std::vector<Vec4f> grassPositions;
Vec4f *sortedGrassPositions;

int WIDTH = 1920;
int HEIGHT = 1080;

float GRASS_SHADOW_STRENGTH = 0.3;

unsigned int shadowMapFBO;
Texture shadowMapDepthTexture;
int SHADOW_MAP_WIDTH = 500;
int SHADOW_MAP_HEIGHT = 500;
float shadowMapScale = 10.0;

unsigned int grassShadowMapFBO;
Texture grassShadowMapDepthTexture;
int GRASS_SHADOW_MAP_WIDTH = 500;
int GRASS_SHADOW_MAP_HEIGHT = 500;
//float grassShadowMapScale = 10.0;

unsigned int bigShadowMapFBO;
Texture bigShadowMapDepthTexture;
int BIG_SHADOW_MAP_WIDTH = 500;
int BIG_SHADOW_MAP_HEIGHT = 500;
float bigShadowMapScale = 70.0;

Texture paintMapTexture;
int PAINT_MAP_WIDTH = 300;
int PAINT_MAP_HEIGHT = 300;
float PAINT_MAP_SCALE = 100.0;
unsigned char *paintMap;

Vec3f lightPos = { 0.0, 20.0, 0.0 };
Vec3f lightDirection = { 0.7, -1.0, 0.5 };

float fov = M_PI / 4;
float farPlane = 100.0;
float nearPlane = 0.1;
Vec3f cameraPos = getVec3f(4.0, 5.0, 0.0);
Vec3f cameraDirection = getVec3f(0.0, 0.0, 1.0);
Vec2f cameraRotation = getVec2f(M_PI / 2.0, 0.0);

Renderer2D_Renderer renderer2D;
Font font;

void Engine_start(){

#ifndef RUN_OFFLINE
	Game_initClient(&game);
#endif

	Game_loadAssets(&game);

	Renderer2D_init(&renderer2D, WIDTH, HEIGHT);

	font = getFont("assets/times.ttf", 60);

	//generate water mesh
	{
		float scale = 20.0;
		int n_triangles = (scale) * (scale) * 2;
		float *meshData = (float *)malloc(n_triangles * 6 * 8 * sizeof(float));

		int index = 0;

		for(float x = 0.0; x < 1.0; x += 1.0 / scale){
			for(float z = 0.0; z < 1.0; z += 1.0 / scale){

				Vec2f t1 = getVec2f(x, z);
				Vec2f t2 = getVec2f(x + 1.0 / scale, z);
				Vec2f t3 = getVec2f(x, z + 1.0 / scale);
				Vec2f t4 = getVec2f(x + 1.0 / scale, z + 1.0 / scale);

				Vec3f p1 = getVec3f(t1.x, t1.y, t4.x);
				Vec3f n1 = getVec3f(t4.y, t2.x, t2.y);
				Vec3f p2 = getVec3f(t4.x, t4.y, t2.x);
				Vec3f n2 = getVec3f(t2.y, t1.x, t1.y);
				Vec3f p3 = getVec3f(t2.x, t2.y, t1.x);
				Vec3f n3 = getVec3f(t1.y, t4.x, t4.y);

				Vec3f p4 = getVec3f(t1.x, t1.y, t3.x);
				Vec3f n4 = getVec3f(t3.y, t4.x, t4.y);
				Vec3f p5 = getVec3f(t3.x, t3.y, t4.x);
				Vec3f n5 = getVec3f(t4.y, t1.x, t1.y);
				Vec3f p6 = getVec3f(t4.x, t4.y, t1.x);
				Vec3f n6 = getVec3f(t1.y, t3.x, t3.y);

				memcpy(meshData + index * 8, &p1, sizeof(Vec3f));
				memcpy(meshData + index * 8 + 3, &t1, sizeof(Vec2f));
				memcpy(meshData + index * 8 + 5, &n1, sizeof(Vec3f));
				index++;

				memcpy(meshData + index * 8, &p2, sizeof(Vec3f));
				memcpy(meshData + index * 8 + 3, &t4, sizeof(Vec2f));
				memcpy(meshData + index * 8 + 5, &n2, sizeof(Vec3f));
				index++;

				memcpy(meshData + index * 8, &p3, sizeof(Vec3f));
				memcpy(meshData + index * 8 + 3, &t2, sizeof(Vec2f));
				memcpy(meshData + index * 8 + 5, &n3, sizeof(Vec3f));
				index++;

				memcpy(meshData + index * 8, &p4, sizeof(Vec3f));
				memcpy(meshData + index * 8 + 3, &t1, sizeof(Vec2f));
				memcpy(meshData + index * 8 + 5, &n4, sizeof(Vec3f));
				index++;

				memcpy(meshData + index * 8, &p5, sizeof(Vec3f));
				memcpy(meshData + index * 8 + 3, &t3, sizeof(Vec2f));
				memcpy(meshData + index * 8 + 5, &n5, sizeof(Vec3f));
				index++;

				memcpy(meshData + index * 8, &p6, sizeof(Vec3f));
				memcpy(meshData + index * 8 + 3, &t4, sizeof(Vec2f));
				memcpy(meshData + index * 8 + 5, &n6, sizeof(Vec3f));
				index++;

			}
		}

		Model model;
		Model_initFromMeshData(&model, (unsigned char *)meshData, n_triangles);
		String_set(model.name, "water", STRING_SIZE);

		game.models.push_back(model);

	}

	//generate underwater terrain
	{
	
	}

	//generate terrain
	{
		setRandomSeed(1);

		int width = 20;

		Vec3f *points = (Vec3f *)malloc(sizeof(Vec3f) * width * width);

		for(int x = 0; x < width; x++){
			for(int y = 0; y < width; y++){

				int index = y * width + x;

				points[index].x = x;
				points[index].z = y;
				points[index].y = getRandom() / TERRAIN_SCALE * 2.0;

				if(y == 0 || x == 0 || y == width - 1 || x == width - 1){
					points[index].y = -0.0;
				}

				points[index].x /= (float)width;
				points[index].z /= (float)width;
			
			}
		}

		int n_triangles = 2 * (width - 1) * (width - 1);

		Vec3f *triangles = (Vec3f *)malloc(sizeof(Vec3f) * 3 * n_triangles);

		int triangleIndex = 0;

		for(int x = 0; x < width - 1; x++){
			for(int y = 0; y < width - 1; y++){

				int pointsIndex1 = y * width + x;
				int pointsIndex2 = (y + 1) * width + x;
				int pointsIndex3 = y * width + (x + 1);
				int pointsIndex4 = (y + 1) * width + (x + 1);

				Vec3f point1 = points[pointsIndex1];
				Vec3f point2 = points[pointsIndex2];
				Vec3f point3 = points[pointsIndex3];
				Vec3f point4 = points[pointsIndex4];

				triangles[triangleIndex * 3 * 2 + 0] = point1;
				triangles[triangleIndex * 3 * 2 + 1] = point2;
				triangles[triangleIndex * 3 * 2 + 2] = point4;
				triangles[triangleIndex * 3 * 2 + 3] = point1;
				triangles[triangleIndex * 3 * 2 + 4] = point4;
				triangles[triangleIndex * 3 * 2 + 5] = point3;

				triangleIndex++;

			}
		}

		free(points);
		
		TriangleMesh triangleMesh;
		triangleMesh.triangles = triangles;
		triangleMesh.n_triangles = n_triangles;
		String_set(triangleMesh.name, "terrain", STRING_SIZE);

		game.triangleMeshes.push_back(triangleMesh);

		unsigned char *meshData = generateMeshDataFromTriangleMesh(triangleMesh, NULL);

		Model model;

		Model_initFromMeshData(&model, meshData, triangleMesh.n_triangles);

		String_set(model.name, "terrain", STRING_SIZE);

		game.models.push_back(model);

		//free unneeded data
		free(meshData);

	}

	//generate grass positions
	{
		TriangleMesh *terrainTriangleMesh_p = Game_getTriangleMeshPointerByName(&game, "terrain");

		Vec2f center = getVec2f(TERRAIN_SCALE / 2.0, TERRAIN_SCALE / 2.0);

		float tolerance = GRASS_DISTANCE / 100.0;
		Vec2f stepPos = getVec2f(0.0, 0.0);
		Vec2f dir = getVec2f(0.0, GRASS_DISTANCE);
		float marginNorth = 0.0;
		float marginSouth = GRASS_DISTANCE;
		float marginWest = 0.0;
		float marginEast = GRASS_DISTANCE;

		int n = 0;

		/*
		for(int i = 0; i < TERRAIN_SCALE * TERRAIN_SCALE / (GRASS_DISTANCE * GRASS_DISTANCE); i++){

			if(stepPos.y - GRASS_DISTANCE + tolerance > TERRAIN_SCALE - marginSouth){
				stepPos.y -= GRASS_DISTANCE;
				dir = getVec2f(GRASS_DISTANCE, 0.0);
				stepPos += dir;
				marginWest += GRASS_DISTANCE;
			}else if(stepPos.x - GRASS_DISTANCE + tolerance > TERRAIN_SCALE - marginEast){
				stepPos.x -= GRASS_DISTANCE;
				dir = getVec2f(0.0, -GRASS_DISTANCE);
				stepPos += dir;
				marginSouth += GRASS_DISTANCE;
			}else if(stepPos.y + GRASS_DISTANCE - tolerance < marginNorth){
				stepPos.y += GRASS_DISTANCE;
				dir = getVec2f(-GRASS_DISTANCE, 0.0);
				stepPos += dir;
				marginEast += GRASS_DISTANCE;
			}else if(stepPos.x + GRASS_DISTANCE - tolerance < marginWest){
				stepPos.x += GRASS_DISTANCE;
				dir = getVec2f(0.0, GRASS_DISTANCE);
				stepPos += dir;
				marginNorth += GRASS_DISTANCE;
			}
			*/

		float margin = 3.0;

		for(float x = margin; x < TERRAIN_SCALE - margin; x += GRASS_DISTANCE){
			for(float y = margin; y < TERRAIN_SCALE - margin; y += GRASS_DISTANCE){

				Vec4f pos = getVec4f(x, 20.0, y, 0.0);

				pos.x += getRandom() * 0.5 * GRASS_DISTANCE;
				pos.z += getRandom() * 0.5 * GRASS_DISTANCE;

				Vec3f rayOrigin = getVec3f(pos.x, pos.y, pos.z);
				Vec3f rayDirection = getVec3f(0.0, -1.0, 0.0);

				Vec3f intersectionPoint;
				bool hit = false;

				{
					bool somethingHit = false;
					bool hasTriangle = false;

					for(int j = 0; j < terrainTriangleMesh_p->n_triangles; j++){

						Vec3f p1 = terrainTriangleMesh_p->triangles[j * 3 + 0] * TERRAIN_SCALE;
						Vec3f p2 = terrainTriangleMesh_p->triangles[j * 3 + 1] * TERRAIN_SCALE;
						Vec3f p3 = terrainTriangleMesh_p->triangles[j * 3 + 2] * TERRAIN_SCALE;

						float east = fmin(fmin(p1.x, p2.x), p3.x);
						float west = fmax(fmax(p1.x, p2.x), p3.x);
						float north = fmin(fmin(p1.z, p2.z), p3.z);
						float south = fmax(fmax(p1.z, p2.z), p3.z);

						if(rayOrigin.x > east && rayOrigin.x < west
						&& rayOrigin.z > north && rayOrigin.z < south){

							Vec3f checkIntersectionPoint;
							bool checkHit = checkLineToTriangleIntersectionVec3f(rayOrigin, rayOrigin + rayDirection, p1, p2, p3, &checkIntersectionPoint);

							if(checkHit){
								intersectionPoint = checkIntersectionPoint;
								hit = true;
							}

						}

					}
				}

				if(hit){
					pos.y = intersectionPoint.y + 0.5;
					pos.w = getRandom() * M_PI;
					grassPositions.push_back(pos);
					pos.y += 3.0;
					//grassPositions2.push_back(pos);
				}

				n++;

				//stepPos += dir;
			}
		}

		TextureBuffer_initAsVec4fArray(&grassPositionsTextureBuffer, &grassPositions[0], grassPositions.size());

		sortedGrassPositions = (Vec4f *)malloc(sizeof(Vec4f) * grassPositions.size());
	}

	Engine_setWindowSize(WIDTH / 2, HEIGHT / 2);

	Engine_centerWindow();

	Engine_setFPSMode(true);

	//OpenGL stuff
	glEnable(GL_BLEND);

	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	glDepthFunc(GL_LESS);
	glDepthMask(GL_TRUE);
	glEnable(GL_DEPTH_TEST);
	glDepthRange(0.0, 1.0);

	glEnable(GL_CULL_FACE);
	glCullFace(GL_FRONT);

	//generate shadow map depth textures
	Texture_initAsDepthMap(&shadowMapDepthTexture, SHADOW_MAP_WIDTH, SHADOW_MAP_HEIGHT);
	Texture_initAsDepthMap(&grassShadowMapDepthTexture, GRASS_SHADOW_MAP_WIDTH, GRASS_SHADOW_MAP_HEIGHT);
	Texture_initAsDepthMap(&bigShadowMapDepthTexture, BIG_SHADOW_MAP_WIDTH, BIG_SHADOW_MAP_HEIGHT);
	//Texture_initAsColorMap(&shadowMapDataTexture, SHADOW_MAP_WIDTH, SHADOW_MAP_HEIGHT);

	//generate shadow map frame buffer
	glGenFramebuffers(1, &shadowMapFBO);
	glBindFramebuffer(GL_FRAMEBUFFER, shadowMapFBO);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, shadowMapDepthTexture.ID, 0);
	//glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, shadowMapDataTexture.ID, 0);

	//generate grass shadow map frame buffer
	glGenFramebuffers(1, &grassShadowMapFBO);
	glBindFramebuffer(GL_FRAMEBUFFER, grassShadowMapFBO);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, grassShadowMapDepthTexture.ID, 0);

	//generate big shadow map frame buffer
	glGenFramebuffers(1, &bigShadowMapFBO);
	glBindFramebuffer(GL_FRAMEBUFFER, bigShadowMapFBO);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, bigShadowMapDepthTexture.ID, 0);

	//generate paint map
	//Texture_initAsDepthMap(&paintMapTexture, PAINT_MAP_WIDTH, PAINT_MAP_HEIGHT);
	paintMap = (unsigned char *)malloc(sizeof(unsigned char) * PAINT_MAP_WIDTH * PAINT_MAP_HEIGHT);
	memset(paintMap, 0, sizeof(unsigned char) * PAINT_MAP_WIDTH * PAINT_MAP_HEIGHT);
		
	

	glGenTextures(1, &paintMapTexture.ID);
	glBindTexture(GL_TEXTURE_2D, paintMapTexture.ID);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_R8, PAINT_MAP_WIDTH, PAINT_MAP_HEIGHT, 0, GL_RED, GL_UNSIGNED_BYTE, paintMap);
	glGenerateMipmap(GL_TEXTURE_2D);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

	//glDrawBuffer(GL_NONE);
	//glReadBuffer(GL_NONE);

	glBindFramebuffer(GL_FRAMEBUFFER, 0);  

	//init game
	pthread_mutex_init(&game.serverStateMutex, NULL);

	{
		Obstacle obstacle;	
		obstacle.pos = getVec3f(0.0, 0.0, 0.0);
		obstacle.scale = TERRAIN_SCALE;
		obstacle.modelIndex = Game_getModelIndexByName(&game, "terrain");
		obstacle.textureIndex = Game_getTextureIndexByName(&game, "blank");
		obstacle.alphaTextureIndex = Game_getTextureIndexByName(&game, "blank-alpha");
		obstacle.triangleMeshIndex = Game_getTriangleMeshIndexByName(&game, "terrain");
		obstacle.color = TERRAIN_COLOR;

		game.obstacles.push_back(obstacle);
	}
	{
		Obstacle obstacle;	
		obstacle.pos = getVec3f(30.0, 2.0, 20.0);
		obstacle.scale = 2.0;
		obstacle.modelIndex = Game_getModelIndexByName(&game, "cube");
		obstacle.textureIndex = Game_getTextureIndexByName(&game, "blank");
		obstacle.alphaTextureIndex = Game_getTextureIndexByName(&game, "blank-alpha");
		obstacle.triangleMeshIndex = Game_getTriangleMeshIndexByName(&game, "cube");
		obstacle.color = getVec4f(0.7, 0.7, 0.7, 1.0);

		game.obstacles.push_back(obstacle);
	}
	{
		game.player.pos = getVec3f(5.0, 3.0, 5.0);
		game.player.lastPos = game.player.pos;
		game.player.velocity = getVec3f(0.0, 0.0, 0.0);
		game.player.onGround = false;
		game.player.height = PLAYER_HEIGHT_STANDING;
		//Vec3f lastPlayerPos = game.player.pos;
		//Vec3f game.player.velocity = { 0.0, 0.0, 0.0 };
		//bool playerOnGround = false;
		//float game.player.height = PLAYER_HEIGHT_STANDING;
	}
	{
		Player player;
		player.pos = getVec3f(10.0, 5.0, 30.0);
		game.otherPlayers.push_back(player);
	}

	Game_addTree(&game, getVec3f(15.0, 0.0, 15.0));

	Game_addTree(&game, getVec3f(25.0, 0.0, 15.0));

	Game_addTree(&game, getVec3f(35.0, 0.0, 15.0));

	Game_addTree(&game, getVec3f(90.0, 0.0, 15.0));

	Game_addTree(&game, getVec3f(15.0, 0.0, 90.0));

	Game_addTree(&game, getVec3f(90.0, 0.0, 90.0));

	Game_addTree(&game, getVec3f(40.0, 0.0, 40.0));

	//Engine_toggleFullscreen();

}

void Engine_update(float deltaTime){

	//handle window controls
	if(Engine_keys[ENGINE_KEY_Q].down){
		Engine_quit();
	}
	if(Engine_keys[ENGINE_KEY_F].downed){
		Engine_toggleFullscreen();
	}
	if(Engine_keys[ENGINE_KEY_T].downed){
		drawTimings = !drawTimings;
	}

	//printf("---\n");

	//get latest server game state
	pthread_mutex_lock(&game.serverStateMutex);

	ServerGameState latestServerGameState = game.latestServerGameState_mutexed;

	pthread_mutex_unlock(&game.serverStateMutex);

	//game.player.pos = latestServerGameState.playerPos;

	//printf("%i, %i\n", latestServerGameState.n_handledInputs, game.n_sentInputs);

	//Vec3f_log(latestServerGameState.playerPos);

	Inputs inputs;
	memset(&inputs, 0, sizeof(Inputs));

	//handle keyboard inputs
	if(Engine_keys[ENGINE_KEY_W].down){
		inputs.forwards = 1;
	}
	if(Engine_keys[ENGINE_KEY_S].down){
		inputs.backwards = 1;
	}
	if(Engine_keys[ENGINE_KEY_A].down){
		inputs.left = 1;
	}
	if(Engine_keys[ENGINE_KEY_D].down){
		inputs.right = 1;
	}
	if(Engine_keys[ENGINE_KEY_SPACE].down){
		inputs.jump = 1;
	}
	if(Engine_keys[ENGINE_KEY_CONTROL].down){
		inputs.crouch = 1;
	}
	if(Engine_pointer.downed){
		inputs.shoot = 1;
	}

	//handle pointer intputs
	cameraRotation.x += -Engine_pointer.movement.x * PLAYER_LOOK_SPEED;
	cameraRotation.y += -Engine_pointer.movement.y * PLAYER_LOOK_SPEED;

	if(cameraRotation.y > M_PI / 2 - 0.01){
		cameraRotation.y = M_PI / 2 - 0.01;
	}
	if(cameraRotation.y < -(M_PI / 2 - 0.01)){
		cameraRotation.y = -(M_PI / 2 - 0.01);
	}

	cameraDirection.y = sin(cameraRotation.y);
	cameraDirection.x = cos(cameraRotation.x) * cos(cameraRotation.y);
	cameraDirection.z = sin(cameraRotation.x) * cos(cameraRotation.y);
	Vec3f_normalize(&cameraDirection);

	inputs.cameraDirection = cameraDirection;

	//buffer inputs
	game.inputsBuffer.push_back(inputs);

#ifndef RUN_OFFLINE
	//send inputs to server
	Game_sendInputsToServer(&game, inputs);

	//update client based on latest server state
	game.otherPlayers.clear();

	for(int i = 0; i < latestServerGameState.n_players; i++){
		printf("player %i\n", i);
		if(game.connectionID == latestServerGameState.playerConnectionIDs[i]){
			game.player.pos = latestServerGameState.playerPositions[i];
			game.player.velocity = latestServerGameState.playerVelocities[i];
			game.player.onGround = latestServerGameState.playerOnGrounds[i];
		}else{
			Player otherPlayer;
			otherPlayer.pos = latestServerGameState.playerPositions[i];
			game.otherPlayers.push_back(otherPlayer);
		}
	}

	while(game.inputsBuffer.size() > game.n_sentInputs - latestServerGameState.n_handledInputs){
		game.inputsBuffer.erase(game.inputsBuffer.begin());
	}
#endif

	//simulate inputs on client
	for(int i = 0; i < game.inputsBuffer.size(); i++){

		Inputs inputs = game.inputsBuffer[i];

		//control player based on inputs
		Vec2f cameraDirectionXZ = getVec2f(inputs.cameraDirection.x, inputs.cameraDirection.z);
		Vec2f_normalize(&cameraDirectionXZ);

		float speed = PLAYER_SPEED;
		if(inputs.crouch == 1){
			speed = PLAYER_CROUCH_SPEED;
		}

		if(inputs.forwards == 1){
			game.player.velocity.x += cameraDirectionXZ.x * speed;
			game.player.velocity.z += cameraDirectionXZ.y * speed;
		}
		if(inputs.backwards == 1){
			game.player.velocity.x += -cameraDirectionXZ.x * speed;
			game.player.velocity.z += -cameraDirectionXZ.y * speed;
		}
		if(inputs.left == 1){
			Vec3f left = getCrossVec3f(inputs.cameraDirection, getVec3f(0, 1.0, 0));
			Vec3f_normalize(&left);
			game.player.velocity.x += left.x * speed;
			game.player.velocity.z += left.z * speed;
		}
		if(inputs.right == 1){
			Vec3f right = getCrossVec3f(getVec3f(0, 1.0, 0), inputs.cameraDirection);
			Vec3f_normalize(&right);
			game.player.velocity.x += right.x * speed;
			game.player.velocity.z += right.z * speed;
		}
		if(inputs.jump == 1
		&& game.player.onGround){
			game.player.velocity.y += PLAYER_JUMP_SPEED;
		}
		if(inputs.jump == 0
		&& game.player.velocity.y > 0.0){
			game.player.velocity.y *= 0.9;
		}
		if(inputs.crouch == 1){
			if(game.player.height > PLAYER_HEIGHT_CROUCHING){
				game.player.height -= PLAYER_HEIGHT_SPEED;
			}
		}else{
			if(game.player.height < PLAYER_HEIGHT_STANDING){
				game.player.height += PLAYER_HEIGHT_SPEED * 1.2;
			}
		}
		if(inputs.shoot == 1){

			for(int i = 0; i < game.otherPlayers.size(); i++){
				
				Player *player_p = &game.otherPlayers[i];

				TriangleMesh *triangleMesh_p = Game_getTriangleMeshPointerByName(&game, "cube");

				Vec3f intersectionPoint;
				Vec3f intersectionNormal;
				float intersectionDistance = -1.0;

				for(int j = 0; j < triangleMesh_p->n_triangles; j++){

					Vec3f checkPoint;

					if(checkLineToTriangleIntersectionVec3f(cameraPos, cameraPos + cameraDirection, player_p->pos + triangleMesh_p->triangles[j * 3 + 0], player_p->pos + triangleMesh_p->triangles[j * 3 + 1], player_p->pos + triangleMesh_p->triangles[j * 3 + 2], &checkPoint)
					&& (intersectionDistance < 0.0
					|| length(checkPoint - cameraPos) < intersectionDistance)){
						intersectionPoint = checkPoint;
						intersectionNormal = cross(triangleMesh_p->triangles[j * 3 + 0] - triangleMesh_p->triangles[j * 3 + 1], triangleMesh_p->triangles[j * 3 + 0] - triangleMesh_p->triangles[j * 3 + 2]);
						intersectionDistance = length(checkPoint - cameraPos);
					}
				}

				intersectionNormal = normalize(intersectionNormal);

				if(intersectionDistance >= 0.0){

					int n_particles = 17 + getRandom() * 5;

					for(int i = 0; i < n_particles; i++){

						Particle particle;
						particle.pos = intersectionPoint;
						particle.velocity = intersectionNormal * 0.1;
						particle.velocity.y += 0.1;
						particle.velocity += normalize(getVec3f(getRandom() - 0.5, getRandom() - 0.5, getRandom() - 0.5)) * 0.05;
						particle.scale = 0.1 * (0.8 + 0.4 * getRandom());

						game.particles.push_back(particle);

					}

				}

			}

		}

		//handle player physics
		game.player.velocity.y += -PLAYER_GRAVITY;

		game.player.velocity.x *= PLAYER_WALK_RESISTANCE;
		game.player.velocity.z *= PLAYER_WALK_RESISTANCE;

		Vec3f_add(&game.player.pos, game.player.velocity);

		game.player.onGround = false;

		//handle collision between player and obstacles
		for(int i = 0; i < game.obstacles.size(); i++){

			Obstacle *obstacle_p = &game.obstacles[i];
			TriangleMesh *triangleMesh_p = &game.triangleMeshes[obstacle_p->triangleMeshIndex];

			for(int j = 0; j < triangleMesh_p->n_triangles; j++){

				Vec3f triangle1 = triangleMesh_p->triangles[j * 3 + 0];
				Vec3f triangle2 = triangleMesh_p->triangles[j * 3 + 1];
				Vec3f triangle3 = triangleMesh_p->triangles[j * 3 + 2];

				Vec3f_mulByFloat(&triangle1, obstacle_p->scale);
				Vec3f_mulByFloat(&triangle2, obstacle_p->scale);
				Vec3f_mulByFloat(&triangle3, obstacle_p->scale);

				Vec3f_add(&triangle1, obstacle_p->pos);
				Vec3f_add(&triangle2, obstacle_p->pos);
				Vec3f_add(&triangle3, obstacle_p->pos);

				//printf("%i\n", i);
				//Vec3f_log(triangle1);
				//Vec3f_log(triangle2);
				//Vec3f_log(triangle3);

				Vec3f up = getVec3f(0.0, 1.0, 0.0);

				Vec3f u = getSubVec3f(triangle2, triangle1);
				Vec3f v = getSubVec3f(triangle3, triangle1);
				Vec3f N = getCrossVec3f(u, v);
				Vec3f_normalize(&N);

				float r = 0.2;
				Vec3f playerFeetPos = game.player.pos;
				playerFeetPos.y += r;

				if(getDotVec3f(up, N) > 0.7){

					Vec3f intersectionPoint;
					bool hit = checkLineToTriangleIntersectionVec3f(game.player.pos, getAddVec3f(game.player.pos, up), triangle1, triangle2, triangle3, &intersectionPoint);

					if(hit
					&& intersectionPoint.y > game.player.pos.y
					&& intersectionPoint.y < game.player.lastPos.y + r){
						game.player.pos.y = intersectionPoint.y;
						game.player.velocity.y = 0.0;
						game.player.onGround = true;
					}

				}else{

					bool hit = checkSphereToTriangleCollision(playerFeetPos, r, triangle1, triangle2, triangle3);

					if(hit){

						float distance = r - fabs((getDotVec3f(playerFeetPos, N) - getDotVec3f(triangle1, N)) / getDotVec3f(N, N));

						Vec3f_add(&game.player.pos, getMulVec3fFloat(N, distance));
						Vec3f_add(&game.player.velocity, getMulVec3fFloat(N, distance));
						//Vec3f_add(&game.player.velocity, N);
					}
				
				}

			}

		}

	}

	//PERFORMANCE TEST CAMERA
	//game.player.pos = getVec3f(90.0, 5.0, 20.0);
	//cameraDirection = getVec3f(-0.9, -0.33, 0.26);

#ifdef RUN_OFFLINE
		game.inputsBuffer.clear();
#endif
	//Game_simulateInputsOnClient(&game, inputs);

	//handle camera movement
	/*
	Vec2f cameraDirectionXZ = getVec2f(cameraDirection.x, cameraDirection.z);
	Vec2f_normalize(&cameraDirectionXZ);

	if(Engine_keys[ENGINE_KEY_W].down){
		game.player.velocity.x += cameraDirectionXZ.x * PLAYER_SPEED;
		game.player.velocity.z += cameraDirectionXZ.y * PLAYER_SPEED;
	}
	if(Engine_keys[ENGINE_KEY_S].down){
		game.player.velocity.x += -cameraDirectionXZ.x * PLAYER_SPEED;
		game.player.velocity.z += -cameraDirectionXZ.y * PLAYER_SPEED;
	}
	if(Engine_keys[ENGINE_KEY_A].down){
		Vec3f left = getCrossVec3f(cameraDirection, getVec3f(0, 1.0, 0));
		Vec3f_normalize(&left);
		game.player.velocity.x += left.x * PLAYER_SPEED;
		game.player.velocity.z += left.z * PLAYER_SPEED;
	}
	if(Engine_keys[ENGINE_KEY_D].down){
		Vec3f right = getCrossVec3f(getVec3f(0, 1.0, 0), cameraDirection);
		Vec3f_normalize(&right);
		game.player.velocity.x += right.x * PLAYER_SPEED;
		game.player.velocity.z += right.z * PLAYER_SPEED;
	}

	if(Engine_keys[ENGINE_KEY_SPACE].down
	&& game.player.onGround){
		game.player.velocity.y += PLAYER_JUMP_SPEED;
	}

	if(!Engine_keys[ENGINE_KEY_SPACE].down
	&& game.player.velocity.y > 0.0){
		game.player.velocity.y *= 0.9;
	}

	game.player.velocity.x *= PLAYER_WALK_RESISTANCE;
	game.player.velocity.z *= PLAYER_WALK_RESISTANCE;

	if(Engine_keys[ENGINE_KEY_CONTROL].down){
		if(game.player.height > PLAYER_HEIGHT_CROUCHING){
			game.player.height -= PLAYER_HEIGHT_SPEED;
		}
	}else{
		if(game.player.height < PLAYER_HEIGHT_STANDING){
			game.player.height += PLAYER_HEIGHT_SPEED * 1.2;
		}
	}

	game.player.velocity.y += -PLAYER_GRAVITY;

	game.player.lastPos = game.player.pos;
	Vec3f_add(&game.player.pos, game.player.velocity);

	game.player.onGround = false;

	if(game.player.pos.y < 0.0){
		game.player.pos.y = 0.0;
		game.player.velocity.y = 0.0;
		game.player.onGround = true;
	}

	//handle collision between player and obstacles
	for(int i = 0; i < game.obstacles.size(); i++){

		Obstacle *obstacle_p = &game.obstacles[i];
		TriangleMesh *triangleMesh_p = &game.triangleMeshes[obstacle_p->triangleMeshIndex];

		for(int j = 0; j < triangleMesh_p->n_triangles; j++){

			Vec3f triangle1 = triangleMesh_p->triangles[j * 3 + 0];
			Vec3f triangle2 = triangleMesh_p->triangles[j * 3 + 1];
			Vec3f triangle3 = triangleMesh_p->triangles[j * 3 + 2];

			Vec3f_mulByFloat(&triangle1, obstacle_p->scale);
			Vec3f_mulByFloat(&triangle2, obstacle_p->scale);
			Vec3f_mulByFloat(&triangle3, obstacle_p->scale);

			Vec3f_add(&triangle1, obstacle_p->pos);
			Vec3f_add(&triangle2, obstacle_p->pos);
			Vec3f_add(&triangle3, obstacle_p->pos);

			//printf("%i\n", i);
			//Vec3f_log(triangle1);
			//Vec3f_log(triangle2);
			//Vec3f_log(triangle3);

			Vec3f up = getVec3f(0.0, 1.0, 0.0);

			Vec3f u = getSubVec3f(triangle2, triangle1);
			Vec3f v = getSubVec3f(triangle3, triangle1);
			Vec3f N = getCrossVec3f(u, v);
			Vec3f_normalize(&N);

			float r = 0.2;
			Vec3f playerFeetPos = game.player.pos;
			playerFeetPos.y += r;

			if(getDotVec3f(up, N) > 0.7){

				Vec3f intersectionPoint;
				bool hit = checkLineToTriangleIntersectionVec3f(game.player.pos, getAddVec3f(game.player.pos, up), triangle1, triangle2, triangle3, &intersectionPoint);

				if(hit
				&& intersectionPoint.y > game.player.pos.y
				&& intersectionPoint.y < game.player.lastPos.y + r){
					game.player.pos.y = intersectionPoint.y;
					game.player.velocity.y = 0.0;
					game.player.onGround = true;
				}

			}else{

				bool hit = checkSphereToTriangleCollision(playerFeetPos, r, triangle1, triangle2, triangle3);

				if(hit){

					float distance = r - fabs((getDotVec3f(playerFeetPos, N) - getDotVec3f(triangle1, N)) / getDotVec3f(N, N));

					Vec3f_add(&game.player.pos, getMulVec3fFloat(N, distance));
					Vec3f_add(&game.player.velocity, getMulVec3fFloat(N, distance));
					//Vec3f_add(&game.player.velocity, N);
				}
			
			}

		}

	}
*/

	cameraPos = game.player.pos;
	cameraPos.y += game.player.height;

	//update particles
	for(int i = 0; i < game.particles.size(); i++){
		
		Particle *particle_p = &game.particles[i];

		particle_p->velocity.y -= BLOOD_PARTICLE_GRAVITY;

		particle_p->pos += particle_p->velocity;

		//check distance to ground
		TriangleMesh *triangleMesh_p = Game_getTriangleMeshPointerByName(&game, "terrain");

		Vec3f intersectionPoint;
		checkClosestLineTriangleMeshIntersection(particle_p->pos, getVec3f(0.0, -1.0, 0.0), *triangleMesh_p, getVec3f(0.0, 0.0, 0.0), TERRAIN_SCALE, &intersectionPoint, NULL);

		float distanceToGround = length(particle_p->pos - intersectionPoint);

		float hitHeight = 1.0;

		if(distanceToGround < hitHeight){

			Vec2f paintMapPos = getVec2f(particle_p->pos.x * PAINT_MAP_WIDTH, particle_p->pos.z * PAINT_MAP_HEIGHT) / PAINT_MAP_SCALE;

			int index = (int)paintMapPos.y * PAINT_MAP_WIDTH + (int)paintMapPos.x;

			paintMap[index] = (unsigned char)((255.0 * particle_p->pos.y) / 5.0);

			game.particles.erase(game.particles.begin() + i);
			i--;
			continue;
		}

	}

	//sort grass positions
	int n_buckets = 200;
	int bucketSizes[n_buckets];
	int bucketOffsets[n_buckets];
	int bucketCounts[n_buckets];
	memset(bucketSizes, 0, sizeof(int) * n_buckets);
	memset(bucketOffsets, 0, sizeof(int) * n_buckets);
	memset(bucketCounts, 0, sizeof(int) * n_buckets);

	unsigned char depths[grassPositions.size()];
	memset(depths, 0, sizeof(unsigned char) * grassPositions.size());

	Mat4f cameraMatrix = getLookAtMat4f(cameraPos, cameraDirection);

	for(int i = 0; i < grassPositions.size(); i++){

		Vec4f projectedPos = cameraMatrix * grassPositions[i];

		unsigned char depth = (unsigned char)fabs(projectedPos.z);

		depths[i] = depth * (depth < n_buckets);

		bucketSizes[depths[i]]++;

	}

	for(int i = 1; i < n_buckets; i++){
		bucketOffsets[i] = bucketOffsets[i - 1] + bucketSizes[i - 1];
	}

	for(int i = 0; i < grassPositions.size(); i++){

		sortedGrassPositions[bucketOffsets[depths[i]] + bucketCounts[depths[i]]] = grassPositions[i];
		bucketCounts[depths[i]]++;

	}


}

void Engine_draw(){

	glBindTexture(GL_TEXTURE_2D, paintMapTexture.ID);
	glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, PAINT_MAP_WIDTH, PAINT_MAP_HEIGHT, GL_RED, GL_UNSIGNED_BYTE, paintMap);

	TextureBuffer_update(&grassPositionsTextureBuffer, 0, grassPositions.size() * sizeof(Vec4f), sortedGrassPositions);

	bool renderFromLightPerspective = false;
	for(int renderStage = 0; renderStage < N_RENDER_STAGES; renderStage++){

		if(renderStage == RENDER_STAGE_BIG_SHADOWS){
			glViewport(0.0, 0.0, BIG_SHADOW_MAP_WIDTH, BIG_SHADOW_MAP_HEIGHT);
			glBindFramebuffer(GL_FRAMEBUFFER, bigShadowMapFBO);
			//glColorMask(true, true, true, true);
			glColorMask(false, false, false, false);
			glDepthMask(true);
			//glViewport(0.0, 0.0, Engine_clientWidth, Engine_clientHeight);
			//glBindFramebuffer(GL_FRAMEBUFFER, 0);  
		}else{
			//continue;
		}

		if(renderStage == RENDER_STAGE_GRASS_SHADOWS){
			glViewport(0.0, 0.0, GRASS_SHADOW_MAP_WIDTH, GRASS_SHADOW_MAP_HEIGHT);
			glBindFramebuffer(GL_FRAMEBUFFER, grassShadowMapFBO);
			glColorMask(false, false, false, false);
			glDepthMask(true);
		}
		if(renderStage == RENDER_STAGE_SHADOWS){
			glViewport(0.0, 0.0, SHADOW_MAP_WIDTH, SHADOW_MAP_HEIGHT);
			glBindFramebuffer(GL_FRAMEBUFFER, shadowMapFBO);
			glColorMask(false, false, false, false);
			//glColorMask(true, true, true, true);
			glDepthMask(true);
			//glViewport(0.0, 0.0, Engine_clientWidth, Engine_clientHeight);
			//glBindFramebuffer(GL_FRAMEBUFFER, 0);  
		}else{
			//continue;
		}
		if(renderStage == RENDER_STAGE_SCENE_DEPTH){
			continue;
			glViewport(0.0, 0.0, Engine_clientWidth, Engine_clientHeight);
			glBindFramebuffer(GL_FRAMEBUFFER, 0);  
			//glColorMask(false, false, false, false);
			glColorMask(true, true, true, true);
			//glDepthMask(true);
		}
		if(renderStage == RENDER_STAGE_SCENE){
			glViewport(0.0, 0.0, Engine_clientWidth, Engine_clientHeight);
			glBindFramebuffer(GL_FRAMEBUFFER, 0);  
			glColorMask(true, true, true, true);
			//glDepthMask(false);
		}

		//printf("%i\n", renderStage);

		glClearColor(0.5, 0.7, 0.9, 1.0);
		glClearDepth(1.0);

		if(renderStage == RENDER_STAGE_SCENE){
			glClear(GL_COLOR_BUFFER_BIT);
			glClear(GL_DEPTH_BUFFER_BIT);
			//glClear(GL_DEPTH_BUFFER_BIT);
		}else{
			//printf("cleared buffer\n");
			glClear(GL_COLOR_BUFFER_BIT);
			glClear(GL_DEPTH_BUFFER_BIT);
		}


		Mat4f viewMatrix = getLookAtMat4f(cameraPos, cameraDirection);
		viewMatrix *= getPerspectiveMat4f(fov, (float)Engine_clientWidth / (float)Engine_clientHeight, farPlane, nearPlane);

		//Mat4f perspectiveMatrix = getPerspectiveMat4f(fov, (float)Engine_clientWidth / (float)Engine_clientHeight, farPlane, nearPlane);

		//cameraMatrix *= perspectiveMatrix;

		//perspectiveMatrix = getIdentityMat4f();

		Mat4f lightViewMatrix = getTranslationMat4f(round(-game.player.pos.x), round(-game.player.pos.y), round(-game.player.pos.z));
		lightViewMatrix *= getTranslationMat4f(getVec3f(lightDirection.x, 0.0, lightDirection.z) * lightPos.y);
		lightViewMatrix *= getTranslationMat4f(getVec3f(round(-cameraDirection.x * shadowMapScale), 0.0, round(-cameraDirection.z * shadowMapScale)));
		lightViewMatrix *= getLookAtMat4f(lightPos, lightDirection);
		lightViewMatrix *= getOrthographicMat4f(shadowMapScale, 1.0, farPlane, nearPlane);

		//Mat4f lightPerspectiveMatrix = getOrthographicMat4f(shadowMapScale, 1.0, farPlane, nearPlane);

		//lightCameraMatrix *= lightPerspectiveMatrix;
		//lightPerspectiveMatrix = getIdentityMat4f();

		Mat4f bigLightViewMatrix = getTranslationMat4f(getVec3f(-17.0, 0.0, -22.0));
		bigLightViewMatrix *= getLookAtMat4f(lightPos, lightDirection);
		bigLightViewMatrix *= getOrthographicMat4f(bigShadowMapScale, 1.0, farPlane, nearPlane);

		//Mat4f bigLightPerspectiveMatrix = getOrthographicMat4f(bigShadowMapScale, 1.0, farPlane, nearPlane);

		//bigLightCameraMatrix *= bigLightPerspectiveMatrix;
		//bigLightPerspectiveMatrix = getIdentityMat4f();

		if(renderStage == RENDER_STAGE_BIG_SHADOWS){
			viewMatrix = bigLightViewMatrix;
			//cameraMatrix = bigLightCameraMatrix;
			//perspectiveMatrix = bigLightPerspectiveMatrix;
		}

		if(renderStage == RENDER_STAGE_GRASS_SHADOWS
		|| renderStage == RENDER_STAGE_SHADOWS){
			viewMatrix = lightViewMatrix;
			//cameraMatrix = lightCameraMatrix;
			//perspectiveMatrix = lightPerspectiveMatrix;
		}

		//Mat4f frustumMatrix = cameraMatrix;

		//cull bounding boxes
		for(int i = 0; i < game.boundingBoxesCulled.size(); i++){
			game.boundingBoxesCulled[i] = false;
		}
		/*
		for(int i = 0; i < game.boundingBoxes.size(); i++){

			Box *boundingBox_p = &game.boundingBoxes[i];

			if(cameraPos.x > boundingBox_p->pos.x && cameraPos.x < boundingBox_p->pos.x + boundingBox_p->size.x
			&& cameraPos.z > boundingBox_p->pos.z && cameraPos.z < boundingBox_p->pos.z + boundingBox_p->size.z){
				game.boundingBoxesCulled[i] = false;
				continue;
			}

			Vec4f positions[8] = {
				boundingBox_p->pos.x, boundingBox_p->pos.y, boundingBox_p->pos.z, 1.0,
				boundingBox_p->pos.x + boundingBox_p->size.x, boundingBox_p->pos.y, boundingBox_p->pos.z, 1.0,
				boundingBox_p->pos.x, boundingBox_p->pos.y + boundingBox_p->size.y, boundingBox_p->pos.z, 1.0,
				boundingBox_p->pos.x, boundingBox_p->pos.y, boundingBox_p->pos.z + boundingBox_p->size.z, 1.0,
				boundingBox_p->pos.x + boundingBox_p->size.x, boundingBox_p->pos.y + boundingBox_p->size.y, boundingBox_p->pos.z, 1.0,
				boundingBox_p->pos.x + boundingBox_p->size.x, boundingBox_p->pos.y, boundingBox_p->pos.z + boundingBox_p->size.z, 1.0,
				boundingBox_p->pos.x, boundingBox_p->pos.y + boundingBox_p->size.y, boundingBox_p->pos.z + boundingBox_p->size.z, 1.0,
				boundingBox_p->pos.x + boundingBox_p->size.x, boundingBox_p->pos.y + boundingBox_p->size.y, boundingBox_p->pos.z + boundingBox_p->size.z, 1.0,
			};

			game.boundingBoxesCulled[i] = true;

			for(int j = 0; j < 8; j++){

				Vec4f_mulByMat4f(&positions[j], frustumMatrix);

				if(renderStage == RENDER_STAGE_BIG_SHADOWS
				|| positions[j].z > 0.0
				&& ((renderStage == RENDER_STAGE_SHADOWS || renderStage == RENDER_STAGE_GRASS_SHADOWS)
				&& fabs(positions[j].x) < shadowMapScale && fabs(positions[j].y) < shadowMapScale
				|| (renderStage == RENDER_STAGE_SCENE
				|| renderStage == RENDER_STAGE_SCENE_DEPTH)
				&& fabs(positions[j].x) < positions[j].z)){
					game.boundingBoxesCulled[i] = false;
					break;
				}

			}

		}
		*/

		//draw obstacles
		if(renderStage == RENDER_STAGE_BIG_SHADOWS
		|| renderStage == RENDER_STAGE_SHADOWS
		|| renderStage == RENDER_STAGE_SCENE_DEPTH
		|| renderStage == RENDER_STAGE_SCENE){
			for(int i = 0; i < game.obstacles.size(); i++){

				Obstacle *obstacle_p = &game.obstacles[i];

				float scale = obstacle_p->scale;

				Mat4f modelMatrix = getIdentityMat4f();

				modelMatrix *= getScalingMat4f(scale);

				modelMatrix *= getTranslationMat4f(obstacle_p->pos);

				Shader *shader_p = Game_getShaderPointerByName(&game, "model");
				if(renderStage == RENDER_STAGE_SHADOWS
				|| renderStage == RENDER_STAGE_BIG_SHADOWS
				|| renderStage == RENDER_STAGE_SCENE_DEPTH){
					shader_p = Game_getShaderPointerByName(&game, "model-shadow");
				}

				glUseProgram(shader_p->ID);
				
				Model *model_p = &game.models[obstacle_p->modelIndex];

				Texture *texture_p = &game.textures[obstacle_p->textureIndex];
				Texture *alphaTexture_p = &game.textures[obstacle_p->alphaTextureIndex];

				glBindBuffer(GL_ARRAY_BUFFER, model_p->VBO);
				glBindVertexArray(model_p->VAO);

				GL3D_uniformFloat(shader_p->ID, "grassShadowStrength", GRASS_SHADOW_STRENGTH);

				GL3D_uniformTexture(shader_p->ID, "colorTexture", 0, texture_p->ID);
				GL3D_uniformTexture(shader_p->ID, "alphaTexture", 1, alphaTexture_p->ID);
				GL3D_uniformTexture(shader_p->ID, "shadowMapDepthTexture", 2, shadowMapDepthTexture.ID);
				GL3D_uniformTexture(shader_p->ID, "grassShadowMapDepthTexture", 3, grassShadowMapDepthTexture.ID);
				GL3D_uniformTexture(shader_p->ID, "bigShadowMapDepthTexture", 4, bigShadowMapDepthTexture.ID);

				GL3D_uniformMat4f(shader_p->ID, "viewMatrix", viewMatrix);
				GL3D_uniformMat4f(shader_p->ID, "lightViewMatrix", lightViewMatrix);
				GL3D_uniformMat4f(shader_p->ID, "bigLightViewMatrix", bigLightViewMatrix);
				GL3D_uniformMat4f(shader_p->ID, "modelMatrix", modelMatrix);
				/*
				GL3D_uniformMat4f(shader_p->ID, "perspectiveMatrix", perspectiveMatrix);
				GL3D_uniformMat4f(shader_p->ID, "cameraMatrix", cameraMatrix);
				GL3D_uniformMat4f(shader_p->ID, "lightPerspectiveMatrix", lightPerspectiveMatrix);
				GL3D_uniformMat4f(shader_p->ID, "lightCameraMatrix", lightCameraMatrix);
				GL3D_uniformMat4f(shader_p->ID, "bigLightPerspectiveMatrix", bigLightPerspectiveMatrix);
				GL3D_uniformMat4f(shader_p->ID, "bigLightCameraMatrix", bigLightCameraMatrix);
				*/

				GL3D_uniformVec4f(shader_p->ID, "inputColor", obstacle_p->color);

				glDrawArrays(GL_TRIANGLES, 0, model_p->numberOfTriangles * 3);

				
			}
		}

		//draw grass
		if((renderStage == RENDER_STAGE_GRASS_SHADOWS
		|| renderStage == RENDER_STAGE_SCENE) && true){
			glDisable(GL_CULL_FACE);

			Vec3f pos = getVec3f(0.0, 4.0, 0.0);
			float scale = 1.0;

			Shader *shader_p = Game_getShaderPointerByName(&game, "grass");
			if(renderStage == RENDER_STAGE_GRASS_SHADOWS){
				shader_p = Game_getShaderPointerByName(&game, "grass-shadow");
			}

			glUseProgram(shader_p->ID);

			Texture *texture_p = Game_getTexturePointerByName(&game, "grass");
			Texture *alphaTexture_p = Game_getTexturePointerByName(&game, "grass-alpha");
			//Texture *texture_p = Game_getTexturePointerByName(&game, "small-grass");
			//Texture *alphaTexture_p = Game_getTexturePointerByName(&game, "small-grass-alpha");

			Model *model_p = Game_getModelPointerByName(&game, "quad");

			glBindBuffer(GL_ARRAY_BUFFER, model_p->VBO);
			glBindVertexArray(model_p->VAO);

			GL3D_uniformTexture(shader_p->ID, "colorTexture", 0, texture_p->ID);
			GL3D_uniformTexture(shader_p->ID, "alphaTexture", 1, alphaTexture_p->ID);
			GL3D_uniformTextureBuffer(shader_p->ID, "grassPositions", 2, grassPositionsTextureBuffer.TB);
			GL3D_uniformTexture(shader_p->ID, "shadowMapDepthTexture", 3, shadowMapDepthTexture.ID);
			GL3D_uniformTexture(shader_p->ID, "grassShadowMapDepthTexture", 4, grassShadowMapDepthTexture.ID);
			GL3D_uniformTexture(shader_p->ID, "bigShadowMapDepthTexture", 5, bigShadowMapDepthTexture.ID);
			GL3D_uniformTexture(shader_p->ID, "paintMapTexture", 6, paintMapTexture.ID);

			GL3D_uniformMat4f(shader_p->ID, "viewMatrix", viewMatrix);
			GL3D_uniformMat4f(shader_p->ID, "lightViewMatrix", lightViewMatrix);
			GL3D_uniformMat4f(shader_p->ID, "bigLightViewMatrix", bigLightViewMatrix);

			GL3D_uniformFloat(shader_p->ID, "windTime", windTime);

			GL3D_uniformFloat(shader_p->ID, "shadowStrength", 0.3);
			GL3D_uniformFloat(shader_p->ID, "grassShadowStrength", GRASS_SHADOW_STRENGTH);

			GL3D_uniformInt(shader_p->ID, "n_grassPositions", grassPositions.size());

			Vec2f cameraPos2 = getVec2f(cameraPos.x, cameraPos.z);
			GL3D_uniformVec2f(shader_p->ID, "cameraPos", cameraPos2);

			float rotation = 0.0;

			Mat2f rotationMatrix1 = getRotationMat2f(rotation);
			Mat2f rotationMatrix2 = getRotationMat2f(rotation + M_PI * (1.0 / 3.0));
			//Mat2f rotationMatrix2 = getRotationMat2f(rotation + M_PI / 2.0);
			Mat2f rotationMatrix3 = getRotationMat2f(rotation + M_PI * (2.0 / 3.0));

			GL3D_uniformMat2f(shader_p->ID, "rotationMatrix", rotationMatrix1);

			glDrawArraysInstanced(GL_TRIANGLES, 0, model_p->numberOfTriangles * 3, grassPositions.size());

			GL3D_uniformMat2f(shader_p->ID, "rotationMatrix", rotationMatrix2);

			glDrawArraysInstanced(GL_TRIANGLES, 0, model_p->numberOfTriangles * 3, grassPositions.size());

			GL3D_uniformMat2f(shader_p->ID, "rotationMatrix", rotationMatrix3);

			glDrawArraysInstanced(GL_TRIANGLES, 0, model_p->numberOfTriangles * 3, grassPositions.size());

			glEnable(GL_CULL_FACE);
		}

		//draw leaves
		if((renderStage == RENDER_STAGE_BIG_SHADOWS
		|| renderStage == RENDER_STAGE_SHADOWS
		|| renderStage == RENDER_STAGE_SCENE) && true){

			glDisable(GL_CULL_FACE);

			for(int i = 0; i < game.trees.size(); i++){

				Tree *tree_p = &game.trees[i];

				Shader *shader_p = Game_getShaderPointerByName(&game, "leaf");
				if(renderStage == RENDER_STAGE_SHADOWS
				|| renderStage == RENDER_STAGE_BIG_SHADOWS){
					shader_p = Game_getShaderPointerByName(&game, "leaf-shadow");
				}

				glUseProgram(shader_p->ID);

				Texture *texture_p = Game_getTexturePointerByName(&game, "leaves");
				Texture *alphaTexture_p = Game_getTexturePointerByName(&game, "leaves-alpha");

				Model *model_p = Game_getModelPointerByName(&game, "quad");

				glBindBuffer(GL_ARRAY_BUFFER, model_p->VBO);
				glBindVertexArray(model_p->VAO);

				GL3D_uniformTexture(shader_p->ID, "colorTexture", 0, texture_p->ID);
				GL3D_uniformTexture(shader_p->ID, "alphaTexture", 1, alphaTexture_p->ID);
				GL3D_uniformTexture(shader_p->ID, "shadowMapDepthTexture", 2, shadowMapDepthTexture.ID);
				GL3D_uniformTexture(shader_p->ID, "grassShadowMapDepthTexture", 3, grassShadowMapDepthTexture.ID);
				GL3D_uniformTexture(shader_p->ID, "bigShadowMapDepthTexture", 4, bigShadowMapDepthTexture.ID);
				GL3D_uniformTextureBuffer(shader_p->ID, "modelTransformationsBuffer", 5, tree_p->leafTransformationsTextureBuffer.TB);

				GL3D_uniformFloat(shader_p->ID, "windTime", windTime);

				GL3D_uniformMat4f(shader_p->ID, "viewMatrix", viewMatrix);
				GL3D_uniformMat4f(shader_p->ID, "lightViewMatrix", lightViewMatrix);
				GL3D_uniformMat4f(shader_p->ID, "bigLightViewMatrix", bigLightViewMatrix);

				/*
				GL3D_uniformMat4f(shader_p->ID, "perspectiveMatrix", perspectiveMatrix);
				GL3D_uniformMat4f(shader_p->ID, "cameraMatrix", cameraMatrix);
				GL3D_uniformMat4f(shader_p->ID, "lightPerspectiveMatrix", lightPerspectiveMatrix);
				GL3D_uniformMat4f(shader_p->ID, "lightCameraMatrix", lightCameraMatrix);
				GL3D_uniformMat4f(shader_p->ID, "bigLightPerspectiveMatrix", bigLightPerspectiveMatrix);
				GL3D_uniformMat4f(shader_p->ID, "bigLightCameraMatrix", bigLightCameraMatrix);
				*/

				glDrawArraysInstanced(GL_TRIANGLES, 0, model_p->numberOfTriangles * 3, tree_p->leafTransformationsTextureBuffer.n_elements);

			}

			glEnable(GL_CULL_FACE);
		
		}

		float t = (1.0 + sin(gameTime * 0.1)) * 0.5;

		std::vector<Bone> interpolatedBones = getInterpolatedBones(game.boneModels[0].bones, game.boneModels[1].bones, t);

		std::vector<Mat4f> bindMatrices = getBindMatricesFromBones(interpolatedBones);

		std::vector<Mat4f> boneTransformations;

		for(int i = 0; i < bindMatrices.size(); i++){
			Mat4f transformation = game.boneModels[0].inverseBindMatrices[i];
			transformation *= bindMatrices[i];
			boneTransformations.push_back(transformation);
		}

		/*
		//draw bone model
		if(renderStage == RENDER_STAGE_SCENE){

			Vec3f pos = getVec3f(30.0, 3.0, 30.0);
			float scale = 0.5;

			Mat4f modelMatrix = getIdentityMat4f();

			modelMatrix *= getScalingMat4f(scale);

			modelMatrix *= getTranslationMat4f(pos);

			//unsigned int currentShaderProgram = game.boneModelShader;
			Shader *shader_p = Game_getShaderPointerByName(&game, "bone");
			//Shader *shader_p = Game_getShaderPointerByName(&game, "bone-shadow");

			glUseProgram(shader_p->ID);

			BoneModel *model_p = &game.boneModels[0];

			glBindBuffer(GL_ARRAY_BUFFER, model_p->VBO);
			glBindVertexArray(model_p->VAO);

			GL3D_uniformMat4f(shader_p->ID, "modelMatrix", modelMatrix);
			GL3D_uniformMat4f(shader_p->ID, "perspectiveMatrix", perspectiveMatrix);
			GL3D_uniformMat4f(shader_p->ID, "cameraMatrix", cameraMatrix);

			GL3D_uniformVec4f(shader_p->ID, "inputColor", getVec4f(0.7, 0.7, 0.7, 1.0));

			GL3D_uniformMat4fArray(shader_p->ID, "boneTransformations", &boneTransformations[0], boneTransformations.size());

			GL3D_uniformFloat(shader_p->ID, "shadowFactor", 0.0);

			glDrawArrays(GL_TRIANGLES, 0, model_p->n_triangles * 3);

		}
		*/

		/*
		//draw bounding boxes
		if(renderStage == 1 && false){

			glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
			glDisable(GL_CULL_FACE);
			glLineWidth(5.0);

			for(int i = 0; i < game.boundingBoxes.size(); i++){

				Box *boundingBox_p = &game.boundingBoxes[i];

				Mat4f modelMatrix = getIdentityMat4f();

				modelMatrix *= getTranslationMat4f(1.0, 1.0, 1.0);

				modelMatrix *= getScalingMat4f(boundingBox_p->size);

				modelMatrix *= getTranslationMat4f(boundingBox_p->pos);

				//unsigned int currentShaderProgram = game.modelShader;
				Shader *shader_p = Game_getShaderPointerByName(&game, "model");

				glUseProgram(shader_p->ID);
				
				Model *model_p = Game_getModelPointerByName(&game, "cube");

				glBindBuffer(GL_ARRAY_BUFFER, model_p->VBO);
				glBindVertexArray(model_p->VAO);

				GL3D_uniformMat4f(shader_p->ID, "modelMatrix", modelMatrix);
				GL3D_uniformMat4f(shader_p->ID, "perspectiveMatrix", perspectiveMatrix);
				GL3D_uniformMat4f(shader_p->ID, "cameraMatrix", cameraMatrix);

				GL3D_uniformVec4f(shader_p->ID, "inputColor", getVec4f(1.0, 0.0, 0.0, 1.0));

				glDrawArrays(GL_TRIANGLES, 0, model_p->numberOfTriangles * 3);
				
			}

			glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
			glEnable(GL_CULL_FACE);

		}
		*/

		/*
		//draw camera
		if(renderFromLightPerspective){

			float scale = 1.0;

			Mat4f modelMatrix = getIdentityMat4f();

			modelMatrix *= getScalingMat4f(scale);

			modelMatrix *= getTranslationMat4f(cameraPos);

			//unsigned int currentShaderProgram = game.modelShader;
			Shader *shader_p = Game_getShaderPointerByName(&game, "model");

			glUseProgram(shader_p->ID);
			
			Model *model_p = &game.models[Game_getModelIndexByName(&game, "cube")];

			glBindBuffer(GL_ARRAY_BUFFER, model_p->VBO);
			glBindVertexArray(model_p->VAO);

			GL3D_uniformTexture(shader_p->ID, "shadowMapDepthTexture", 0, shadowMapDepthTexture.ID);
			GL3D_uniformTexture(shader_p->ID, "grassShadowMapDepthTexture", 1, grassShadowMapDepthTexture.ID);

			GL3D_uniformMat4f(shader_p->ID, "modelMatrix", modelMatrix);
			GL3D_uniformMat4f(shader_p->ID, "perspectiveMatrix", perspectiveMatrix);
			GL3D_uniformMat4f(shader_p->ID, "cameraMatrix", cameraMatrix);
			GL3D_uniformMat4f(shader_p->ID, "lightPerspectiveMatrix", lightPerspectiveMatrix);
			GL3D_uniformMat4f(shader_p->ID, "lightCameraMatrix", lightCameraMatrix);

			GL3D_uniformVec4f(shader_p->ID, "inputColor", getVec4f(1.0, 0.0, 0.0, 1.0));

			glDrawArrays(GL_TRIANGLES, 0, model_p->numberOfTriangles * 3);

		}
		*/

		//draw other players
		if(renderStage == RENDER_STAGE_SCENE){

			for(int i = 0; i < game.otherPlayers.size(); i++){

				Player *player_p = &game.otherPlayers[i];

				float scale = 1.0;

				Mat4f modelMatrix = getIdentityMat4f();

				modelMatrix *= getScalingMat4f(scale);

				modelMatrix *= getTranslationMat4f(player_p->pos);

				Shader *shader_p = Game_getShaderPointerByName(&game, "model");
				if(renderStage == RENDER_STAGE_SHADOWS
				|| renderStage == RENDER_STAGE_BIG_SHADOWS
				|| renderStage == RENDER_STAGE_SCENE_DEPTH){
					shader_p = Game_getShaderPointerByName(&game, "model-shadow");
				}

				glUseProgram(shader_p->ID);
				
				Model *model_p = Game_getModelPointerByName(&game, "cube");

				Texture *texture_p = Game_getTexturePointerByName(&game, "blank");
				Texture *alphaTexture_p = Game_getTexturePointerByName(&game, "blank-alpha");

				glBindBuffer(GL_ARRAY_BUFFER, model_p->VBO);
				glBindVertexArray(model_p->VAO);

				GL3D_uniformFloat(shader_p->ID, "grassShadowStrength", GRASS_SHADOW_STRENGTH);

				GL3D_uniformTexture(shader_p->ID, "colorTexture", 0, texture_p->ID);
				GL3D_uniformTexture(shader_p->ID, "alphaTexture", 1, alphaTexture_p->ID);
				GL3D_uniformTexture(shader_p->ID, "shadowMapDepthTexture", 2, shadowMapDepthTexture.ID);
				GL3D_uniformTexture(shader_p->ID, "grassShadowMapDepthTexture", 3, grassShadowMapDepthTexture.ID);
				GL3D_uniformTexture(shader_p->ID, "bigShadowMapDepthTexture", 4, bigShadowMapDepthTexture.ID);

				GL3D_uniformMat4f(shader_p->ID, "viewMatrix", viewMatrix);
				GL3D_uniformMat4f(shader_p->ID, "lightViewMatrix", lightViewMatrix);
				GL3D_uniformMat4f(shader_p->ID, "bigLightViewMatrix", bigLightViewMatrix);
				GL3D_uniformMat4f(shader_p->ID, "modelMatrix", modelMatrix);
				/*
				GL3D_uniformMat4f(shader_p->ID, "perspectiveMatrix", perspectiveMatrix);
				GL3D_uniformMat4f(shader_p->ID, "cameraMatrix", cameraMatrix);
				GL3D_uniformMat4f(shader_p->ID, "lightPerspectiveMatrix", lightPerspectiveMatrix);
				GL3D_uniformMat4f(shader_p->ID, "lightCameraMatrix", lightCameraMatrix);
				GL3D_uniformMat4f(shader_p->ID, "bigLightPerspectiveMatrix", bigLightPerspectiveMatrix);
				GL3D_uniformMat4f(shader_p->ID, "bigLightCameraMatrix", bigLightCameraMatrix);
				*/

				GL3D_uniformVec4f(shader_p->ID, "inputColor", getVec4f(0.5, 0.5, 0.5, 1.0));

				glDrawArrays(GL_TRIANGLES, 0, model_p->numberOfTriangles * 3);

			}
		
		}

		//draw particles
		if(renderStage == RENDER_STAGE_SCENE){

			for(int i = 0; i < game.particles.size(); i++){

				Particle *particle_p = &game.particles[i];

				float scale = particle_p->scale;

				Mat4f modelMatrix = getIdentityMat4f();

				modelMatrix *= getScalingMat4f(scale);

				modelMatrix *= getTranslationMat4f(particle_p->pos);

				Shader *shader_p = Game_getShaderPointerByName(&game, "model");
				if(renderStage == RENDER_STAGE_SHADOWS
				|| renderStage == RENDER_STAGE_BIG_SHADOWS
				|| renderStage == RENDER_STAGE_SCENE_DEPTH){
					shader_p = Game_getShaderPointerByName(&game, "model-shadow");
				}

				glUseProgram(shader_p->ID);
				
				Model *model_p = Game_getModelPointerByName(&game, "cube");

				Texture *texture_p = Game_getTexturePointerByName(&game, "blank");
				Texture *alphaTexture_p = Game_getTexturePointerByName(&game, "blank-alpha");

				glBindBuffer(GL_ARRAY_BUFFER, model_p->VBO);
				glBindVertexArray(model_p->VAO);

				GL3D_uniformFloat(shader_p->ID, "grassShadowStrength", GRASS_SHADOW_STRENGTH);

				GL3D_uniformTexture(shader_p->ID, "colorTexture", 0, texture_p->ID);
				GL3D_uniformTexture(shader_p->ID, "alphaTexture", 1, alphaTexture_p->ID);
				GL3D_uniformTexture(shader_p->ID, "shadowMapDepthTexture", 2, shadowMapDepthTexture.ID);
				GL3D_uniformTexture(shader_p->ID, "grassShadowMapDepthTexture", 3, grassShadowMapDepthTexture.ID);
				GL3D_uniformTexture(shader_p->ID, "bigShadowMapDepthTexture", 4, bigShadowMapDepthTexture.ID);

				GL3D_uniformMat4f(shader_p->ID, "viewMatrix", viewMatrix);
				GL3D_uniformMat4f(shader_p->ID, "lightViewMatrix", lightViewMatrix);
				GL3D_uniformMat4f(shader_p->ID, "bigLightViewMatrix", bigLightViewMatrix);
				GL3D_uniformMat4f(shader_p->ID, "modelMatrix", modelMatrix);
				/*
				GL3D_uniformMat4f(shader_p->ID, "perspectiveMatrix", perspectiveMatrix);
				GL3D_uniformMat4f(shader_p->ID, "cameraMatrix", cameraMatrix);
				GL3D_uniformMat4f(shader_p->ID, "lightPerspectiveMatrix", lightPerspectiveMatrix);
				GL3D_uniformMat4f(shader_p->ID, "lightCameraMatrix", lightCameraMatrix);
				GL3D_uniformMat4f(shader_p->ID, "bigLightPerspectiveMatrix", bigLightPerspectiveMatrix);
				GL3D_uniformMat4f(shader_p->ID, "bigLightCameraMatrix", bigLightCameraMatrix);
				*/

				GL3D_uniformVec4f(shader_p->ID, "inputColor", getVec4f(1.0, 0.0, 0.1, 1.0));

				glDrawArrays(GL_TRIANGLES, 0, model_p->numberOfTriangles * 3);

			}
		
		}

		//draw water
		if(renderStage == RENDER_STAGE_SCENE && true){

			float scale = 100.0;

			Mat4f rotationMatrix = getQuaternionMat4f(getQuaternion(getVec3f(1.0, 0.0, 0.0), 0.0));

			Mat4f scalingMatrix = getScalingMat4f(scale);

			Mat4f translationMatrix = getTranslationMat4f(getVec3f(-scale, 0.0, 0.0));
			//Mat4f translationMatrix = getTranslationMat4f(getVec3f(5.0, 5.0, 5.0));

			Shader *shader_p = Game_getShaderPointerByName(&game, "water");

			glUseProgram(shader_p->ID);

			Model *model_p = &game.models[Game_getModelIndexByName(&game, "water")];

			//Texture *voronoiTexture_p = Game_getTexturePointerByName(&game, "voronoi-smooth-normals");
			Texture *voronoiTexture_p = Game_getTexturePointerByName(&game, "voronoi");
			Texture *normalMap_p = Game_getTexturePointerByName(&game, "water-normals");
			Texture *noiseMap_p = Game_getTexturePointerByName(&game, "ripple-noise");

			glBindBuffer(GL_ARRAY_BUFFER, model_p->VBO);
			glBindVertexArray(model_p->VAO);

			GL3D_uniformTexture(shader_p->ID, "voronoiTexture", 0, voronoiTexture_p->ID);
			GL3D_uniformTexture(shader_p->ID, "normalMap", 1, normalMap_p->ID);
			GL3D_uniformTexture(shader_p->ID, "noiseMap", 2, noiseMap_p->ID);

			GL3D_uniformMat4f(shader_p->ID, "viewMatrix", viewMatrix);
			GL3D_uniformMat4f(shader_p->ID, "rotationMatrix", rotationMatrix);
			GL3D_uniformMat4f(shader_p->ID, "scalingMatrix", scalingMatrix);
			GL3D_uniformMat4f(shader_p->ID, "translationMatrix", translationMatrix);

			GL3D_uniformFloat(shader_p->ID, "inputT", windTime);
			GL3D_uniformFloat(shader_p->ID, "scale", scale);

			GL3D_uniformVec3f(shader_p->ID, "cameraDirection", cameraDirection);

			glDrawArrays(GL_TRIANGLES, 0, model_p->numberOfTriangles * 3);
			
		}
	
	}

	//draw hud
	{
		glDisable(GL_DEPTH_TEST);

		Renderer2D_updateDrawSize(&renderer2D, Engine_clientWidth, Engine_clientHeight);

		//draw hud
		Renderer2D_setShader(&renderer2D, renderer2D.colorShader);

		Renderer2D_setColor(&renderer2D, getVec4f(1.0, 0.0, 1.0, 1.0));

		Renderer2D_drawRectangle(&renderer2D, WIDTH / 2 - 4, HEIGHT / 2 - 4, 8, 8);

		//draw timing information
		if(drawTimings){

			Renderer2D_setShader(&renderer2D, renderer2D.colorShader);

			Renderer2D_setColor(&renderer2D, getVec4f(0.0, 0.0, 0.0, 0.5));

			Renderer2D_drawRectangle(&renderer2D, 50, 50, 400, 225);

			Renderer2D_setShader(&renderer2D, renderer2D.textureColorShader);

			Renderer2D_setColor(&renderer2D, getVec4f(1.0, 1.0, 1.0, 1.0));

			char updateText[STRING_SIZE];
			String_set(updateText, "", STRING_SIZE);
			sprintf(updateText, "update: %.2f ms", Engine_frameUpdateTime);

			char drawText[STRING_SIZE];
			String_set(drawText, "", STRING_SIZE);
			sprintf(drawText, "draw: %.2f ms", Engine_frameDrawTime);

			char totalText[STRING_SIZE];
			String_set(totalText, "", STRING_SIZE);
			sprintf(totalText, "total: %.2f ms", Engine_frameTime);

			Renderer2D_drawText(&renderer2D, updateText, 80, 60, 60, font);

			Renderer2D_drawText(&renderer2D, drawText, 80, 130, 60, font);

			Renderer2D_drawText(&renderer2D, totalText, 80, 200, 60, font);
		
		}
	
		glEnable(GL_DEPTH_TEST);
	}

	//glBindFramebuffer(GL_READ_FRAMEBUFFER, shadowMapFBO);
	//glBlitFramebuffer(0, 0, SHADOW_MAP_WIDTH, SHADOW_MAP_HEIGHT, 0, 0, Engine_clientWidth, Engine_clientHeight, GL_COLOR_BUFFER_BIT, GL_NEAREST);
	//glBindFramebuffer(GL_READ_FRAMEBUFFER, bigShadowMapFBO);
	//glBlitFramebuffer(0, 0, BIG_SHADOW_MAP_WIDTH, BIG_SHADOW_MAP_HEIGHT, 0, 0, Engine_clientWidth, Engine_clientHeight, GL_COLOR_BUFFER_BIT, GL_NEAREST);

	gameTime += 0.1;
	windTime += 0.01;

}

void Engine_finnish(){

}
