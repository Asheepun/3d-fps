#include "engine/shaders.h"

#include "game.h"

#include "stdio.h"
#include "stdlib.h"
#include "math.h"
#include "time.h"
#include <cstring>
#include <vector>
#include <immintrin.h>

bool drawTimings = false;

Game game;

//const float GRASS_DISTANCE = 1.0;
//const float GRASS_DISTANCE = 1.0;
//const float GRASS_DISTANCE = 3.0;
//const float GRASS_DISTANCE = 1.5;
//float grassShadowMapScale = 10.0;

/*
float *terrainHeightMap;
int TERRAIN_HEIGHT_MAP_WIDTH = 100;
int TERRAIN_HEIGHT_MAP_HEIGHT = 100;
*/

//Vec3f lastTorque = getVec3f(0.0, 0.0, 0.0);
//int framesSinceLastSwitch = 0;
//int unstableHits = 0;
bool stabelized = false;
bool stable = false;
int framesSinceHit = 0;

void Engine_start(){

	setRandomSeed(1);

#ifndef RUN_OFFLINE
	Client_init(&game.client);
	//Game_initClient(&game);
#endif

	game.dead = false;

	pthread_t loadAssetsThread;
	//pthread_create(&loadAssetsThread, NULL, loadAssetsAndGenerateStuffThreaded, &game);

	loadAssetsAndGenerateStuffThreaded(&game);

	//generate water mesh
	{
		Game *game_p = &game;

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

		game_p->models.push_back(model);

	}

	//generate terrain
	{
		Game *game_p = &game;

		Vec3f *triangles;
		Vec2f *textureCoords;
		int n_triangles;

		setRandomSeed(1);
		generateTerrainTriangles(&triangles, &textureCoords, &n_triangles);

		TriangleMesh triangleMesh;
		triangleMesh.triangles = triangles;
		triangleMesh.n_triangles = n_triangles;
		String_set(triangleMesh.name, "terrain", STRING_SIZE);

		game_p->world.triangleMeshes.push_back(triangleMesh);

		unsigned char *meshData = generateMeshDataFromTriangleMesh(triangleMesh, textureCoords);

		Model model;

		Model_initFromMeshData(&model, meshData, triangleMesh.n_triangles);

		String_set(model.name, "terrain", STRING_SIZE);

		game_p->models.push_back(model);

		game_p->terrainMaxHeight = 0.0;
		for(int i = 0; i < n_triangles * 3; i++){
			game_p->terrainMaxHeight = fmax(game_p->terrainMaxHeight, triangles[i].y * TERRAIN_SCALE);
		}

		//free unneeded data
		free(meshData);
		free(textureCoords);

	}

	//generate terrain texture
	{
		Game *game_p = &game;

		Texture texture;

		game_p->terrainTextureData = (unsigned char *)malloc(sizeof(unsigned char) * 4 * TERRAIN_TEXTURE_WIDTH * TERRAIN_TEXTURE_WIDTH);

		for(int i = 0; i < TERRAIN_TEXTURE_WIDTH * TERRAIN_TEXTURE_WIDTH; i++){
			game_p->terrainTextureData[i * 4 + 0] = (unsigned char)(TERRAIN_COLOR.x * 255.0);
			game_p->terrainTextureData[i * 4 + 1] = (unsigned char)(TERRAIN_COLOR.y * 255.0);
			game_p->terrainTextureData[i * 4 + 2] = (unsigned char)(TERRAIN_COLOR.z * 255.0);
			game_p->terrainTextureData[i * 4 + 3] = (unsigned char)(TERRAIN_COLOR.w * 255.0);
		}

		Texture_init(&texture, "terrain", game_p->terrainTextureData, TERRAIN_TEXTURE_WIDTH, TERRAIN_TEXTURE_WIDTH);

		game_p->textures.push_back(texture);
	}


	//generate grass positions
	{
		Game *game_p = &game;

		TriangleMesh *terrainTriangleMesh_p = World_getTriangleMeshPointerByName(&game_p->world, "terrain");

		int n = 0;
		int hits = 0;

		//float margin = 3.0;

		for(float y = GRASS_MARGIN; y < TERRAIN_SCALE - GRASS_MARGIN; y += GRASS_DISTANCE){
			for(float x = GRASS_MARGIN; x < TERRAIN_SCALE - GRASS_MARGIN; x += GRASS_DISTANCE){

				//set grass variables
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

						if(rayOrigin.x >= east && rayOrigin.x <= west
						&& rayOrigin.z >= north && rayOrigin.z <= south){

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
					pos.w = getRandom() + 100.0;
					game_p->grassPositions.push_back(pos);
					pos.y += 3.0;

					hits++;
				}

				n++;

				//stepPos += dir;
			}
		}

		//printf("%i, %i\n", hits, n);

		TextureBuffer_initAsVec4fArray(&game_p->grassPositionsTextureBuffer, &game_p->grassPositions[0], game_p->grassPositions.size());

		game_p->sortedGrassPositions = (Vec4f *)malloc(sizeof(Vec4f) * game_p->grassPositions.size());
	}


	Renderer2D_init(&game.renderer2D, WIDTH, HEIGHT);

	game.font = getFont("assets/times.ttf", 60);

#ifdef RUN_OFFLINE
	game.currentState = GAME_STATE_LEVEL;
#else
	game.currentState = GAME_STATE_LOBBY;
#endif

	Engine_setWindowSize(WIDTH / 2, HEIGHT / 2);

	Engine_centerWindow();

#ifdef RUN_OFFLINE
	Engine_fpsModeOn = true;
#else
	Engine_fpsModeOn = false;
#endif

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
	Texture_initAsDepthMap(&game.shadowMapDepthTexture, SHADOW_MAP_WIDTH, SHADOW_MAP_HEIGHT);
	Texture_initAsDepthMap(&game.grassShadowMapDepthTexture, GRASS_SHADOW_MAP_WIDTH, GRASS_SHADOW_MAP_HEIGHT);
	Texture_initAsDepthMap(&game.bigShadowMapDepthTexture, BIG_SHADOW_MAP_WIDTH, BIG_SHADOW_MAP_HEIGHT);
	//Texture_initAsColorMap(&shadowMapDataTexture, SHADOW_MAP_WIDTH, SHADOW_MAP_HEIGHT);

	//generate shadow map frame buffer
	glGenFramebuffers(1, &game.shadowMapFBO);
	glBindFramebuffer(GL_FRAMEBUFFER, game.shadowMapFBO);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, game.shadowMapDepthTexture.ID, 0);
	//glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, shadowMapDataTexture.ID, 0);

	//generate grass shadow map frame buffer
	glGenFramebuffers(1, &game.grassShadowMapFBO);
	glBindFramebuffer(GL_FRAMEBUFFER, game.grassShadowMapFBO);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, game.grassShadowMapDepthTexture.ID, 0);

	//generate big shadow map frame buffer
	glGenFramebuffers(1, &game.bigShadowMapFBO);
	glBindFramebuffer(GL_FRAMEBUFFER, game.bigShadowMapFBO);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, game.bigShadowMapDepthTexture.ID, 0);

	//generate paint map
	//Texture_initAsDepthMap(&paintMapTexture, PAINT_MAP_WIDTH, PAINT_MAP_HEIGHT);
	game.paintMap = (unsigned char *)malloc(sizeof(unsigned char) * PAINT_MAP_WIDTH * PAINT_MAP_HEIGHT);
	memset(game.paintMap, 0, sizeof(unsigned char) * PAINT_MAP_WIDTH * PAINT_MAP_HEIGHT);

	glGenTextures(1, &game.paintMapTexture.ID);
	glBindTexture(GL_TEXTURE_2D, game.paintMapTexture.ID);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_R8, PAINT_MAP_WIDTH, PAINT_MAP_HEIGHT, 0, GL_RED, GL_UNSIGNED_BYTE, game.paintMap);
	glGenerateMipmap(GL_TEXTURE_2D);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

	//glDrawBuffer(GL_NONE);
	//glReadBuffer(GL_NONE);

	glBindFramebuffer(GL_FRAMEBUFFER, 0);  

	//init game
	game.startLevel = false;

