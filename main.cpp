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
//const float GRASS_DISTANCE = 1.0;
const float GRASS_DISTANCE = 1.1;
const float GRASS_MARGIN = 3.0;
//const float GRASS_DISTANCE = 3.0;
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
//int PAINT_MAP_WIDTH = 30;
//int PAINT_MAP_HEIGHT = 30;
float PAINT_MAP_SCALE = 100.0;
unsigned char *paintMap;

float *terrainHeightMap;
int TERRAIN_HEIGHT_MAP_WIDTH = 100;
int TERRAIN_HEIGHT_MAP_HEIGHT = 100;

unsigned char *terrainTextureData;
int TERRAIN_TEXTURE_WIDTH = 1000;

float terrainMaxHeight = 0.0;

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

//Vec3f lastTorque = getVec3f(0.0, 0.0, 0.0);
//int framesSinceLastSwitch = 0;
//int unstableHits = 0;
bool stabelized = false;
bool stable = false;
int framesSinceHit = 0;

int currentlyHeldRigidBodyIndex = -1;
float holdingDistance = 0.0;

float SWING_ANGLE_RANGE = M_PI / 4.0;
float swingAngle = SWING_ANGLE_RANGE;

void Engine_start(){

	srand(0);

#ifndef RUN_OFFLINE
	Client_init(&game.client);
	//Game_initClient(&game);
#endif

	Game_loadAssets(&game);
	World_loadAssets(&game.world);

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

		Vec3f *points = (Vec3f *)malloc(sizeof(Vec3f) * (width + 1) * (width + 1));

		for(int x = 0; x < width + 1; x++){
			for(int y = 0; y < width + 1; y++){

				int index = y * (width + 1) + x;

				points[index].x = x;
				points[index].z = y;
				points[index].y = getRandom() / TERRAIN_SCALE * 2.0;
				//points[index].y = 0.0;

				if(y == 0 || x == 0 || y == width || x == width){
					points[index].y = 0.0;
				}

				points[index].x /= (float)width;
				points[index].z /= (float)width;

				terrainMaxHeight = fmax(terrainMaxHeight, points[index].y * TERRAIN_SCALE);
			
			}
		}

		//printf("%f\n", terrainMaxHeight);

		int n_triangles = 2 * (width) * (width);

		Vec3f *triangles = (Vec3f *)malloc(sizeof(Vec3f) * 3 * n_triangles);
		Vec2f *textureCoords = (Vec2f *)malloc(sizeof(Vec2f) * 3 * n_triangles);

		int triangleIndex = 0;

		for(int x = 0; x < width; x++){
			for(int y = 0; y < width; y++){

				int pointsIndex1 = y * (width + 1) + x;
				int pointsIndex2 = (y + 1) * (width + 1) + x;
				int pointsIndex3 = y * (width + 1) + (x + 1);
				int pointsIndex4 = (y + 1) * (width + 1) + (x + 1);

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

				Vec2f t1 = getVec2f(x, y) / (float)width;
				Vec2f t2 = getVec2f(x, y + 1.0) / (float)width;
				Vec2f t3 = getVec2f(x + 1.0, y) / (float)width;
				Vec2f t4 = getVec2f(x + 1.0, y + 1.0) / (float)width;

				textureCoords[triangleIndex * 3 * 2 + 0] = t1;
				textureCoords[triangleIndex * 3 * 2 + 1] = t2;
				textureCoords[triangleIndex * 3 * 2 + 2] = t4;
				textureCoords[triangleIndex * 3 * 2 + 3] = t1;
				textureCoords[triangleIndex * 3 * 2 + 4] = t4;
				textureCoords[triangleIndex * 3 * 2 + 5] = t3;

				triangleIndex++;

			}
		}

		free(points);
		
		TriangleMesh triangleMesh;
		triangleMesh.triangles = triangles;
		triangleMesh.n_triangles = n_triangles;
		String_set(triangleMesh.name, "terrain", STRING_SIZE);

		game.world.triangleMeshes.push_back(triangleMesh);

		unsigned char *meshData = generateMeshDataFromTriangleMesh(triangleMesh, textureCoords);

		Model model;

		Model_initFromMeshData(&model, meshData, triangleMesh.n_triangles);

		String_set(model.name, "terrain", STRING_SIZE);

		game.models.push_back(model);

		//free unneeded data
		free(meshData);

	}

	//generate terrain texture
	{
		Texture texture;

		terrainTextureData = (unsigned char *)malloc(sizeof(unsigned char) * 4 * TERRAIN_TEXTURE_WIDTH * TERRAIN_TEXTURE_WIDTH);

		for(int i = 0; i < TERRAIN_TEXTURE_WIDTH * TERRAIN_TEXTURE_WIDTH; i++){
			terrainTextureData[i * 4 + 0] = (unsigned char)(TERRAIN_COLOR.x * 255.0);
			terrainTextureData[i * 4 + 1] = (unsigned char)(TERRAIN_COLOR.y * 255.0);
			terrainTextureData[i * 4 + 2] = (unsigned char)(TERRAIN_COLOR.z * 255.0);
			terrainTextureData[i * 4 + 3] = (unsigned char)(TERRAIN_COLOR.w * 255.0);
		}

		Texture_init(&texture, "terrain", terrainTextureData, TERRAIN_TEXTURE_WIDTH, TERRAIN_TEXTURE_WIDTH);

		game.textures.push_back(texture);
	}

	//generate grass positions
	{
		TriangleMesh *terrainTriangleMesh_p = World_getTriangleMeshPointerByName(&game.world, "terrain");

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
					grassPositions.push_back(pos);
					pos.y += 3.0;

					hits++;
				}

				n++;

				//stepPos += dir;
			}
		}

		//printf("%i, %i\n", hits, n);

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

	World_addObstacle(
		&game.world,
		getVec3f(0.0, 0.0, 0.0),
		TERRAIN_SCALE,
		World_getTriangleMeshIndexByName(&game.world, "terrain"),
		Game_getModelIndexByName(&game, "terrain"),
		Game_getTextureIndexByName(&game, "terrain"),
		getVec4f(1.0, 1.0, 1.0, 1.0)
	);

	World_addObstacle(
		&game.world,
		getVec3f(30.0, 0.0, 20.0),
		3.0,
		World_getTriangleMeshIndexByName(&game.world, "cube"),
		Game_getModelIndexByName(&game, "cube"),
		Game_getTextureIndexByName(&game, "blank"),
		getVec4f(0.7, 0.7, 0.7, 1.0)
	);

	World_addPlayer(&game.world, getVec3f(5.0, 3.0, 5.0), game.client.connectionID);

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

	Game_addTree(&game, getVec3f(15.0, 0.0, 15.0));

	Game_addTree(&game, getVec3f(25.0, 0.0, 15.0));

	Game_addTree(&game, getVec3f(35.0, 0.0, 15.0));

	Game_addTree(&game, getVec3f(90.0, 0.0, 15.0));

	Game_addTree(&game, getVec3f(15.0, 0.0, 90.0));

	Game_addTree(&game, getVec3f(90.0, 0.0, 90.0));

	Game_addTree(&game, getVec3f(40.0, 0.0, 40.0));

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

	//Engine_toggleFullscreen();

}

