#include "engine/engine.h"
#include "engine/geometry.h"
#include "engine/3d.h"
#include "engine/shaders.h"
#include "engine/renderer2d.h"
#include "engine/igui.h"
#include "engine/strings.h"
#include "engine/files.h"

#include "stdio.h"
#include "stdlib.h"
#include "math.h"
#include "time.h"
#include <cstring>
#include <vector>
float PLAYER_HEIGHT_STANDING = 2.0;
float PLAYER_HEIGHT_CROUCHING = 1.0;
float PLAYER_HEIGHT_SPEED = 0.1;
float PLAYER_GRAVITY = 0.005;
float PLAYER_JUMP_SPEED = 0.2;

Vec3f playerPos = { 5.0, 10.0, 5.0 };
Vec3f playerVelocity = { 0.0, 0.0, 0.0 };
bool playerOnGround = false;
float playerHeight = PLAYER_HEIGHT_STANDING;

float windTime = 0.0;

Model terrainModel;
float TERRAIN_SCALE = 100.0;
Texture terrainHeightMapTexture;
Vec3f *terrainTriangles;
//THIS VALUE (N_GRASS) WILL BE MODIFIED DANGER DANGER!!!
int N_GRASS = 10000;
TextureBuffer grassPositionsTextureBuffer;

int WIDTH = 1920;
int HEIGHT = 1080;

unsigned int modelShader;
unsigned int shadowMapShader;
unsigned int grassShader;

unsigned int shadowMapFBO;
unsigned int shadowMapStaticFBO;
Texture shadowMapDepthTexture;
Texture staticShadowMapDepthTexture;
Texture shadowMapDepthTextureStatic;
unsigned int transparentShadowMapFBO;
unsigned int transparentShadowMapStaticFBO;
Texture transparentShadowMapDepthTexture;
Texture transparentShadowMapColorTexture;
Texture transparentShadowMapDepthTextureStatic;
Texture transparentShadowMapColorTextureStatic;
int SHADOW_MAP_WIDTH = 1000;
int SHADOW_MAP_HEIGHT = 1000;
float shadowMapScale = 10.0;

bool staticShadowMapInited = false;

float fov = M_PI / 4;
Vec3f lightPos = { -10.0, 20.0, -10.0 };
Vec3f lightDirection = { 0.3, -1.0, 0.5 };

float PLAYER_LOOK_SPEED = 0.0015;
float PLAYER_SPEED = 0.15;

Vec3f cameraPos = getVec3f(4.0, 5.0, 0.0);
Vec3f cameraDirection = getVec3f(0.0, 0.0, 1.0);
Vec2f cameraRotation = getVec2f(M_PI / 2.0, 0.0);

std::vector<Model> models;
std::vector<Texture> textures;

Vec3f pos = getVec3f(0.0, 0.0, 10.0);
Vec3f rotation = getVec3f(0.0, 0.0, 0.0);
float voxelScale = 0.05;

int TERRAIN_WIDTH = 20;