#ifdef RUN_OFFLINE
	World_addObstacle(
		&game.world,
		getVec3f(0.0, 0.0, 0.0),
		TERRAIN_SCALE,
		World_getTriangleMeshIndexByName(&game.world, "terrain"),
		Game_getModelIndexByName(&game, "terrain"),
		Game_getTextureIndexByName(&game, "terrain"),
		getVec4f(1.0, 1.0, 1.0, 1.0)
	);
#endif

	/*
	World_addObstacle(
		&game.world,
		getVec3f(30.0, 0.0, 20.0),
		3.0,
		World_getTriangleMeshIndexByName(&game.world, "cube"),
		Game_getModelIndexByName(&game, "cube"),
		Game_getTextureIndexByName(&game, "blank"),
		getVec4f(0.7, 0.7, 0.7, 1.0)
	);
	*/

#ifdef RUN_OFFLINE
	World_addPlayer(&game.world, getVec3f(5.0, 3.0, 5.0), game.client.connectionID);
	World_addPlayer(&game.world, getVec3f(10.0, 3.0, 15.0), 100);
#endif

	/*
	{
		game.player.pos = getVec3f(5.0, 3.0, 5.0);
		game.player.lastPos = game.player.pos;
		game.player.velocity = getVec3f(0.0, 0.0, 0.0);
		game.player.onGround = false;
		game.player.height = PLAYER_HEIGHT_STANDING;
		game.player.weapon = WEAPON_GUN;
		//game.player.weapon = WEAPON_SWORD;
		//Vec3f lastPlayerPos = game.player.pos;
	}
	{
		Player player;
		player.pos = getVec3f(10.0, 5.0, 30.0);
		game.otherPlayers.push_back(player);
	}
	*/
	/*
	{
		RigidBody rigidBody;
		rigidBody.pos = getVec3f(20.0, 10.0, 35.0);
		rigidBody.velocity = getVec3f(0.0, 0.0, 0.0);
		rigidBody.orientation = getQuaternion(getVec3f(0.0, 0.0, 1.0), 0.3);
		rigidBody.angularVelocity = getVec3f(0.0, 0.0, 0.0);
		rigidBody.modelIndex = Game_getModelIndexByName(&game, "cube");
		rigidBody.textureIndex = Game_getTextureIndexByName(&game, "blank");
		rigidBody.triangleMeshIndex = Game_getTriangleMeshIndexByName(&game, "cube");
		rigidBody.lastTorque = getVec3f(0.0, 0.0, 0.0);
		rigidBody.framesSinceHit = 0;
		game.rigidBodies.push_back(rigidBody);
	}
	{
		RigidBody rigidBody;
		rigidBody.pos = getVec3f(30.0, 10.0, 35.0);
		rigidBody.velocity = getVec3f(0.0, 0.0, 0.0);
		rigidBody.orientation = getQuaternion(getVec3f(0.0, 0.0, 1.0), 0.3);
		rigidBody.angularVelocity = getVec3f(0.0, 0.0, 0.0);
		rigidBody.modelIndex = Game_getModelIndexByName(&game, "cube");
		rigidBody.textureIndex = Game_getTextureIndexByName(&game, "blank");
		rigidBody.triangleMeshIndex = Game_getTriangleMeshIndexByName(&game, "cube");
		rigidBody.lastTorque = getVec3f(0.0, 0.0, 0.0);
		rigidBody.framesSinceHit = 0;
		game.rigidBodies.push_back(rigidBody);
	}
	*/
	/*
	{
		Obstacle obstacle;	
		obstacle.pos = getVec3f(20.0, 0.0, 35.0);
		obstacle.scale = 3.0;
		obstacle.modelIndex = Game_getModelIndexByName(&game, "cube");
		obstacle.textureIndex = Game_getTextureIndexByName(&game, "blank");
		obstacle.alphaTextureIndex = Game_getTextureIndexByName(&game, "blank-alpha");
		obstacle.triangleMeshIndex = Game_getTriangleMeshIndexByName(&game, "cube");
		obstacle.color = getVec4f(0.7, 0.7, 0.7, 1.0);

		game.obstacles.push_back(obstacle);
	}
	*/

	/*
	{
		Vec3f pos = getVec3f(10.0, 0.0, 10.0);
		Tree tree;
		Model model;
		TriangleMesh triangleMesh;

		generateTree(pos, &tree, &model, &triangleMesh);

		game.trees.push_back(tree);

		game.models.push_back(model);
		game.world.triangleMeshes.push_back(triangleMesh);

		World_addObstacle(
			&game.world,
			pos,
			TREE_SCALE,
			game.world.triangleMeshes.size() - 1,
			game.models.size() - 1,
			Game_getTextureIndexByName(&game, "bark"),
			getVec4f(0.7, 0.7, 0.7, 1.0)
		);
	}
	*/

	/*
	Game_addTree(&game, getVec3f(15.0, 0.0, 15.0));

	Game_addTree(&game, getVec3f(25.0, 0.0, 15.0));

	Game_addTree(&game, getVec3f(35.0, 0.0, 15.0));

	Game_addTree(&game, getVec3f(90.0, 0.0, 15.0));

	Game_addTree(&game, getVec3f(15.0, 0.0, 90.0));

	Game_addTree(&game, getVec3f(90.0, 0.0, 90.0));

	Game_addTree(&game, getVec3f(40.0, 0.0, 40.0));
	*/

	/*
	//generate point meshes
	for(int i = 0; i < game.world.triangleMeshes.size(); i++){
		PointMesh pointMesh;
		PointMesh_initFromTriangleMesh(&pointMesh, game.world.triangleMeshes[i]);
		game.pointMeshes.push_back(pointMesh);
	}

	//generate centers for triangle meshes
	for(int i = 0; i < game.world.triangleMeshes.size(); i++){
		
		TriangleMesh *triangleMesh_p = &game.world.triangleMeshes[i];

		triangleMesh_p->triangleRadii = (float *)malloc(sizeof(float) * triangleMesh_p->n_triangles);

		for(int j = 0; j < triangleMesh_p->n_triangles; j++){

			Vec3f p1 = triangleMesh_p->triangles[j * 3 + 0];
			Vec3f p2 = triangleMesh_p->triangles[j * 3 + 1];
			Vec3f p3 = triangleMesh_p->triangles[j * 3 + 2];

			Vec3f center = (p1 + p2 + p3) / 3.0;

			float radius = fmax(fmax(getMagVec3f(p1 - center), getMagVec3f(p2 - center)), getMagVec3f(p3 - center));

			triangleMesh_p->triangleRadii[j] = radius;

		}

	}
	*/

	/*
	//generate bounding boxes for obstacles
	for(int i = 0; i < game.world.obstacles.size(); i++){
		
		Obstacle *obstacle_p = &game.world.obstacles[i];
		PointMesh *pointMesh_p = &game.pointMeshes[obstacle_p->triangleMeshIndex];

		Mat4f modelMatrix = getIdentityMat4f();
		modelMatrix *= getScalingMat4f(obstacle_p->scale);
		modelMatrix *= getTranslationMat4f(obstacle_p->pos);

		for(int j = 0; j < pointMesh_p->n_points; j++){

			Vec3f point = mulVec3fMat4f(pointMesh_p->points[j], modelMatrix, 1.0);

			if(j == 0){
				obstacle_p->boundingBox.pos = point;
				obstacle_p->boundingBox.size = point;
			}

			obstacle_p->boundingBox.pos.x = fmin(obstacle_p->boundingBox.pos.x, point.x);
			obstacle_p->boundingBox.pos.y = fmin(obstacle_p->boundingBox.pos.y, point.y);
			obstacle_p->boundingBox.pos.z = fmin(obstacle_p->boundingBox.pos.z, point.z);

			obstacle_p->boundingBox.size.x = fmax(obstacle_p->boundingBox.size.x, point.x);
			obstacle_p->boundingBox.size.y = fmax(obstacle_p->boundingBox.size.y, point.y);
			obstacle_p->boundingBox.size.z = fmax(obstacle_p->boundingBox.size.z, point.z);

		}

		obstacle_p->boundingBox.size = obstacle_p->boundingBox.size - obstacle_p->boundingBox.pos;

		//printf("obs\n");
		//Vec3f_log(obstacle_p->boundingBox.pos);
		//Vec3f_log(obstacle_p->boundingBox.size);

	}
	*/

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

	/*
	if(game.client.startLevel){

		//for(int i = )

		game.currentState = GAME_STATE_LEVEL;
		game.client.startLevel = false;
	}
	*/

	switch(game.currentState){
		case GAME_STATE_LEVEL:
			levelState(&game);
			break;
		case GAME_STATE_LOBBY:
			lobbyState(&game);
			break;
	}

}

