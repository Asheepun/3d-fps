#include "engine/shaders.h"
#include "engine/renderer2d.h"
#include "engine/igui.h"

#include "game.h"

#include "stdio.h"
#include "stdlib.h"
#include "math.h"
#include "time.h"
#include <cstring>
#include <vector>

struct TreeNode{
	int parent;
	Vec3f pos;
	float radius;
};

std::vector<Mat4f> leafTransformations;
Vec3f treePos = { 10.0, 0.0, 15.0 };
TextureBuffer leafTransformationsTextureBuffer;
float TREE_SCALE = 1.5;

float gameTime = 0.0;

Game game;

float windTime = 0.0;

//THIS VALUE (N_GRASS) WILL BE MODIFIED DANGER DANGER!!!
//int N_GRASS = 10000;
//TextureBuffer grassPositionsTextureBuffer;
const int GRASS_GRID_WIDTH = 10;
const float GRASS_DISTANCE = 1.0;
//const float GRASS_DISTANCE = 1.5;
TextureBuffer grassPositionsTextureBufferGrid[GRASS_GRID_WIDTH * GRASS_GRID_WIDTH];
int grassBoundingBoxIndices[GRASS_GRID_WIDTH * GRASS_GRID_WIDTH];

int WIDTH = 1920;
int HEIGHT = 1080;

unsigned int shadowMapFBO;
Texture shadowMapDepthTexture;
Texture shadowMapDataTexture;
int SHADOW_MAP_WIDTH = 1000;
int SHADOW_MAP_HEIGHT = 1000;
float shadowMapScale = 10.0;

Vec3f lightPos = { 0.0, 20.0, 0.0 };
Vec3f lightDirection = { 0.7, -1.0, 0.5 };

float fov = M_PI / 4;
Vec3f cameraPos = getVec3f(4.0, 5.0, 0.0);
Vec3f cameraDirection = getVec3f(0.0, 0.0, 1.0);
Vec2f cameraRotation = getVec2f(M_PI / 2.0, 0.0);