void Engine_start(){

	//generate terrain
	{

		Vec3f *terrainPoints = (Vec3f *)malloc(sizeof(Vec3f) * TERRAIN_WIDTH * TERRAIN_WIDTH);

		for(int x = 0; x < TERRAIN_WIDTH; x++){
			for(int y = 0; y < TERRAIN_WIDTH; y++){

				int index = y * TERRAIN_WIDTH + x;

				terrainPoints[index].x = x;
				terrainPoints[index].z = y;
				terrainPoints[index].y = getRandom() / TERRAIN_SCALE * 2.0;

				terrainPoints[index].x /= (float)TERRAIN_WIDTH;
				terrainPoints[index].z /= (float)TERRAIN_WIDTH;
			
			}
		}

		int componentSize = 8;
		int n_terrainTriangles = 2 * (TERRAIN_WIDTH - 1) * (TERRAIN_WIDTH - 1);

		int terrainMeshSize = sizeof(float) * componentSize * 3 * n_terrainTriangles;
		float *terrainMesh = (float *)malloc(terrainMeshSize);
		memset(terrainMesh, 0, terrainMeshSize);

		terrainTriangles = (Vec3f *)malloc(sizeof(Vec3f) * 3 * 2 * (TERRAIN_WIDTH - 1) * (TERRAIN_WIDTH - 1));

		int meshIndex = 0;

		for(int x = 0; x < TERRAIN_WIDTH - 1; x++){
			for(int y = 0; y < TERRAIN_WIDTH - 1; y++){

				int pointsIndex1 = y * TERRAIN_WIDTH + x;
				int pointsIndex2 = (y + 1) * TERRAIN_WIDTH + x;
				int pointsIndex3 = y * TERRAIN_WIDTH + (x + 1);
				int pointsIndex4 = (y + 1) * TERRAIN_WIDTH + (x + 1);

				Vec3f point1 = terrainPoints[pointsIndex1];
				Vec3f point2 = terrainPoints[pointsIndex2];
				Vec3f point3 = terrainPoints[pointsIndex3];
				Vec3f point4 = terrainPoints[pointsIndex4];

				Vec3f normal1 = getCrossVec3f(getSubVec3f(point1, point2), getSubVec3f(point1, point4));
				Vec3f normal2 = getCrossVec3f(getSubVec3f(point1, point4), getSubVec3f(point1, point3));
				Vec3f_normalize(&normal1);
				Vec3f_normalize(&normal2);

				//copy into mesh
				memcpy(terrainMesh + meshIndex * componentSize * 3 * 2 + componentSize * 0, &point1, sizeof(Vec3f));
				memcpy(terrainMesh + meshIndex * componentSize * 3 * 2 + componentSize * 1, &point2, sizeof(Vec3f));
				memcpy(terrainMesh + meshIndex * componentSize * 3 * 2 + componentSize * 2, &point4, sizeof(Vec3f));
				memcpy(terrainMesh + meshIndex * componentSize * 3 * 2 + componentSize * 3, &point1, sizeof(Vec3f));
				memcpy(terrainMesh + meshIndex * componentSize * 3 * 2 + componentSize * 4, &point4, sizeof(Vec3f));
				memcpy(terrainMesh + meshIndex * componentSize * 3 * 2 + componentSize * 5, &point3, sizeof(Vec3f));

				memcpy(terrainMesh + meshIndex * componentSize * 3 * 2 + componentSize * 0 + 5, &normal1, sizeof(Vec3f));
				memcpy(terrainMesh + meshIndex * componentSize * 3 * 2 + componentSize * 1 + 5, &normal1, sizeof(Vec3f));
				memcpy(terrainMesh + meshIndex * componentSize * 3 * 2 + componentSize * 2 + 5, &normal1, sizeof(Vec3f));
				memcpy(terrainMesh + meshIndex * componentSize * 3 * 2 + componentSize * 3 + 5, &normal2, sizeof(Vec3f));
				memcpy(terrainMesh + meshIndex * componentSize * 3 * 2 + componentSize * 4 + 5, &normal2, sizeof(Vec3f));
				memcpy(terrainMesh + meshIndex * componentSize * 3 * 2 + componentSize * 5 + 5, &normal2, sizeof(Vec3f));

				//push into triangles
				terrainTriangles[meshIndex * 3 * 2 + 0] = point1;
				terrainTriangles[meshIndex * 3 * 2 + 1] = point2;
				terrainTriangles[meshIndex * 3 * 2 + 2] = point4;
				terrainTriangles[meshIndex * 3 * 2 + 3] = point1;
				terrainTriangles[meshIndex * 3 * 2 + 4] = point4;
				terrainTriangles[meshIndex * 3 * 2 + 5] = point3;

				meshIndex++;

			}
		}

		Model_initFromMeshData(&terrainModel, (unsigned char *)terrainMesh, n_terrainTriangles);

		//generate grass positions
		std::vector<Vec4f> grassPositions;

		for(int i = 0; i < N_GRASS; i++){

			Vec2f mapPos = getVec2f(
				floor(i / (int)(sqrt(N_GRASS))),
				floor(i % (int)(sqrt(N_GRASS)))
			);

			mapPos.x += getRandom() * 0.5;
			mapPos.y += getRandom() * 0.5;

			//Vec2f_mulByFloat(&mapPos, TERRAIN_SCALE / sqrt(N_GRASS));

			Vec3f rayOrigin = getVec3f(mapPos.x / sqrt(N_GRASS), 10.0, mapPos.y / sqrt(N_GRASS));
			Vec3f rayDirection = getVec3f(0.0, -1.0, 0.0);
			Vec3f_normalize(&rayDirection);

			//Vec3f_log(rayOrigin);

			Vec3f intersectionPoint;
			bool hit = false;

			//printf("test\n");

			{

				bool somethingHit = false;
				bool hasTriangle = false;

				for(int j = 0; j < n_terrainTriangles; j++){

					Vec3f p1 = terrainTriangles[j * 3 + 0];
					Vec3f p2 = terrainTriangles[j * 3 + 1];
					Vec3f p3 = terrainTriangles[j * 3 + 2];

					float east = fmin(fmin(p1.x, p2.x), p3.x);
					float west = fmax(fmax(p1.x, p2.x), p3.x);
					float north = fmin(fmin(p1.z, p2.z), p3.z);
					float south = fmax(fmax(p1.z, p2.z), p3.z);

					if(rayOrigin.x > east && rayOrigin.x < west
					&& rayOrigin.z > north && rayOrigin.z < south){

						Vec3f checkIntersectionPoint;
						bool checkHit = checkLineToTriangleIntersectionVec3f(rayOrigin, getAddVec3f(rayOrigin, rayDirection), p1, p2, p3, &checkIntersectionPoint);

						if(checkHit){
							intersectionPoint = checkIntersectionPoint;
							hit = true;
						}

					}

				}

			}

			//Vec4f pos = getVec4f(mapPos.x, 5.0, mapPos.y, 0.0);
			Vec4f pos = getVec4f(intersectionPoint.x * TERRAIN_SCALE, intersectionPoint.y * TERRAIN_SCALE + 0.5, intersectionPoint.z * TERRAIN_SCALE, 0.0);

			//pos.w
			pos.w = getRandom() * M_PI;

			if(hit){
				grassPositions.push_back(pos);
			}

		}

		//DANGEROUS!!!
		N_GRASS = grassPositions.size();

		TextureBuffer_init(&grassPositionsTextureBuffer, &grassPositions[0], sizeof(Vec4f) * N_GRASS, TYPE_F32);

	}

	Engine_setWindowSize(WIDTH / 2, HEIGHT / 2);

	Engine_centerWindow();

	Engine_setFPSMode(true);

	{
		Model model;

		Model_initFromFile_mesh(&model, "assets/models/quad.mesh");

		String_set(model.name, "teapot", STRING_SIZE);

		models.push_back(model);
	}

	{
		Texture texture;
		Texture_initFromFile(&texture, "assets/textures/blank.png", "blank");
		textures.push_back(texture);
	}

	{
		Texture texture;
		Texture_initFromFile(&texture, "assets/textures/grass.png", "grass");
		textures.push_back(texture);
	}

	//OpenGL stuff
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	glDepthFunc(GL_LESS);
	glDepthMask(GL_TRUE);
	glEnable(GL_DEPTH_TEST);
	glDepthRange(0.0, 1.0);

	glEnable(GL_CULL_FACE);
	glCullFace(GL_FRONT);

	//compile shaders
	{
		unsigned int vertexShader = getCompiledShader("shaders/vertex-shader.glsl", GL_VERTEX_SHADER);
		unsigned int fragmentShader = getCompiledShader("shaders/fragment-shader.glsl", GL_FRAGMENT_SHADER);

		unsigned int shaderProgram = glCreateProgram();
		glAttachShader(shaderProgram, vertexShader);
		glAttachShader(shaderProgram, fragmentShader);
		glLinkProgram(shaderProgram);

		modelShader = shaderProgram;
	}

	/*
	{
		unsigned int vertexShader = getCompiledShader("shaders/shadow-map-vertex-shader.glsl", GL_VERTEX_SHADER);
		unsigned int fragmentShader = getCompiledShader("shaders/shadow-map-fragment-shader.glsl", GL_FRAGMENT_SHADER);

		unsigned int shaderProgram = glCreateProgram();
		glAttachShader(shaderProgram, vertexShader);
		glAttachShader(shaderProgram, fragmentShader);
		glLinkProgram(shaderProgram);

		shadowMapShader = shaderProgram;
	}
	*/
	{
		unsigned int vertexShader = getCompiledShader("shaders/grass-vertex-shader.glsl", GL_VERTEX_SHADER);
		unsigned int fragmentShader = getCompiledShader("shaders/grass-fragment-shader.glsl", GL_FRAGMENT_SHADER);

		unsigned int shaderProgram = glCreateProgram();
		glAttachShader(shaderProgram, vertexShader);
		glAttachShader(shaderProgram, fragmentShader);
		glLinkProgram(shaderProgram);

		grassShader = shaderProgram;
	}

	//generate shadow map depth texture
	Texture_initAsDepthMap(&shadowMapDepthTexture, SHADOW_MAP_WIDTH, SHADOW_MAP_HEIGHT);

	//generate shadow map frame buffer
	glGenFramebuffers(1, &shadowMapFBO);
	glBindFramebuffer(GL_FRAMEBUFFER, shadowMapFBO);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, shadowMapDepthTexture.ID, 0);
	glDrawBuffer(GL_NONE);
	glReadBuffer(GL_NONE);

	//generate static shadow map depth texture
	Texture_initAsDepthMap(&shadowMapDepthTextureStatic, SHADOW_MAP_WIDTH, SHADOW_MAP_HEIGHT);

	//generate static shadow map frame buffer
	glGenFramebuffers(1, &shadowMapStaticFBO);
	glBindFramebuffer(GL_FRAMEBUFFER, shadowMapStaticFBO);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, shadowMapDepthTextureStatic.ID, 0);
	glDrawBuffer(GL_NONE);
	glReadBuffer(GL_NONE);

	Texture_initAsDepthMap(&transparentShadowMapDepthTexture, SHADOW_MAP_WIDTH, SHADOW_MAP_HEIGHT);
	Texture_initAsColorMap(&transparentShadowMapColorTexture, SHADOW_MAP_WIDTH, SHADOW_MAP_HEIGHT);

	//generate transparent shadow map frame buffer
	glGenFramebuffers(1, &transparentShadowMapFBO);
	glBindFramebuffer(GL_FRAMEBUFFER, transparentShadowMapFBO);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, transparentShadowMapDepthTexture.ID, 0);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, transparentShadowMapColorTexture.ID, 0);

	Texture_initAsDepthMap(&transparentShadowMapDepthTextureStatic, SHADOW_MAP_WIDTH, SHADOW_MAP_HEIGHT);
	Texture_initAsColorMap(&transparentShadowMapColorTextureStatic, SHADOW_MAP_WIDTH, SHADOW_MAP_HEIGHT);

	//generate transparent shadow map frame buffer
	glGenFramebuffers(1, &transparentShadowMapStaticFBO);
	glBindFramebuffer(GL_FRAMEBUFFER, transparentShadowMapStaticFBO);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, transparentShadowMapDepthTextureStatic.ID, 0);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, transparentShadowMapColorTextureStatic.ID, 0);

	glBindFramebuffer(GL_FRAMEBUFFER, 0);  

}