void Engine_update(float deltaTime){

	//printf("---\n");

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
	if(Engine_keys[ENGINE_KEY_G].downed){
		game.players[0].weapon = (Weapon)((int)game.players[0].weapon + 1);
		if(game.players[0].weapon >= N_WEAPONS){
			game.players[0].weapon = (Weapon)0;
		}
	}
	*/

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
	game.inputsBuffer.clear();
	game.inputsBuffer.push_back(inputs);

#ifndef RUN_OFFLINE
	//send inputs to server
	Client_sendInputsToServer(&game.client, inputs);
	//Game_sendInputsToServer(&game, inputs);

	//apply inputs to player
	{
	}

	/*
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
	*/
#endif

	//simulate inputs on client
	for(int i = 0; i < game.inputsBuffer.size(); i++){

		//printf("%i\n", i);

		Inputs inputs = game.inputsBuffer[i];

		//control player based on inputs
		Player *player_p = World_getPlayerPointerByConnectionID(&game.world, game.client.connectionID);

		//Player_applyInputs(player_p, inputs);

		/*
		if(inputs.shoot == 1){

			if(game.player.weapon == WEAPON_GUN){
				for(int i = 0; i < game.otherPlayers.size(); i++){
					
					Player *player_p = &game.otherPlayers[i];

					BoneTriangleMesh *boneTriangleMesh_p = &game.boneTriangleMeshes[0];
					BoneRig *boneRig_p = &game.boneRigs[0];
					BoneRig *boneRig2_p = &game.boneRigs[1];

					std::vector<Mat4f> boneTransformations = getBoneRigTransformations(boneRig_p, boneRig2_p->originBones);

					float scale = 0.5;

					Mat4f modelMatrix = getModelMatrix(player_p->pos, getVec3f(scale), IDENTITY_QUATERNION);

					Vec3f intersectionPoint;
					Vec3f intersectionNormal;
					float intersectionDistance = -1.0;

					for(int j = 0; j < boneTriangleMesh_p->n_triangles; j++){

						Vec3f checkPoint;

						Vec3f points[3];

						for(int k = 0; k < 3; k++){

							int triangleIndex = j * 3 + k;

							Mat4f jointMatrix = boneTransformations[boneTriangleMesh_p->indices[triangleIndex * 4 + 0] - 1] * boneTriangleMesh_p->weights[triangleIndex].x
								+ boneTransformations[boneTriangleMesh_p->indices[triangleIndex * 4 + 1] - 1] * boneTriangleMesh_p->weights[triangleIndex].y
								+ boneTransformations[boneTriangleMesh_p->indices[triangleIndex * 4 + 2] - 1] * boneTriangleMesh_p->weights[triangleIndex].z
								+ boneTransformations[boneTriangleMesh_p->indices[triangleIndex * 4 + 3] - 1] * boneTriangleMesh_p->weights[triangleIndex].w;

							points[k] = mulVec3fMat4f(boneTriangleMesh_p->triangles[triangleIndex], modelMatrix * jointMatrix, 1.0);

						}

						if(checkLineToTriangleIntersectionVec3f(cameraPos, cameraPos + cameraDirection, points[0], points[1], points[2], &checkPoint)
						&& (intersectionDistance < 0.0
						|| length(checkPoint - cameraPos) < intersectionDistance)){
							intersectionPoint = checkPoint;
							intersectionNormal = cross(points[0] - points[1], points[0] - points[2]);
							intersectionDistance = length(checkPoint - cameraPos);
						}
					}

					intersectionNormal = normalize(intersectionNormal);

					if(intersectionDistance >= 0.0){

						int n_particles = 17 + getRandom() * 5;

						for(int j = 0; j < n_particles; j++){

							Particle particle;
							particle.pos = intersectionPoint;
							particle.velocity = intersectionNormal * 0.1;
							particle.velocity.y += 0.1;
							particle.velocity += normalize(getVec3f(getRandom() - 0.5, getRandom() - 0.5, getRandom() - 0.5)) * 0.05;
							particle.scale = getVec3f(0.1 * (0.8 + 0.4 * getRandom()));
							
							game.bloodParticles.push_back(particle);

						}

					}

				}
			}
			if(game.player.weapon == WEAPON_SWORD){

				swingAngle = -SWING_ANGLE_RANGE;
				//swingAngle = 0.0;

			}

		}
*/

		//World_updatePlayersAndObstaclesCommon(&game.world);

	}


	/*
	//check if rigid bodies are dragged
	for(int i = 0; i < game.rigidBodies.size(); i++){
		
		RigidBody *rigidBody_p = &game.rigidBodies[i];
		TriangleMesh *triangleMesh_p = &game.triangleMeshes[rigidBody_p->triangleMeshIndex];

		rigidBody_p->lastPos = rigidBody_p->pos;
		rigidBody_p->lastOrientation = rigidBody_p->orientation;

		if(Engine_keys[ENGINE_KEY_U].downed){
			rigidBody_p->velocity.y += 0.5;
			rigidBody_p->angularVelocity = getVec3f(getRandom(), getRandom(), getRandom());
		}

		float scale = 1.0;
		Mat4f modelMatrix = getIdentityMat4f();
		modelMatrix *= getScalingMat4f(scale);
		modelMatrix *= getQuaternionMat4f(rigidBody_p->orientation);
		modelMatrix *= getTranslationMat4f(rigidBody_p->pos);

		if(Engine_pointer.downed){

			Vec3f intersectionPoint;
			bool hit = false;

			for(int j = 0; j < triangleMesh_p->n_triangles; j++){
				Vec3f p1 = mulVec3fMat4f(triangleMesh_p->triangles[j * 3 + 0], modelMatrix, 1.0);
				Vec3f p2 = mulVec3fMat4f(triangleMesh_p->triangles[j * 3 + 1], modelMatrix, 1.0);
				Vec3f p3 = mulVec3fMat4f(triangleMesh_p->triangles[j * 3 + 2], modelMatrix, 1.0);


				if(checkLineToTriangleIntersectionVec3f(cameraPos, cameraPos + cameraDirection, p1, p2, p3, &intersectionPoint)){
					hit = true;
				}
			}

			if(hit){
				currentlyHeldRigidBodyIndex = i;
				holdingDistance = getMagVec3f(cameraPos - rigidBody_p->pos);
			}
		}

		if(Engine_pointer.down
		&& currentlyHeldRigidBodyIndex == i){
			rigidBody_p->pos = cameraPos + cameraDirection * holdingDistance;
			rigidBody_p->velocity = getVec3f(0.0, 0.0, 0.0);
			rigidBody_p->angularVelocity = getVec3f(0.0, 0.0, 0.0);
		}

		if(Engine_pointer.upped
		&& currentlyHeldRigidBodyIndex == i){
			rigidBody_p->velocity = cameraPos + cameraDirection * holdingDistance - rigidBody_p->pos;
			if(getMagVec3f(rigidBody_p->velocity) > 0.01){
				rigidBody_p->velocity = normalize(rigidBody_p->velocity) * 0.3;
			}
		}

	}

	if(!Engine_pointer.down){
		currentlyHeldRigidBodyIndex = -1;
	}

	//move rigid bodies
	for(int i = 0; i < game.rigidBodies.size(); i++){
		
		RigidBody *rigidBody_p = &game.rigidBodies[i];

		float resistance = 0.97;

		rigidBody_p->velocity.y += -0.01;

		rigidBody_p->velocity *= resistance;
		rigidBody_p->angularVelocity *= resistance;

		//Vec3f lastPos = rigidBody_p->pos;
		Vec4f lastOrientation = rigidBody_p->orientation;

		rigidBody_p->pos += rigidBody_p->velocity;
		if(getMagVec3f(rigidBody_p->angularVelocity) > 0.0){
			rigidBody_p->orientation = mulQuaternions(getQuaternion(rigidBody_p->angularVelocity, getMagVec3f(rigidBody_p->angularVelocity)), rigidBody_p->orientation);
		}

		rigidBody_p->orientation = normalize(rigidBody_p->orientation);

		//calculate bounding box
		float scale = 1.0;
		Mat4f modelMatrix = getIdentityMat4f();
		modelMatrix *= getScalingMat4f(scale);
		modelMatrix *= getQuaternionMat4f(rigidBody_p->orientation);
		modelMatrix *= getTranslationMat4f(rigidBody_p->pos);

		PointMesh *pointMesh_p = &game.pointMeshes[rigidBody_p->triangleMeshIndex];

		for(int j = 0; j < pointMesh_p->n_points; j++){

			Vec3f point = mulVec3fMat4f(pointMesh_p->points[j], modelMatrix, 1.0);

			if(j == 0){
				rigidBody_p->boundingBox.pos = point;
				rigidBody_p->boundingBox.size = point;
			}

			rigidBody_p->boundingBox.pos.x = fmin(rigidBody_p->boundingBox.pos.x, point.x);
			rigidBody_p->boundingBox.pos.y = fmin(rigidBody_p->boundingBox.pos.y, point.y);
			rigidBody_p->boundingBox.pos.z = fmin(rigidBody_p->boundingBox.pos.z, point.z);

			rigidBody_p->boundingBox.size.x = fmax(rigidBody_p->boundingBox.size.x, point.x);
			rigidBody_p->boundingBox.size.y = fmax(rigidBody_p->boundingBox.size.y, point.y);
			rigidBody_p->boundingBox.size.z = fmax(rigidBody_p->boundingBox.size.z, point.z);

		}

		rigidBody_p->boundingBox.size = rigidBody_p->boundingBox.size - rigidBody_p->boundingBox.pos;

	}

	//check rigid body collisions
	std::vector<Collision> rigidBodyObstacleCollisions;
	std::vector<Collision> rigidBodyRigidBodyCollisions;

	int n_triangleChecks = 0;

	for(int i = 0; i < game.rigidBodies.size(); i++){
		
		RigidBody *rigidBody_p = &game.rigidBodies[i];
		TriangleMesh *triangleMesh_p = &game.triangleMeshes[rigidBody_p->triangleMeshIndex];
		PointMesh *pointMesh_p = &game.pointMeshes[rigidBody_p->triangleMeshIndex];

		//calculate model matrix
		float scale = 1.0;
		Mat4f modelMatrix = getIdentityMat4f();
		modelMatrix *= getScalingMat4f(scale);
		modelMatrix *= getQuaternionMat4f(rigidBody_p->orientation);
		modelMatrix *= getTranslationMat4f(rigidBody_p->pos);

		//check collision vs obstacles
		for(int j = 0; j < game.obstacles.size(); j++){

			Obstacle *obstacle_p = &game.obstacles[j];

			if(!checkBoxBoxCollision(rigidBody_p->boundingBox, obstacle_p->boundingBox)){
				continue;
			}

			Mat4f obstacleMatrix = getIdentityMat4f();
			obstacleMatrix *= getScalingMat4f(obstacle_p->scale);
			obstacleMatrix *= getTranslationMat4f(obstacle_p->pos);

			TriangleMesh *obstacleTriangleMesh_p = &game.triangleMeshes[obstacle_p->triangleMeshIndex];
			PointMesh *obstaclePointMesh_p = &game.pointMeshes[obstacle_p->triangleMeshIndex];

			Collision collision;
			collision.pos = getVec3f(0.0, 0.0, 0.0);
			collision.normal = getVec3f(0.0, 0.0, 0.0);
			int n_hits = 0;

			//float distance = getMagVec3f(rigidBody_p->pos - obstacle_p->pos);

			for(int k = 0; k < obstacleTriangleMesh_p->n_triangles; k++){

				Vec3f a1 = mulVec3fMat4f(obstacleTriangleMesh_p->triangles[k * 3 + 0], obstacleMatrix, 1.0);
				Vec3f a2 = mulVec3fMat4f(obstacleTriangleMesh_p->triangles[k * 3 + 1], obstacleMatrix, 1.0);
				Vec3f a3 = mulVec3fMat4f(obstacleTriangleMesh_p->triangles[k * 3 + 2], obstacleMatrix, 1.0);
				Vec3f aNormal = normalize(cross(a2 - a1, a3 - a1));

				float aRadius = max(dot(a1 - a2, a1 - a2), dot(a1 - a3, a1 - a3));

				if(dot(a1 - rigidBody_p->pos, a1 - rigidBody_p->pos) > aRadius + 2.0 * 2.0){
					continue;
				}

				for(int l = 0; l < triangleMesh_p->n_triangles; l++){

					Vec3f b1 = mulVec3fMat4f(triangleMesh_p->triangles[l * 3 + 0], modelMatrix, 1.0);
					Vec3f b2 = mulVec3fMat4f(triangleMesh_p->triangles[l * 3 + 1], modelMatrix, 1.0);
					Vec3f b3 = mulVec3fMat4f(triangleMesh_p->triangles[l * 3 + 2], modelMatrix, 1.0);

					n_triangleChecks++;

					Vec3f collisionPoint;

					if(checkTriangleTriangleCollisionVec3f(a1, a2, a3, b1, b2, b3, &collisionPoint)){

						collision.pos += collisionPoint;

						if(dot(rigidBody_p->velocity * -1.0, aNormal) > dot(rigidBody_p->velocity * -1.0, collision.normal)
						|| getMagVec3f(collision.normal) < 0.01){
							collision.normal = aNormal;
						}

						n_hits++;

					}

				}
				
			}

			if(n_hits > 0){
				collision.pos /= (float)n_hits;
				collision.normal = normalize(collision.normal);
				collision.index1 = i;
				collision.index2 = j;
				rigidBodyObstacleCollisions.push_back(collision);
			}

		}

		//check collision vs other rigid bodies
		for(int j = 0; j < game.rigidBodies.size(); j++){

			RigidBody *rigidBody2_p = &game.rigidBodies[j];
			TriangleMesh *triangleMesh2_p = &game.triangleMeshes[rigidBody2_p->triangleMeshIndex];

			float scale = 1.0;
			Mat4f modelMatrix2 = getIdentityMat4f();
			modelMatrix2 *= getScalingMat4f(scale);
			modelMatrix2 *= getQuaternionMat4f(rigidBody2_p->orientation);
			modelMatrix2 *= getTranslationMat4f(rigidBody2_p->pos);

			Collision collision;
			collision.pos = getVec3f(0.0, 0.0, 0.0);
			collision.normal = getVec3f(0.0, 0.0, 0.0);
			int n_hits = 0;

			if(i != j
			&& checkBoxBoxCollision(rigidBody_p->boundingBox, rigidBody2_p->boundingBox)){
				for(int k = 0; k < triangleMesh2_p->n_triangles; k++){

					Vec3f a1 = mulVec3fMat4f(triangleMesh2_p->triangles[k * 3 + 0], modelMatrix2, 1.0);
					Vec3f a2 = mulVec3fMat4f(triangleMesh2_p->triangles[k * 3 + 1], modelMatrix2, 1.0);
					Vec3f a3 = mulVec3fMat4f(triangleMesh2_p->triangles[k * 3 + 2], modelMatrix2, 1.0);
					Vec3f aNormal = normalize(cross(a2 - a1, a3 - a1));

					float aRadius = max(dot(a1 - a2, a1 - a2), dot(a1 - a3, a1 - a3));

					if(dot(a1 - rigidBody_p->pos, a1 - rigidBody_p->pos) > aRadius + 2.0 * 2.0){
						continue;
					}

					for(int l = 0; l < triangleMesh_p->n_triangles; l++){

						Vec3f b1 = mulVec3fMat4f(triangleMesh_p->triangles[l * 3 + 0], modelMatrix, 1.0);
						Vec3f b2 = mulVec3fMat4f(triangleMesh_p->triangles[l * 3 + 1], modelMatrix, 1.0);
						Vec3f b3 = mulVec3fMat4f(triangleMesh_p->triangles[l * 3 + 2], modelMatrix, 1.0);

						n_triangleChecks++;

						Vec3f collisionPoint;

						if(checkTriangleTriangleCollisionVec3f(a1, a2, a3, b1, b2, b3, &collisionPoint)){

							collision.pos += collisionPoint;

							if(dot(rigidBody_p->velocity * -1.0, aNormal) > dot(rigidBody_p->velocity * -1.0, collision.normal)
							|| getMagVec3f(collision.normal) < 0.01){
								collision.normal = aNormal;
							}

							n_hits++;

						}
					
					}

				}
			}

			if(n_hits > 0){
				collision.pos /= (float)n_hits;
				collision.normal = normalize(collision.normal);
				collision.index1 = i;
				collision.index2 = j;
				rigidBodyRigidBodyCollisions.push_back(collision);
			
			}
			
		}

	}

	//printf("triangle checks: %i\n", n_triangleChecks);

	//resolve rigid body collisions
	for(int i = 0; i < game.rigidBodies.size(); i++){
		
		RigidBody *rigidBody_p = &game.rigidBodies[i];
			
		float elasticity = 0.1;
		float torqueFactor = 0.5;

		Vec3f collisionNormal = getVec3f(0.0, 0.0, 0.0);
		Vec3f collisionPoint = getVec3f(0.0, 0.0, 0.0);
		int n_hits = 0;
		
		for(int j = 0; j < rigidBodyObstacleCollisions.size(); j++){

			Collision *collision_p = &rigidBodyObstacleCollisions[j];

			if(collision_p->index1 == i){
				collisionNormal += collision_p->normal;
				collisionPoint += collision_p->pos;
				n_hits++;
			}

		}

		Vec3f rigidBodyForces = getVec3f(0.0, 0.0, 0.0);
		Vec3f rigidBodyTorque = getVec3f(0.0, 0.0, 0.0);
		bool hitByRigidBody = false;

		for(int j = 0; j < rigidBodyRigidBodyCollisions.size(); j++){

			Collision *collision_p = &rigidBodyRigidBodyCollisions[j];

			Vec3f collisionDiff = rigidBody_p->pos - collision_p->pos;

			if(collision_p->index1 == i){

				RigidBody *rigidBody2_p = &game.rigidBodies[collision_p->index2];
				Vec3f force = collision_p->normal * getMagVec3f(rigidBody2_p->velocity) * elasticity;
				Vec3f torque = cross(force, collisionDiff);

				rigidBodyForces += force;
				//rigidBodyTorque += getVec3f(0.0, 1.0, 0.0);
				rigidBodyTorque += torque;
				//hitByRigidBody = true;

				collisionNormal += collision_p->normal;
				collisionPoint += collision_p->pos;
				n_hits++;

			}

		}

		if(n_hits > 0){

			//rigidBody_p->pos += collisionNormal * fabs(dot(rigidBody_p->velocity, collisionNormal));
			rigidBody_p->pos = rigidBody_p->lastPos;
			rigidBody_p->orientation = rigidBody_p->lastOrientation;
			//rigidBody_p->angularVelocity *= 0.9;
			//rigidBody_p->velocity *= 0.9;

			collisionPoint /= (float)n_hits;
			collisionNormal = normalize(collisionNormal);

			Vec3f collisionDiff = rigidBody_p->pos - collisionPoint;
			Vec3f force = collisionNormal * getMagVec3f(rigidBody_p->velocity) * (1.0 + elasticity);
			Vec3f torque = cross(force, collisionDiff);

			if(dot(rigidBody_p->lastTorque, torque) < 0.0
			&& dot(rigidBody_p->velocity, collisionNormal) < 0.0
			&& rigidBody_p->framesSinceHit < 2){
				torque *= 0.1;
				rigidBody_p->angularVelocity *= 0.1;
				rigidBody_p->velocity *= 0.1;
				force *= 0.1;
			}

			rigidBody_p->angularVelocity += torque * torqueFactor;

			rigidBody_p->velocity += cross(torque, collisionDiff) * torqueFactor;
			rigidBody_p->velocity += force;

			rigidBody_p->lastTorque = torque;
			rigidBody_p->framesSinceHit = 0;
		
		}

		if(hitByRigidBody){
			//rigidBody_p->pos = rigidBody_p->lastPos;
			//rigidBody_p->orientation = rigidBody_p->lastOrientation;
			rigidBody_p->velocity += rigidBodyForces;
			//rigidBody_p->angularVelocity += rigidBodyTorque * torqueFactor;
		}

		rigidBody_p->framesSinceHit++;

	}
	*/

	//PERFORMANCE TEST CAMERA
	//game.player.pos = getVec3f(90.0, 5.0, 23.0);
	//cameraDirection = getVec3f(-0.9, -0.33, 0.26);

#ifdef RUN_OFFLINE
		game.inputsBuffer.clear();
#endif

	//update camera position
	{
		Player *player_p = World_getPlayerPointerByConnectionID(&game.world, game.client.connectionID);

		cameraPos = player_p->pos;
		cameraPos.y += player_p->height;
	}

	//update blood particles
	for(int i = 0; i < game.bloodParticles.size(); i++){
		
		Particle *particle_p = &game.bloodParticles[i];

		particle_p->velocity.y -= BLOOD_PARTICLE_GRAVITY;

		particle_p->pos += particle_p->velocity;

		//check distance to ground to see if further checks are necessary
		if(particle_p->pos.y <= terrainMaxHeight){

			//check collision with grass
			Vec4f hitPosition;
			bool hitGrass = false;

			for(int j = 0; j < grassPositions.size(); j++){
				if(square(grassPositions[j].x - particle_p->pos.x) + square(grassPositions[j].z - particle_p->pos.z) < 1.0
				&& particle_p->pos.y < grassPositions[j].y - 1.2 + 2.0 * (grassPositions[j].w / 100.0)){
					hitGrass = true;
					hitPosition = grassPositions[j];
					break;
				}
			}

			if(hitGrass){

				Vec2f paintMapPos = getVec2f(particle_p->pos.x * PAINT_MAP_WIDTH, particle_p->pos.z * PAINT_MAP_HEIGHT) / PAINT_MAP_SCALE;

				int index = (int)paintMapPos.y * PAINT_MAP_WIDTH + (int)paintMapPos.x;

				paintMap[index] = (unsigned char)(255.0 * (fmax(floor(hitPosition.w) / 100.0 - 0.2 + getRandom() * 0.2, 0.0)));

				game.bloodParticles.erase(game.bloodParticles.begin() + i);
				i--;
				continue;
			
			}

			//check collision with ground
			TriangleMesh *triangleMesh_p = World_getTriangleMeshPointerByName(&game.world, "terrain");

			Vec3f intersectionPoint;
			checkClosestLineTriangleMeshIntersection(particle_p->pos, getVec3f(0.0, -1.0, 0.0), *triangleMesh_p, getVec3f(0.0, 0.0, 0.0), TERRAIN_SCALE, &intersectionPoint, NULL);

			if(particle_p->pos.y < intersectionPoint.y){

				for(int x = 0; x < 3; x++){
					for(int z = 0; z < 3; z++){

						if(getRandom() > 0.3){
							continue;
						}

						int terrainTextureIndex = TERRAIN_TEXTURE_WIDTH * (int)((float)TERRAIN_TEXTURE_WIDTH * particle_p->pos.z / TERRAIN_SCALE + z) + (int)((float)TERRAIN_TEXTURE_WIDTH * particle_p->pos.x / TERRAIN_SCALE + x);

						if(terrainTextureIndex > 0
						&& terrainTextureIndex < TERRAIN_TEXTURE_WIDTH * TERRAIN_TEXTURE_WIDTH){
							terrainTextureData[terrainTextureIndex * 4 + 0] = (unsigned char)(255.0 * 0.9);
							terrainTextureData[terrainTextureIndex * 4 + 1] = 0;
							terrainTextureData[terrainTextureIndex * 4 + 2] = 0;
							terrainTextureData[terrainTextureIndex * 4 + 3] = 255;
						}

					}
				}

			}

		}

		if(particle_p->pos.y < 0.0){
			game.bloodParticles.erase(game.bloodParticles.begin() + i);
			i--;
			continue;
		}

	}

	//update grass particles
	for(int i = 0; i < game.grassParticles.size(); i++){

		Particle *particle_p = &game.grassParticles[i];

		particle_p->velocity.y -= 0.01;

		particle_p->pos += particle_p->velocity;

		particle_p->color.w -= 0.04;

		if(particle_p->color.w < 0.0){
			game.grassParticles.erase(game.grassParticles.begin() + i);
			i--;
			continue;
		}
	
	}

	//update sword
	{
		if(swingAngle < SWING_ANGLE_RANGE){

			TriangleMesh *triangleMesh_p = World_getTriangleMeshPointerByName(&game.world, "quad");

			Mat4f swordMatrix = getSwordMatrix(cameraPos, cameraDirection, swingAngle);

			for(int i = 0; i < triangleMesh_p->n_triangles; i++){

				Vec3f p1 = mulVec3fMat4f(triangleMesh_p->triangles[i * 3 + 0], swordMatrix, 1.0);
				Vec3f p2 = mulVec3fMat4f(triangleMesh_p->triangles[i * 3 + 1], swordMatrix, 1.0);
				Vec3f p3 = mulVec3fMat4f(triangleMesh_p->triangles[i * 3 + 2], swordMatrix, 1.0);

				for(int j = 0; j < grassPositions.size(); j++){

					if(square(grassPositions[j].x - cameraPos.x) + square(grassPositions[j].z - cameraPos.z) > 10.0){
						continue;
					}

					Vec3f intersectionPoint;

					Vec3f checkPos = getVec3f(grassPositions[j].x, 0.0, grassPositions[j].z);

					if(checkLineToTriangleIntersectionVec3f(checkPos, checkPos + getVec3f(0.0, 1.0, 0.0), p1, p2, p3, &intersectionPoint)){

						Vec4f *position_p = &grassPositions[j];

						float grassHeight = floor(position_p->w);
						float cutHeight = fmax(fmin((intersectionPoint.y - (position_p->y - 1.0)) * 100.0 / 2.0, 100.0), 0.0);
						position_p->w = position_p->w - grassHeight + fmin(cutHeight, grassHeight);

						//add grass particles
						if(grassHeight > cutHeight){
							for(int j = 0; j < 3; j++){

								float height = (grassHeight - cutHeight) / (100.0);
								float posY = -1.0 + height + (cutHeight / 100.0) * 2.0;

								Particle particle;
								particle.pos = getVec3f(position_p->x, position_p->y + posY, position_p->z);
								particle.velocity = getVec3f((getRandom() - 0.5) * 0.01, 0.08 + getRandom() * 0.05, (getRandom() - 0.5) * 0.01);
								particle.scale = getVec3f(1.0, height, 1.0);
								particle.orientation = getQuaternion(getVec3f(0.0, 1.0, 0.0), -(float)j * M_PI / 3.0 + 0.0);
								particle.color = getVec4f(1.0, 1.0, 1.0, 1.0);
								particle.textureY = 1.0 - grassHeight / 100.0;
								particle.textureSizeY = height;

								game.grassParticles.push_back(particle);

							}
						}

					}

				}

			}

			swingAngle += 0.09;
			//swingAngle += 0.012;
		}
	}

	//sort grass positions by depth
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

	//sort leaf transformations by depth
	for(int i = 0; i < game.trees.size(); i++){

		Tree *tree_p = &game.trees[i];

		int n_buckets = 200;
		int bucketSizes[n_buckets];
		int bucketOffsets[n_buckets];
		int bucketCounts[n_buckets];
		memset(bucketSizes, 0, sizeof(int) * n_buckets);
		memset(bucketOffsets, 0, sizeof(int) * n_buckets);
		memset(bucketCounts, 0, sizeof(int) * n_buckets);

		unsigned char depths[tree_p->leafTransformations.size()];
		memset(depths, 0, sizeof(unsigned char) * tree_p->leafTransformations.size());
		
		for(int j = 0; j < tree_p->leafTransformations.size(); j++){
			
			Vec4f projectedPos = getVec4f(tree_p->leafTransformations[i][0][3], tree_p->leafTransformations[i][1][3], tree_p->leafTransformations[i][2][3], tree_p->leafTransformations[i][3][3]);
			projectedPos = cameraMatrix * projectedPos;

			unsigned char depth = (unsigned char)fabs(projectedPos.z);
			//Vec4f_log(pos);
			//plog(tree_p->leafTransformations[j]);

		}

		//printf("tree :)\n");

	}

}