void Engine_start(){

#ifndef RUN_OFFLINE
	Game_initClient(&game);
#endif

	Game_loadAssets(&game);

	//generate tree
	{
		setRandomSeed(1);

		std::vector<TreeNode> nodes;

		//main branch
		{
			TreeNode node;
			node.parent = -1;
			node.pos = getVec3f(0.0, 0.0, 0.0);
			node.radius = 0.4;
			nodes.push_back(node);
		}
		{
			TreeNode node;
			node.parent = 0;
			node.pos = getVec3f(0.0, 2.0, 0.0);
			node.radius = 0.3;
			nodes.push_back(node);
		
		}
		{
			TreeNode node;
			node.parent = 1;
			node.pos = getVec3f(0.0, 1.0, 0.3);
			node.radius = 0.3;
			nodes.push_back(node);
		
		}
		{
			TreeNode node;
			node.parent = 2;
			node.pos = getVec3f(-0.2, 1.5, 1.0);
			node.radius = 0.2;
			nodes.push_back(node);
		}
		{
			TreeNode node;
			node.parent = 3;
			node.pos = getVec3f(0.0, 1.0, 0.0);
			node.radius = 0.1;
			nodes.push_back(node);
		}
		{
			TreeNode node;
			node.parent = 4;
			node.pos = getVec3f(-1.0, 1.5, 0.0);
			node.radius = 0.05;
			nodes.push_back(node);
		}
		{
			TreeNode node;
			node.parent = 5;
			node.pos = getVec3f(-0.1, 1.0, 0.0);
			node.radius = 0.0;
			nodes.push_back(node);
		}

		//second branch
		{
			TreeNode node;
			node.parent = 1;
			node.pos = getVec3f(0.0, 1.0, -1.0);
			node.radius = 0.2;
			nodes.push_back(node);
		}
		int secondBranchIndex = nodes.size() - 1;
		{
			TreeNode node;
			node.parent = secondBranchIndex;
			node.pos = getVec3f(-0.5, 0.7, -0.7);
			node.radius = 0.1;
			nodes.push_back(node);
		}
		{
			TreeNode node;
			node.parent = nodes.size() - 1;
			node.pos = getVec3f(-0.3, 1.0, -0.3);
			node.radius = 0.05;
			nodes.push_back(node);
		}
		{
			TreeNode node;
			node.parent = nodes.size() - 1;
			node.pos = getVec3f(-0.1, 1.0, -0.1);
			node.radius = 0.0;
			nodes.push_back(node);
		}
		{
			TreeNode node;
			node.parent = secondBranchIndex;
			node.pos = getVec3f(0.5, 0.7, -0.7);
			node.radius = 0.1;
			nodes.push_back(node);
		}
		{
			TreeNode node;
			node.parent = nodes.size() - 1;
			node.pos = getVec3f(0.3, 1.0, -0.3);
			node.radius = 0.05;
			nodes.push_back(node);
		}
		{
			TreeNode node;
			node.parent = nodes.size() - 1;
			node.pos = getVec3f(0.1, 1.0, -0.1);
			node.radius = 0.0;
			nodes.push_back(node);
		}

		//third branch
		{
			TreeNode node;
			node.parent = 2;
			node.pos = getVec3f(0.7, 0.7, 0.0);
			node.radius = 0.2;
			nodes.push_back(node);
		}
		{
			TreeNode node;
			node.parent = nodes.size() - 1;
			node.pos = getVec3f(0.7, 1.0, 0.0);
			node.radius = 0.1;
			nodes.push_back(node);
		}
		{
			TreeNode node;
			node.parent = nodes.size() - 1;
			node.pos = getVec3f(0.3, 1.0, 0.0);
			node.radius = 0.05;
			nodes.push_back(node);
		}
		{
			TreeNode node;
			node.parent = nodes.size() - 1;
			node.pos = getVec3f(0.1, 1.0, 0.0);
			node.radius = 0.0;
			nodes.push_back(node);
		}

		//create triangle mesh
		std::vector<Vec3f> triangles;
		std::vector<Vec2f> textureCoords;

		for(int i = 0; i < nodes.size(); i++){

			TreeNode *node1_p = &nodes[i];

			for(int j = 0; j < nodes.size(); j++){

				TreeNode *node2_p = &nodes[j];

				if(i == node2_p->parent){

					node2_p->pos += node1_p->pos;

					Vec3f right2 = getVec3f(1.0, 0.0, 0.0);
					Vec3f forward2 = normalize(cross(right2, node2_p->pos - node1_p->pos));
					right2 = normalize(cross(forward2, node2_p->pos - node1_p->pos));

					Vec3f right1 = right2;
					Vec3f forward1 = forward2;

					if(node1_p->parent != -1){
						TreeNode *node3_p = &nodes[node1_p->parent];
						right1 = getVec3f(1.0, 0.0, 0.0);
						forward1 = normalize(cross(right1, node1_p->pos - node3_p->pos));
						right1 = normalize(cross(forward1, node1_p->pos - node3_p->pos));
					}

					std::vector<Vec3f> bottomPoints;
					std::vector<Vec3f> topPoints;

					//float r1 = 0.3;
					//float r2 = 0.3;

					int edges = 5;

					for(int i = 0; i < edges; i++){
						float angle = 2.0 * M_PI * ((float)i / float(edges));
						bottomPoints.push_back(node1_p->pos + right1 * cos(angle) * node1_p->radius + forward1 * sin(angle) * node1_p->radius);
						topPoints.push_back(node2_p->pos + right2 * cos(angle) * node2_p->radius + forward2 * sin(angle) * node2_p->radius);
					}

					for(int i = 0; i < edges; i++){
						
						Vec3f p1 = bottomPoints[i % edges];
						Vec3f p2 = bottomPoints[(i + 1) % edges];
						Vec3f p3 = topPoints[i % edges];
						Vec3f p4 = topPoints[(i + 1) % edges];

						triangles.push_back(p1);
						triangles.push_back(p2);
						triangles.push_back(p3);

						triangles.push_back(p3);
						triangles.push_back(p2);
						triangles.push_back(p4);

						Vec2f t1 = getVec2f((float)(i % edges) / (float)edges, 0.0);
						Vec2f t2 = getVec2f((float)((i + 1) % edges) / (float)edges, 0.0);
						Vec2f t3 = getVec2f((float)(i % edges) / (float)edges, 1.0);
						Vec2f t4 = getVec2f((float)((i + 1) % edges) / (float)edges, 1.0);

						textureCoords.push_back(t1);
						textureCoords.push_back(t2);
						textureCoords.push_back(t3);

						textureCoords.push_back(t3);
						textureCoords.push_back(t2);
						textureCoords.push_back(t4);

					}
					
				}

			}
			
		}

		TriangleMesh triangleMesh;

		String_set(triangleMesh.name, "tree", STRING_SIZE);

		triangleMesh.n_triangles = triangles.size() / 3;
		triangleMesh.triangles = (Vec3f *)malloc(sizeof(Vec3f) * 3 * triangleMesh.n_triangles);
		memcpy(triangleMesh.triangles, &triangles[0], sizeof(Vec3f) * 3 * triangleMesh.n_triangles);

		unsigned char *meshData = generateMeshDataFromTriangleMesh(triangleMesh, &textureCoords[0]);

		Model model;

		Model_initFromMeshData(&model, meshData, triangleMesh.n_triangles);

		String_set(model.name, "tree", STRING_SIZE);

		game.triangleMeshes.push_back(triangleMesh);
		game.models.push_back(model);

		free(meshData);

		//create leaves
		for(int i = 0; i < nodes.size(); i++){

			TreeNode *node1_p = &nodes[i];
			TreeNode *node2_p = &nodes[node1_p->parent];

			if(node1_p->radius < 0.06){

				int heightSegments = 5;
				int radialSegments = 5;
				//int heightSegments = 1;
				//int radialSegments = 1;

				//generate leaves pointing downards along branch
				for(int j = 0; j < heightSegments; j++){
					for(int n = 0; n < radialSegments; n++){

						float h = (float)j / (float)heightSegments;
						float t = (float)n / (float)radialSegments;

						Mat4f transformations = getIdentityMat4f();

						//transformations *= getTranslationMat4f(getVec3f(0.0, 1.0, 0.0));

						transformations *= getScalingMat4f((0.5 + 0.5 * sin(h * M_PI)) * TREE_SCALE);

						transformations *= getQuaternionMat4f(getQuaternion(getVec3f(1.0, 0.0, 0.0), M_PI / 2.0 + 0.2 + getRandom() * 0.3));

						transformations *= getQuaternionMat4f(getQuaternion(getVec3f(0.0, 1.0, 0.0), t * M_PI * 2.0 + h));

						transformations *= getTranslationMat4f(treePos + (node2_p->pos + (node1_p->pos - node2_p->pos) * h) * TREE_SCALE);

						leafTransformations.push_back(transformations);
					
					}
				}

				heightSegments = 4;
				radialSegments = 5;
				//heightSegments = 1;
				//radialSegments = 1;

				//generate leaves pointing upwards on top
				for(int j = 0; j < heightSegments; j++){
					for(int n = 0; n < radialSegments; n++){

						float h = (float)j / (float)heightSegments;
						float t = (float)n / (float)radialSegments;

						Mat4f transformations = getIdentityMat4f();

						transformations *= getScalingMat4f((0.7 - 0.4 * h) * TREE_SCALE);

						transformations *= getQuaternionMat4f(getQuaternion(getVec3f(1.0, 0.0, 0.0), M_PI / 2.0 - h * M_PI / 3.0 + 0.1 + 0.3 * getRandom()));

						transformations *= getQuaternionMat4f(getQuaternion(getVec3f(0.0, 1.0, 0.0), t * M_PI * 2.0 + h));

						transformations *= getTranslationMat4f(treePos + (node1_p->pos + getVec3f(0.0, -0.2, 0.0)) * TREE_SCALE);

						leafTransformations.push_back(transformations);
					
					}
				}
			
			}

		}

		TextureBuffer_initAsMat4fArray(&leafTransformationsTextureBuffer, &leafTransformations[0], leafTransformations.size(), false);

	}

	//generate terrain
	{
		setRandomSeed(1);
		TriangleMesh triangleMesh = generateTerrainTriangleMesh(TERRAIN_WIDTH, 1.0 / (TERRAIN_SCALE / 2.0));
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

		std::vector<Vec4f> grassPositions[GRASS_GRID_WIDTH * GRASS_GRID_WIDTH];

		int n = 0;
		for(float x = 0.0; x < TERRAIN_SCALE; x += GRASS_DISTANCE){
			for(float y = 0.0; y < TERRAIN_SCALE; y += GRASS_DISTANCE){

				int gridX = (float)GRASS_GRID_WIDTH * (x / TERRAIN_SCALE);
				int gridY = (float)GRASS_GRID_WIDTH * (y / TERRAIN_SCALE);
				int gridIndex = gridY * GRASS_GRID_WIDTH + gridX;

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
					grassPositions[gridIndex].push_back(pos);
				}
		
			}
		}

		//create grass grid texture buffers
		for(int x = 0; x < GRASS_GRID_WIDTH; x++){
			for(int y = 0; y < GRASS_GRID_WIDTH; y++){
				
				int gridIndex = y * GRASS_GRID_WIDTH + x;

				TextureBuffer_initAsVec4fArray(&grassPositionsTextureBufferGrid[gridIndex], &grassPositions[gridIndex][0], grassPositions[gridIndex].size());

				//add bounding box
				Box boundingBox;
				boundingBox.pos = getVec3f(TERRAIN_SCALE * (float)x / (float)GRASS_GRID_WIDTH, 0.0, TERRAIN_SCALE * (float)y / (float)GRASS_GRID_WIDTH);
				boundingBox.size = getVec3f(TERRAIN_SCALE / (float)GRASS_GRID_WIDTH, 2.0, TERRAIN_SCALE / (float)GRASS_GRID_WIDTH);

				game.boundingBoxes.push_back(boundingBox);
				game.boundingBoxesCulled.push_back(false);

				grassBoundingBoxIndices[gridIndex] = game.boundingBoxes.size() - 1;

			}
		}

	}

	Engine_setWindowSize(WIDTH / 2, HEIGHT / 2);

	Engine_centerWindow();

	Engine_setFPSMode(true);

	//OpenGL stuff
	glEnable(GL_BLEND);

	glDepthFunc(GL_LESS);
	glDepthMask(GL_TRUE);
	glEnable(GL_DEPTH_TEST);
	glDepthRange(0.0, 1.0);

	glEnable(GL_CULL_FACE);
	glCullFace(GL_FRONT);

	//generate shadow map depth texture
	Texture_initAsDepthMap(&shadowMapDepthTexture, SHADOW_MAP_WIDTH, SHADOW_MAP_HEIGHT);
	Texture_initAsColorMap(&shadowMapDataTexture, SHADOW_MAP_WIDTH, SHADOW_MAP_HEIGHT);

	//generate shadow map frame buffer
	glGenFramebuffers(1, &shadowMapFBO);
	glBindFramebuffer(GL_FRAMEBUFFER, shadowMapFBO);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, shadowMapDepthTexture.ID, 0);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, shadowMapDataTexture.ID, 0);

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
		obstacle.triangleMeshIndex = Game_getTriangleMeshIndexByName(&game, "cube");
		obstacle.color = getVec4f(0.7, 0.7, 0.7, 1.0);

		game.obstacles.push_back(obstacle);
	}
	{
		Obstacle obstacle;	
		obstacle.pos = treePos;
		obstacle.scale = TREE_SCALE;
		obstacle.modelIndex = Game_getModelIndexByName(&game, "tree");
		obstacle.textureIndex = Game_getTextureIndexByName(&game, "bark");
		obstacle.triangleMeshIndex = Game_getTriangleMeshIndexByName(&game, "tree");
		obstacle.color = getVec4f(1.0, 1.0, 1.0, 1.0);

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

}