void Engine_update(float deltaTime){

	printf("---\n");

	if(Engine_keys[ENGINE_KEY_Q].down){
		Engine_quit();
	}
	if(Engine_keys[ENGINE_KEY_F].downed){
		Engine_toggleFullscreen();
	}

	//handle looking around
	cameraRotation.x += -Engine_pointer.movement.x * PLAYER_LOOK_SPEED;
	cameraRotation.y += -Engine_pointer.movement.y * PLAYER_LOOK_SPEED;

	if(cameraRotation.y > M_PI / 2 - 0.01){
		cameraRotation.y = M_PI / 2 - 0.01;
	}
	if(cameraRotation.y < -(M_PI / 2 - 0.01)){
		cameraRotation.y = -(M_PI / 2 - 0.01);
	}

	//handle camera movement
	Vec2f cameraDirectionXZ = getVec2f(cameraDirection.x, cameraDirection.z);
	Vec2f_normalize(&cameraDirectionXZ);

	if(Engine_keys[ENGINE_KEY_W].down){
		playerPos.x += cameraDirectionXZ.x * PLAYER_SPEED;
		playerPos.z += cameraDirectionXZ.y * PLAYER_SPEED;
	}
	if(Engine_keys[ENGINE_KEY_S].down){
		playerPos.x += -cameraDirectionXZ.x * PLAYER_SPEED;
		playerPos.z += -cameraDirectionXZ.y * PLAYER_SPEED;
	}
	/*
	if(Engine_keys[ENGINE_KEY_SPACE].down){
		cameraPos.y += PLAYER_SPEED;
	}
	if(Engine_keys[ENGINE_KEY_SHIFT].down){
		cameraPos.y += -PLAYER_SPEED;
	}
	*/
	if(Engine_keys[ENGINE_KEY_A].down){
		Vec3f left = getCrossVec3f(cameraDirection, getVec3f(0, 1.0, 0));
		Vec3f_normalize(&left);
		playerPos.x += left.x * PLAYER_SPEED;
		playerPos.z += left.z * PLAYER_SPEED;
	}
	if(Engine_keys[ENGINE_KEY_D].down){
		Vec3f right = getCrossVec3f(getVec3f(0, 1.0, 0), cameraDirection);
		Vec3f_normalize(&right);
		playerPos.x += right.x * PLAYER_SPEED;
		playerPos.z += right.z * PLAYER_SPEED;
	}

	if(Engine_keys[ENGINE_KEY_SPACE].down
	&& playerOnGround){
		playerVelocity.y += PLAYER_JUMP_SPEED;
	}

	if(Engine_keys[ENGINE_KEY_CONTROL].down){
		if(playerHeight > PLAYER_HEIGHT_CROUCHING){
			playerHeight -= PLAYER_HEIGHT_SPEED;
		}
	}else{
		if(playerHeight < PLAYER_HEIGHT_STANDING){
			playerHeight += PLAYER_HEIGHT_SPEED * 1.2;
		}
	}

	playerVelocity.y += -PLAYER_GRAVITY;
	Vec3f_add(&playerPos, playerVelocity);

	playerOnGround = false;

	if(playerPos.y < 0.0){
		playerPos.y = 0.0;
		playerVelocity.y = 0.0;
		playerOnGround = true;
	}

	for(int i = 0; i < TERRAIN_WIDTH * TERRAIN_WIDTH * 2; i++){

		Vec3f t1 = terrainTriangles[i * 3 + 0];
		Vec3f t2 = terrainTriangles[i * 3 + 1];
		Vec3f t3 = terrainTriangles[i * 3 + 2];

		Vec3f_mulByFloat(&t1, TERRAIN_SCALE);
		Vec3f_mulByFloat(&t2, TERRAIN_SCALE);
		Vec3f_mulByFloat(&t3, TERRAIN_SCALE);

		Vec3f intersectionPoint;

		bool hit = checkLineToTriangleIntersectionVec3f(playerPos, getAddVec3f(playerPos, getVec3f(0.0, -1.0, 0.0)), t1, t2, t3, &intersectionPoint);

		if(hit
		&& playerPos.y < intersectionPoint.y){
			playerPos = intersectionPoint;
			playerVelocity.y = 0.0;
			playerOnGround = true;
		}

	}

	cameraPos = playerPos;
	cameraPos.y += playerHeight;

	//set camera direction based on rotation
	cameraDirection.y = sin(cameraRotation.y);
	cameraDirection.x = cos(cameraRotation.x) * cos(cameraRotation.y);
	cameraDirection.z = sin(cameraRotation.x) * cos(cameraRotation.y);
	Vec3f_normalize(&cameraDirection);

}