void Engine_draw(){

	glBindTexture(GL_TEXTURE_2D, paintMapTexture.ID);
	glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, PAINT_MAP_WIDTH, PAINT_MAP_HEIGHT, GL_RED, GL_UNSIGNED_BYTE, paintMap);

	{
		Texture *texture_p = Game_getTexturePointerByName(&game, "terrain");
		glBindTexture(GL_TEXTURE_2D, texture_p->ID);
		glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, TERRAIN_TEXTURE_WIDTH, TERRAIN_TEXTURE_WIDTH, GL_RGBA, GL_UNSIGNED_BYTE, terrainTextureData);
	}

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

		Mat4f lightViewMatrix = getTranslationMat4f(round(-cameraPos.x), round(-cameraPos.y), round(-cameraPos.z));
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

		//draw opaque meshes with standard shader
		if(renderStage == RENDER_STAGE_BIG_SHADOWS
		|| renderStage == RENDER_STAGE_SHADOWS
		|| renderStage == RENDER_STAGE_SCENE){

			Shader *shader_p = Game_getShaderPointerByName(&game, "model");
			if(renderStage == RENDER_STAGE_SHADOWS
			|| renderStage == RENDER_STAGE_BIG_SHADOWS){
				shader_p = Game_getShaderPointerByName(&game, "model-shadow");
			}

			glUseProgram(shader_p->ID);

			GL3D_uniformFloat(shader_p->ID, "grassShadowStrength", GRASS_SHADOW_STRENGTH);

			GL3D_uniformTexture(shader_p->ID, "shadowMapDepthTexture", 1, shadowMapDepthTexture.ID);
			GL3D_uniformTexture(shader_p->ID, "grassShadowMapDepthTexture", 2, grassShadowMapDepthTexture.ID);
			GL3D_uniformTexture(shader_p->ID, "bigShadowMapDepthTexture", 3, bigShadowMapDepthTexture.ID);

			GL3D_uniformMat4f(shader_p->ID, "viewMatrix", viewMatrix);
			GL3D_uniformMat4f(shader_p->ID, "lightViewMatrix", lightViewMatrix);
			GL3D_uniformMat4f(shader_p->ID, "bigLightViewMatrix", bigLightViewMatrix);

			//draw obstacles
			for(int i = 0; i < game.world.obstacles.size(); i++){

				Obstacle *obstacle_p = &game.world.obstacles[i];

				Mat4f modelMatrix = getModelMatrix(obstacle_p->pos, getVec3f(obstacle_p->scale), IDENTITY_QUATERNION);

				Model *model_p = &game.models[obstacle_p->modelIndex];
				Texture *texture_p = &game.textures[obstacle_p->textureIndex];

				glBindBuffer(GL_ARRAY_BUFFER, model_p->VBO);
				glBindVertexArray(model_p->VAO);

				GL3D_uniformTexture(shader_p->ID, "colorTexture", 0, texture_p->ID);

				GL3D_uniformMat4f(shader_p->ID, "modelMatrix", modelMatrix);

				GL3D_uniformVec4f(shader_p->ID, "inputColor", obstacle_p->color);

				glDrawArrays(GL_TRIANGLES, 0, model_p->numberOfTriangles * 3);

			}

			//draw blood particles
			for(int i = 0; i < game.bloodParticles.size(); i++){

				Particle *particle_p = &game.bloodParticles[i];

				Mat4f modelMatrix = getModelMatrix(particle_p->pos, particle_p->scale, IDENTITY_QUATERNION);

				Model *model_p = Game_getModelPointerByName(&game, "cube");
				Texture *texture_p = Game_getTexturePointerByName(&game, "blank");

				glBindBuffer(GL_ARRAY_BUFFER, model_p->VBO);
				glBindVertexArray(model_p->VAO);

				GL3D_uniformTexture(shader_p->ID, "colorTexture", 0, texture_p->ID);

				GL3D_uniformMat4f(shader_p->ID, "modelMatrix", modelMatrix);

				GL3D_uniformVec4f(shader_p->ID, "inputColor", getVec4f(0.9, 0.0, 0.0, 1.0));

				glDrawArrays(GL_TRIANGLES, 0, model_p->numberOfTriangles * 3);

			}

		}

		//draw grass
		if((renderStage == RENDER_STAGE_GRASS_SHADOWS
		|| renderStage == RENDER_STAGE_SCENE) && false){
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

				glDrawArraysInstanced(GL_TRIANGLES, 0, model_p->numberOfTriangles * 3, tree_p->leafTransformationsTextureBuffer.n_elements);

			}

			glEnable(GL_CULL_FACE);
		
		}

		/*
		float t = (1.0 + sin(gameTime * 0.1)) * 0.5;

		//std::vector<Bone> interpolatedBones = getInterpolatedBones(game.boneModels[0].bones, game.boneModels[1].bones, t);

		std::vector<Mat4f> bindMatrices = getBindMatricesFromBones(interpolatedBones);

		std::vector<Mat4f> boneTransformations;

		for(int i = 0; i < bindMatrices.size(); i++){
			Mat4f transformation = game.boneModels[0].inverseBindMatrices[i];
			transformation *= bindMatrices[i];
			boneTransformations.push_back(transformation);
		}
		*/

		/*
		//draw other players
		if(renderStage == RENDER_STAGE_SCENE){

			Shader *shader_p = Game_getShaderPointerByName(&game, "bone");

			glUseProgram(shader_p->ID);

			GL3D_uniformMat4f(shader_p->ID, "viewMatrix", viewMatrix);

			for(int i = 0; i < game.otherPlayers.size(); i++){

				Player *player_p = &game.otherPlayers[i];

				float scale = 0.5;

				Mat4f modelMatrix = getModelMatrix(player_p->pos, getVec3f(scale), IDENTITY_QUATERNION);

				BoneModel *model_p = &game.boneModels[0];

				BoneRig *boneRig_p = &game.boneRigs[0];
				BoneRig *boneRig2_p = &game.boneRigs[1];

				std::vector<Mat4f> boneTransformations = getBoneRigTransformations(boneRig_p, boneRig2_p->originBones);

				glBindBuffer(GL_ARRAY_BUFFER, model_p->VBO);
				glBindVertexArray(model_p->VAO);

				GL3D_uniformMat4f(shader_p->ID, "modelMatrix", modelMatrix);

				GL3D_uniformVec4f(shader_p->ID, "inputColor", getVec4f(0.7, 0.7, 0.7, 1.0));

				GL3D_uniformMat4fArray(shader_p->ID, "boneTransformations", &boneTransformations[0], boneTransformations.size());

				glDrawArrays(GL_TRIANGLES, 0, model_p->n_triangles * 3);
			
			}

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

		//draw rigid bodies
		if(renderStage == RENDER_STAGE_SCENE){
			for(int i = 0; i < game.rigidBodies.size(); i++){

				RigidBody *rigidBody_p = &game.rigidBodies[i];

				float scale = 1.0;

				Mat4f modelMatrix = getModelMatrix(rigidBody_p->pos, getVec3f(1.0), rigidBody_p->orientation);

				Shader *shader_p = Game_getShaderPointerByName(&game, "model");
				if(renderStage == RENDER_STAGE_SHADOWS
				|| renderStage == RENDER_STAGE_BIG_SHADOWS){
					shader_p = Game_getShaderPointerByName(&game, "model-shadow");
				}

				glUseProgram(shader_p->ID);
				
				Model *model_p = &game.models[rigidBody_p->modelIndex];

				Texture *texture_p = &game.textures[rigidBody_p->textureIndex];
				//Texture *alphaTexture_p = &game.textures[rigidBody_p->alphaTextureIndex];

				glBindBuffer(GL_ARRAY_BUFFER, model_p->VBO);
				glBindVertexArray(model_p->VAO);

				GL3D_uniformFloat(shader_p->ID, "grassShadowStrength", GRASS_SHADOW_STRENGTH);

				GL3D_uniformTexture(shader_p->ID, "colorTexture", 0, texture_p->ID);
				//GL3D_uniformTexture(shader_p->ID, "alphaTexture", 1, alphaTexture_p->ID);
				GL3D_uniformTexture(shader_p->ID, "shadowMapDepthTexture", 1, shadowMapDepthTexture.ID);
				GL3D_uniformTexture(shader_p->ID, "grassShadowMapDepthTexture", 2, grassShadowMapDepthTexture.ID);
				GL3D_uniformTexture(shader_p->ID, "bigShadowMapDepthTexture", 3, bigShadowMapDepthTexture.ID);

				GL3D_uniformMat4f(shader_p->ID, "viewMatrix", viewMatrix);
				GL3D_uniformMat4f(shader_p->ID, "lightViewMatrix", lightViewMatrix);
				GL3D_uniformMat4f(shader_p->ID, "bigLightViewMatrix", bigLightViewMatrix);
				GL3D_uniformMat4f(shader_p->ID, "modelMatrix", modelMatrix);

				GL3D_uniformVec4f(shader_p->ID, "inputColor", getVec4f(0.7, 0.7, 0.7, 1.0));

				glDrawArrays(GL_TRIANGLES, 0, model_p->numberOfTriangles * 3);

				
			}
		
		}

		//draw grass particles
		if(renderStage == RENDER_STAGE_SCENE){

			glDisable(GL_CULL_FACE);

			Shader *shader_p = Game_getShaderPointerByName(&game, "grass-particle");

			glUseProgram(shader_p->ID);

			GL3D_uniformFloat(shader_p->ID, "grassShadowStrength", GRASS_SHADOW_STRENGTH);

			GL3D_uniformTexture(shader_p->ID, "shadowMapDepthTexture", 1, shadowMapDepthTexture.ID);
			GL3D_uniformTexture(shader_p->ID, "grassShadowMapDepthTexture", 2, grassShadowMapDepthTexture.ID);
			GL3D_uniformTexture(shader_p->ID, "bigShadowMapDepthTexture", 3, bigShadowMapDepthTexture.ID);
			GL3D_uniformTexture(shader_p->ID, "paintMapTexture", 4, paintMapTexture.ID);

			GL3D_uniformMat4f(shader_p->ID, "viewMatrix", viewMatrix);
			GL3D_uniformMat4f(shader_p->ID, "lightViewMatrix", lightViewMatrix);
			GL3D_uniformMat4f(shader_p->ID, "bigLightViewMatrix", bigLightViewMatrix);

			for(int i = 0; i < game.grassParticles.size(); i++){

				Particle *particle_p = &game.grassParticles[i];

				float scale = 1.0;

				Mat4f modelMatrix = getModelMatrix(particle_p->pos, particle_p->scale, particle_p->orientation);

				Model *model_p = Game_getModelPointerByName(&game, "quad");

				Texture *texture_p = Game_getTexturePointerByName(&game, "grass");

				glBindBuffer(GL_ARRAY_BUFFER, model_p->VBO);
				glBindVertexArray(model_p->VAO);

				GL3D_uniformTexture(shader_p->ID, "colorTexture", 0, texture_p->ID);

				GL3D_uniformMat4f(shader_p->ID, "modelMatrix", modelMatrix);

				GL3D_uniformVec4f(shader_p->ID, "inputColor", particle_p->color);

				GL3D_uniformFloat(shader_p->ID, "textureY", particle_p->textureY);
				GL3D_uniformFloat(shader_p->ID, "textureSizeY", particle_p->textureSizeY);

				glDrawArrays(GL_TRIANGLES, 0, model_p->numberOfTriangles * 3);

				
			}

			glEnable(GL_CULL_FACE);

		}

		//draw sword
		if(renderStage == RENDER_STAGE_SCENE
		&& swingAngle < SWING_ANGLE_RANGE){

			Shader *shader_p = Game_getShaderPointerByName(&game, "model");
			if(renderStage == RENDER_STAGE_SHADOWS
			|| renderStage == RENDER_STAGE_BIG_SHADOWS){
				shader_p = Game_getShaderPointerByName(&game, "model-shadow");
			}

			glUseProgram(shader_p->ID);

			GL3D_uniformFloat(shader_p->ID, "grassShadowStrength", GRASS_SHADOW_STRENGTH);

			GL3D_uniformTexture(shader_p->ID, "shadowMapDepthTexture", 1, shadowMapDepthTexture.ID);
			GL3D_uniformTexture(shader_p->ID, "grassShadowMapDepthTexture", 2, grassShadowMapDepthTexture.ID);
			GL3D_uniformTexture(shader_p->ID, "bigShadowMapDepthTexture", 3, bigShadowMapDepthTexture.ID);

			GL3D_uniformMat4f(shader_p->ID, "viewMatrix", viewMatrix);
			GL3D_uniformMat4f(shader_p->ID, "lightViewMatrix", lightViewMatrix);
			GL3D_uniformMat4f(shader_p->ID, "bigLightViewMatrix", bigLightViewMatrix);

			Mat4f modelMatrix = getSwordMatrix(cameraPos, cameraDirection, swingAngle);

			Model *model_p = Game_getModelPointerByName(&game, "sword");
			Texture *texture_p = Game_getTexturePointerByName(&game, "blank");

			glBindBuffer(GL_ARRAY_BUFFER, model_p->VBO);
			glBindVertexArray(model_p->VAO);

			GL3D_uniformTexture(shader_p->ID, "colorTexture", 0, texture_p->ID);

			GL3D_uniformMat4f(shader_p->ID, "modelMatrix", modelMatrix);

			GL3D_uniformVec4f(shader_p->ID, "inputColor", getVec4f(0.7, 0.7, 0.7, 1.0));

			glDrawArrays(GL_TRIANGLES, 0, model_p->numberOfTriangles * 3);
			
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

			/*
			if(game.player.weapon == WEAPON_GUN){
				Renderer2D_drawText(&renderer2D, "Gun", WIDTH - 220, 40, 60, font);
			}
			if(game.player.weapon == WEAPON_SWORD){
				Renderer2D_drawText(&renderer2D, "Sword", WIDTH - 220, 40, 60, font);
			}
			*/
		
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