void Engine_draw(){

	Renderer2D_updateDrawSize(&game.renderer2D, Engine_clientWidth, Engine_clientHeight);

	switch(game.currentState){
		case GAME_STATE_LEVEL:
			drawLevelState(&game);
			break;
		case GAME_STATE_LOBBY:
			drawLobbyState(&game);
			break;
	}

	//draw timing information
	if(drawTimings){

		glDisable(GL_DEPTH_TEST);

		Renderer2D_setShader(&game.renderer2D, game.renderer2D.colorShader);

		Renderer2D_setColor(&game.renderer2D, getVec4f(0.0, 0.0, 0.0, 0.5));

		Renderer2D_drawRectangle(&game.renderer2D, WIDTH - 450, 50, 400, 225);

		Renderer2D_setShader(&game.renderer2D, game.renderer2D.textureColorShader);

		Renderer2D_setColor(&game.renderer2D, getVec4f(1.0, 1.0, 1.0, 1.0));

		char updateText[STRING_SIZE];
		String_set(updateText, "", STRING_SIZE);
		sprintf(updateText, "update: %.2f ms", Engine_frameUpdateTime);

		char drawText[STRING_SIZE];
		String_set(drawText, "", STRING_SIZE);
		sprintf(drawText, "draw: %.2f ms", Engine_frameDrawTime);

		char totalText[STRING_SIZE];
		String_set(totalText, "", STRING_SIZE);
		sprintf(totalText, "total: %.2f ms", Engine_frameTime);

		Renderer2D_drawText(&game.renderer2D, updateText, WIDTH - 430, 60, 60, game.font);

		Renderer2D_drawText(&game.renderer2D, drawText, WIDTH - 430, 130, 60, game.font);

		Renderer2D_drawText(&game.renderer2D, totalText, WIDTH - 430, 200, 60, game.font);

		glEnable(GL_DEPTH_TEST);
	
	}

}

void Engine_finnish(){

}