void Engine_draw(){

	glViewport(0.0, 0.0, Engine_clientWidth, Engine_clientHeight);

	glClearColor(0.5, 0.7, 0.9, 1.0);
	glClearDepth(1.0);

	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	Mat4f perspectiveMatrix = getPerspectiveMat4f(fov, (float)Engine_clientWidth / (float)Engine_clientHeight);

	Mat4f cameraMatrix = getLookAtMat4f(cameraPos, cameraDirection);

	//draw terrain
	{
		Vec3f pos = getVec3f(0.0, 0.0, 0.0);
		float scale = TERRAIN_SCALE;

		Mat4f modelMatrix = getIdentityMat4f();
		
		Mat4f_mulByMat4f(&modelMatrix, getTranslationMat4f(pos.x, pos.y, pos.z));

		Mat4f_mulByMat4f(&modelMatrix, getScalingMat4f(scale));

		unsigned int currentShaderProgram = modelShader;

		glUseProgram(currentShaderProgram);
		
		Model *model_p;

		//model_p = &models[0];
		model_p = &terrainModel;

		glBindBuffer(GL_ARRAY_BUFFER, model_p->VBO);
		glBindVertexArray(model_p->VAO);

		GL3D_uniformMat4f(currentShaderProgram, "modelMatrix", modelMatrix);
		GL3D_uniformMat4f(currentShaderProgram, "perspectiveMatrix", perspectiveMatrix);
		GL3D_uniformMat4f(currentShaderProgram, "cameraMatrix", cameraMatrix);

		glDrawArrays(GL_TRIANGLES, 0, model_p->numberOfTriangles * 3);
	}

	//draw grass
	{
		glDisable(GL_CULL_FACE);
		
		Vec3f pos = getVec3f(0.0, 4.0, 0.0);
		float scale = 1.0;

		unsigned int currentShaderProgram = grassShader;

		glUseProgram(currentShaderProgram);

		Texture *texture_p = &textures[1];

		Model *model_p;

		model_p = &models[0];

		glBindBuffer(GL_ARRAY_BUFFER, model_p->VBO);
		glBindVertexArray(model_p->VAO);

		GL3D_uniformTexture(currentShaderProgram, "colorTexture", 0, texture_p->ID);

		GL3D_uniformTextureBuffer(currentShaderProgram, "grassPositions", 1, grassPositionsTextureBuffer.TB);

		//GL3D_uniformTexture(currentShaderProgram, "heightMap", 1, terrainHeightMapTexture.ID);

		GL3D_uniformMat4f(currentShaderProgram, "perspectiveMatrix", perspectiveMatrix);
		GL3D_uniformMat4f(currentShaderProgram, "cameraMatrix", cameraMatrix);

		windTime += 0.01;

		GL3D_uniformFloat(currentShaderProgram, "windTime", windTime);

		GL3D_uniformFloat(currentShaderProgram, "rotation", 0.0);

		glDrawArraysInstanced(GL_TRIANGLES, 0, model_p->numberOfTriangles * 3, N_GRASS);

		GL3D_uniformFloat(currentShaderProgram, "rotation", M_PI * (1.0 / 3.0));

		glDrawArraysInstanced(GL_TRIANGLES, 0, model_p->numberOfTriangles * 3, N_GRASS);

		GL3D_uniformFloat(currentShaderProgram, "rotation", M_PI * (2.0 / 3.0));

		glDrawArraysInstanced(GL_TRIANGLES, 0, model_p->numberOfTriangles * 3, N_GRASS);

		glEnable(GL_CULL_FACE);
	}

}

void Engine_finnish(){

}