void Engine_update(float deltaTime){

	//handle window controls
	if(Engine_keys[ENGINE_KEY_Q].down){
		Engine_quit();
	}
	if(Engine_keys[ENGINE_KEY_F].downed){
		Engine_toggleFullscreen();
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

		if(inputs.forwards == 1){
			game.player.velocity.x += cameraDirectionXZ.x * PLAYER_SPEED;
			game.player.velocity.z += cameraDirectionXZ.y * PLAYER_SPEED;
		}
		if(inputs.backwards == 1){
			game.player.velocity.x += -cameraDirectionXZ.x * PLAYER_SPEED;
			game.player.velocity.z += -cameraDirectionXZ.y * PLAYER_SPEED;
		}
		if(inputs.left == 1){
			Vec3f left = getCrossVec3f(inputs.cameraDirection, getVec3f(0, 1.0, 0));
			Vec3f_normalize(&left);
			game.player.velocity.x += left.x * PLAYER_SPEED;
			game.player.velocity.z += left.z * PLAYER_SPEED;
		}
		if(inputs.right == 1){
			Vec3f right = getCrossVec3f(getVec3f(0, 1.0, 0), inputs.cameraDirection);
			Vec3f_normalize(&right);
			game.player.velocity.x += right.x * PLAYER_SPEED;
			game.player.velocity.z += right.z * PLAYER_SPEED;
		}
		if(inputs.jump == 1
		&& game.player.onGround){
			game.player.velocity.y += PLAYER_JUMP_SPEED;
		}
		if(inputs.jump == 0
		&& game.player.velocity.y > 0.0){
			game.player.velocity.y *= 0.9;
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

		if(particle_p->frames >= 10 + getRandom() * 20.0){
			game.particles.erase(game.particles.begin() + i);
			i--;
			continue;
		}

		Vec3f_add(&particle_p->pos, particle_p->velocity);

		particle_p->frames++;

	}

}

void Engine_draw(){

	bool renderFromLightPerspective = false;

	int n_renderingStages = 2;

	for(int renderingStage = 0; renderingStage < n_renderingStages; renderingStage++){

		glColorMask(true, true, true, true);

		if(renderingStage == 0){

			glViewport(0.0, 0.0, SHADOW_MAP_WIDTH, SHADOW_MAP_HEIGHT);
			glBindFramebuffer(GL_FRAMEBUFFER, shadowMapFBO);
			glClearColor(0.0, 0.0, 0.0, 1.0);

			glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

			glBlendFuncSeparate(GL_ONE, GL_NONE, GL_ONE, GL_ONE);
			glBlendEquationSeparate(GL_FUNC_ADD, GL_MIN);
			glDisable(GL_DEPTH_TEST);

		}
		if(renderingStage == 1){

			glViewport(0.0, 0.0, Engine_clientWidth, Engine_clientHeight);
			glBindFramebuffer(GL_FRAMEBUFFER, 0);  
			glClearColor(0.5, 0.7, 0.9, 1.0);
			glClearDepth(1.0);

			glBlendEquation(GL_FUNC_ADD);
			glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
			glDepthFunc(GL_LESS);
			glEnable(GL_DEPTH_TEST);

			glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		}


		Mat4f cameraMatrix = getLookAtMat4f(cameraPos, cameraDirection);

		Mat4f perspectiveMatrix = getPerspectiveMat4f(fov, (float)Engine_clientWidth / (float)Engine_clientHeight);

		Vec2f flatCameraDirection = normalize(getVec2f(cameraDirection.x, cameraDirection.z));
		Vec2f flatLightDirection = normalize(getVec2f(lightDirection.x, lightDirection.z));

		Mat4f lightCameraMatrix = getTranslationMat4f(round(-game.player.pos.x), round(-game.player.pos.y), round(-game.player.pos.z));

		lightCameraMatrix *= getTranslationMat4f(getVec3f(lightDirection.x, 0.0, lightDirection.z) * lightPos.y);

		lightCameraMatrix *= getTranslationMat4f(getVec3f(round(-cameraDirection.x * shadowMapScale), 0.0, round(-cameraDirection.z * shadowMapScale)));

		lightCameraMatrix *= getLookAtMat4f(lightPos, lightDirection);

		

		Mat4f lightPerspectiveMatrix = getScalingMat4f(1.0 / shadowMapScale);

		if(renderingStage == 0 || false){
			cameraMatrix = lightCameraMatrix;
			perspectiveMatrix = lightPerspectiveMatrix;
		}

		Mat4f frustumMatrix = cameraMatrix;

		//cull bounding boxes
		for(int i = 0; i < game.boundingBoxesCulled.size(); i++){
			game.boundingBoxesCulled[i] = false;
		}
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

				if(positions[j].z > 0.0
				&& (renderingStage == 0 && fabs(positions[j].x) < shadowMapScale && fabs(positions[j].y) < shadowMapScale)
				|| renderingStage == 1 && fabs(positions[j].x) < positions[j].z){
					game.boundingBoxesCulled[i] = false;
					break;
				}

			}

		}

		//draw grass
		{
			glDisable(GL_CULL_FACE);

			int drawnCells = 0;

			for(int i = 0; i < GRASS_GRID_WIDTH * GRASS_GRID_WIDTH; i++){

				if(game.boundingBoxesCulled[grassBoundingBoxIndices[i]]){
					continue;
				}

				Box *boundingBox_p = &game.boundingBoxes[grassBoundingBoxIndices[i]];
				Vec3f boundingBoxCenter = boundingBox_p->pos + boundingBox_p->size / 2.0;
				float distanceToCamera = getMagVec2f(getVec2f(cameraPos.x, cameraPos.y) - getVec2f(boundingBoxCenter.x, boundingBoxCenter.y));

				//float LODDistance1 = 20.0;
				float LODDistance1 = 15.0;

				drawnCells++;

				Vec3f pos = getVec3f(0.0, 4.0, 0.0);
				float scale = 1.0;

				Shader *shader_p = Game_getShaderPointerByName(&game, "grass");
				if(renderingStage == 0){
					shader_p = Game_getShaderPointerByName(&game, "grass-shadow");
				}

				glUseProgram(shader_p->ID);

				Texture *texture_p = Game_getTexturePointerByName(&game, "grass");

				Model *model_p = Game_getModelPointerByName(&game, "quad");

				glBindBuffer(GL_ARRAY_BUFFER, model_p->VBO);
				glBindVertexArray(model_p->VAO);

				GL3D_uniformTexture(shader_p->ID, "colorTexture", 0, texture_p->ID);
				GL3D_uniformTextureBuffer(shader_p->ID, "grassPositions", 1, grassPositionsTextureBufferGrid[i].TB);
				GL3D_uniformTexture(shader_p->ID, "shadowMapDepthTexture", 2, shadowMapDepthTexture.ID);
				GL3D_uniformTexture(shader_p->ID, "shadowMapDataTexture", 3, shadowMapDataTexture.ID);

				//GL3D_uniformTexture(currentShaderProgram, "heightMap", 1, terrainHeightMapTexture.ID);

				GL3D_uniformMat4f(shader_p->ID, "perspectiveMatrix", perspectiveMatrix);
				GL3D_uniformMat4f(shader_p->ID, "cameraMatrix", cameraMatrix);
				GL3D_uniformMat4f(shader_p->ID, "lightPerspectiveMatrix", lightPerspectiveMatrix);
				GL3D_uniformMat4f(shader_p->ID, "lightCameraMatrix", lightCameraMatrix);

				GL3D_uniformFloat(shader_p->ID, "windTime", windTime);

				GL3D_uniformFloat(shader_p->ID, "shadowStrength", 0.3);

				float rotation = 0.0;

				if(distanceToCamera > LODDistance1){

					rotation = atan2(cameraDirection.z, cameraDirection.x) + M_PI / 2.0;

					if(rotation > M_PI){
						rotation -= M_PI;
					}
					if(rotation < 0.0){
						rotation += M_PI;
					}

					rotation = round(rotation / (M_PI / 3.0)) * M_PI / 3.0;

				}


				GL3D_uniformFloat(shader_p->ID, "rotation", rotation);

				glDrawArraysInstanced(GL_TRIANGLES, 0, model_p->numberOfTriangles * 3, grassPositionsTextureBufferGrid[i].n_elements);

				if(distanceToCamera < LODDistance1){
					GL3D_uniformFloat(shader_p->ID, "rotation", M_PI * (1.0 / 3.0));

					glDrawArraysInstanced(GL_TRIANGLES, 0, model_p->numberOfTriangles * 3, grassPositionsTextureBufferGrid[i].n_elements);

					GL3D_uniformFloat(shader_p->ID, "rotation", M_PI * (2.0 / 3.0));

					glDrawArraysInstanced(GL_TRIANGLES, 0, model_p->numberOfTriangles * 3, grassPositionsTextureBufferGrid[i].n_elements);
				}
			
			}

			//printf("drew %i / %i gras cells\n", drawnCells, GRASS_GRID_WIDTH * GRASS_GRID_WIDTH);

			glEnable(GL_CULL_FACE);
		}

		//enable depth testing for objects with shadow strengths of 1.0
		if(renderingStage == 0){
			glEnable(GL_DEPTH_TEST);
			glColorMask(false, false, false, false);
		}

		//draw leaves
		{
			glDisable(GL_CULL_FACE);

			//unsigned int currentShaderProgram = game.leafShader;
			Shader *shader_p = Game_getShaderPointerByName(&game, "leaf");
			if(renderingStage == 0){
				shader_p = Game_getShaderPointerByName(&game, "leaf-shadow");
			}

			glUseProgram(shader_p->ID);

			Texture *texture_p = Game_getTexturePointerByName(&game, "leaves");

			Model *model_p = Game_getModelPointerByName(&game, "quad");

			glBindBuffer(GL_ARRAY_BUFFER, model_p->VBO);
			glBindVertexArray(model_p->VAO);

			GL3D_uniformFloat(shader_p->ID, "shadowStrength", 0.2);
			//GL3D_uniformFloat(shader_p->ID, "shadowStrength", 1.0);

			GL3D_uniformTexture(shader_p->ID, "colorTexture", 0, texture_p->ID);
			GL3D_uniformTexture(shader_p->ID, "shadowMapDepthTexture", 1, shadowMapDepthTexture.ID);
			GL3D_uniformTexture(shader_p->ID, "shadowMapDataTexture", 2, shadowMapDataTexture.ID);
			GL3D_uniformTextureBuffer(shader_p->ID, "modelTransformationsBuffer", 3, leafTransformationsTextureBuffer.TB);

			GL3D_uniformFloat(shader_p->ID, "windTime", windTime);

			GL3D_uniformMat4f(shader_p->ID, "perspectiveMatrix", perspectiveMatrix);
			GL3D_uniformMat4f(shader_p->ID, "cameraMatrix", cameraMatrix);
			GL3D_uniformMat4f(shader_p->ID, "lightPerspectiveMatrix", lightPerspectiveMatrix);
			GL3D_uniformMat4f(shader_p->ID, "lightCameraMatrix", lightCameraMatrix);

			//GL3D_uniformFloat(shader_p->ID, "shadowFactor", 0.1);
			//GL3D_uniformFloat(shader_p->ID, "shadowFactor", 0.0);

			glDrawArraysInstanced(GL_TRIANGLES, 0, model_p->numberOfTriangles * 3, leafTransformations.size());

			glEnable(GL_CULL_FACE);

		}

		//draw obstacles
		for(int i = 0; i < game.obstacles.size(); i++){

			Obstacle *obstacle_p = &game.obstacles[i];

			float scale = obstacle_p->scale;

			Mat4f modelMatrix = getIdentityMat4f();

			modelMatrix *= getScalingMat4f(scale);

			modelMatrix *= getTranslationMat4f(obstacle_p->pos);

			//unsigned int currentShaderProgram = game.modelShader;
			Shader *shader_p = Game_getShaderPointerByName(&game, "model");
			if(renderingStage == 0){
				shader_p = Game_getShaderPointerByName(&game, "model-shadow");
			}

			glUseProgram(shader_p->ID);
			
			Model *model_p = &game.models[obstacle_p->modelIndex];

			Texture *texture_p = &game.textures[obstacle_p->textureIndex];

			glBindBuffer(GL_ARRAY_BUFFER, model_p->VBO);
			glBindVertexArray(model_p->VAO);

			GL3D_uniformFloat(shader_p->ID, "shadowStrength", 1.0);
			//GL3D_uniformFloat(shader_p->ID, "shadowStrength", 0.1);

			GL3D_uniformTexture(shader_p->ID, "colorTexture", 0, texture_p->ID);
			GL3D_uniformTexture(shader_p->ID, "shadowMapDepthTexture", 1, shadowMapDepthTexture.ID);
			GL3D_uniformTexture(shader_p->ID, "shadowMapDataTexture", 2, shadowMapDataTexture.ID);

			GL3D_uniformMat4f(shader_p->ID, "modelMatrix", modelMatrix);
			GL3D_uniformMat4f(shader_p->ID, "perspectiveMatrix", perspectiveMatrix);
			GL3D_uniformMat4f(shader_p->ID, "cameraMatrix", cameraMatrix);
			GL3D_uniformMat4f(shader_p->ID, "lightPerspectiveMatrix", lightPerspectiveMatrix);
			GL3D_uniformMat4f(shader_p->ID, "lightCameraMatrix", lightCameraMatrix);

			GL3D_uniformVec4f(shader_p->ID, "inputColor", obstacle_p->color);

			//GL3D_uniformFloat(shader_p->ID, "shadowFactor", 1.0);

			glDrawArrays(GL_TRIANGLES, 0, model_p->numberOfTriangles * 3);

			
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
		{

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
		if(renderingStage == 1 && false){

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
			GL3D_uniformTexture(shader_p->ID, "shadowMapDataTexture", 1, shadowMapDataTexture.ID);

			GL3D_uniformMat4f(shader_p->ID, "modelMatrix", modelMatrix);
			GL3D_uniformMat4f(shader_p->ID, "perspectiveMatrix", perspectiveMatrix);
			GL3D_uniformMat4f(shader_p->ID, "cameraMatrix", cameraMatrix);
			GL3D_uniformMat4f(shader_p->ID, "lightPerspectiveMatrix", lightPerspectiveMatrix);
			GL3D_uniformMat4f(shader_p->ID, "lightCameraMatrix", lightCameraMatrix);

			GL3D_uniformVec4f(shader_p->ID, "inputColor", getVec4f(1.0, 0.0, 0.0, 1.0));

			glDrawArrays(GL_TRIANGLES, 0, model_p->numberOfTriangles * 3);

		}
		*/
	
	}

	//glBindFramebuffer(GL_READ_FRAMEBUFFER, shadowMapFBO);
	//glBlitFramebuffer(0, 0, SHADOW_MAP_WIDTH, SHADOW_MAP_HEIGHT, 0, 0, Engine_clientWidth, Engine_clientHeight, GL_COLOR_BUFFER_BIT, GL_NEAREST);

	gameTime += 0.1;
	windTime += 0.01;

}

void Engine_finnish(){

}
