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

Game game;

float BULLET_SCALE = 0.2;
float BULLET_SPEED = 1.0;
Vec4f BULLET_COLOR = { 0.5, 0.5, 0.4, 1.0 };

float PLAYER_HEIGHT_STANDING = 2.0;
float PLAYER_HEIGHT_CROUCHING = 1.0;
float PLAYER_HEIGHT_SPEED = 0.1;
float PLAYER_GRAVITY = 0.005;
float PLAYER_JUMP_SPEED = 0.2;
float PLAYER_WALK_RESISTANCE = 0.75;
float PLAYER_LOOK_SPEED = 0.0015;
float PLAYER_SPEED = 0.075;

Vec3f playerPos = { 5.0, 3.0, 5.0 };
Vec3f lastPlayerPos = playerPos;
Vec3f playerVelocity = { 0.0, 0.0, 0.0 };
bool playerOnGround = false;
float playerHeight = PLAYER_HEIGHT_STANDING;

float windTime = 0.0;

int TERRAIN_WIDTH = 20;
float TERRAIN_SCALE = 100.0;
Vec4f TERRAIN_COLOR = { 0.14, 0.55, 0.17, 1.0 };
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

Vec3f cameraPos = getVec3f(4.0, 5.0, 0.0);
Vec3f cameraDirection = getVec3f(0.0, 0.0, 1.0);
Vec2f cameraRotation = getVec2f(M_PI / 2.0, 0.0);

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

		Vec3f *terrainTriangles = (Vec3f *)malloc(sizeof(Vec3f) * 3 * 2 * (TERRAIN_WIDTH - 1) * (TERRAIN_WIDTH - 1));

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

		//create terrain model
		Model model;

		Model_initFromMeshData(&model, (unsigned char *)terrainMesh, n_terrainTriangles);

		String_set(model.name, "terrain", STRING_SIZE);

		game.models.push_back(model);

		//create terrain triangle mesh
		TriangleMesh triangleMesh;
		triangleMesh.triangles = terrainTriangles;
		triangleMesh.n_triangles = n_terrainTriangles;
		String_set(triangleMesh.name, "terrain", STRING_SIZE);

		game.triangleMeshes.push_back(triangleMesh);

		//free unneeded terrain data
		free(terrainPoints);
		free(terrainMesh);

	}

	//generate grass positions
	{
		TriangleMesh *terrainTriangleMesh_p = Game_getTriangleMeshPointerByName(&game, "terrain");

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

				for(int j = 0; j < terrainTriangleMesh_p->n_triangles; j++){

					Vec3f p1 = terrainTriangleMesh_p->triangles[j * 3 + 0];
					Vec3f p2 = terrainTriangleMesh_p->triangles[j * 3 + 1];
					Vec3f p3 = terrainTriangleMesh_p->triangles[j * 3 + 2];

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

		String_set(model.name, "quad", STRING_SIZE);

		game.models.push_back(model);
	}
	{
		Model model;

		Model_initFromFile_mesh(&model, "assets/models/cube.mesh");

		String_set(model.name, "cube", STRING_SIZE);

		game.models.push_back(model);
	}

	{
		Texture texture;
		Texture_initFromFile(&texture, "assets/textures/blank.png", "blank");
		game.textures.push_back(texture);
	}

	{
		Texture texture;
		Texture_initFromFile(&texture, "assets/textures/grass.png", "grass");
		game.textures.push_back(texture);
	}
	{
		TriangleMesh triangleMesh;

		TriangleMesh_initFromFile_mesh(&triangleMesh, "assets/models/cube.mesh");

		String_set(triangleMesh.name, "cube", STRING_SIZE);

		game.triangleMeshes.push_back(triangleMesh);
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

	//init game
	{
		Obstacle obstacle;	
		obstacle.pos = getVec3f(30.0, 2.0, 20.0);
		obstacle.scale = 2.0;
		obstacle.modelIndex = Game_getModelIndexByName(&game, "cube");
		obstacle.triangleMeshIndex = Game_getTriangleMeshIndexByName(&game, "cube");
		obstacle.color = getVec4f(0.7, 0.7, 0.7, 1.0);

		game.obstacles.push_back(obstacle);
	}
	{
		Obstacle obstacle;	
		obstacle.pos = getVec3f(0.0, 0.0, 0.0);
		obstacle.scale = TERRAIN_SCALE;
		obstacle.modelIndex = Game_getModelIndexByName(&game, "terrain");
		obstacle.triangleMeshIndex = Game_getTriangleMeshIndexByName(&game, "terrain");
		obstacle.color = TERRAIN_COLOR;

		game.obstacles.push_back(obstacle);
	}

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
		playerVelocity.x += cameraDirectionXZ.x * PLAYER_SPEED;
		playerVelocity.z += cameraDirectionXZ.y * PLAYER_SPEED;
	}
	if(Engine_keys[ENGINE_KEY_S].down){
		playerVelocity.x += -cameraDirectionXZ.x * PLAYER_SPEED;
		playerVelocity.z += -cameraDirectionXZ.y * PLAYER_SPEED;
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
		playerVelocity.x += left.x * PLAYER_SPEED;
		playerVelocity.z += left.z * PLAYER_SPEED;
	}
	if(Engine_keys[ENGINE_KEY_D].down){
		Vec3f right = getCrossVec3f(getVec3f(0, 1.0, 0), cameraDirection);
		Vec3f_normalize(&right);
		playerVelocity.x += right.x * PLAYER_SPEED;
		playerVelocity.z += right.z * PLAYER_SPEED;
	}

	if(Engine_keys[ENGINE_KEY_SPACE].down
	&& playerOnGround){
		playerVelocity.y += PLAYER_JUMP_SPEED;
	}

	if(!Engine_keys[ENGINE_KEY_SPACE].down
	&& playerVelocity.y > 0.0){
		playerVelocity.y *= 0.9;
	}

	playerVelocity.x *= PLAYER_WALK_RESISTANCE;
	playerVelocity.z *= PLAYER_WALK_RESISTANCE;

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

	lastPlayerPos = playerPos;
	Vec3f_add(&playerPos, playerVelocity);

	playerOnGround = false;

	if(playerPos.y < 0.0){
		playerPos.y = 0.0;
		playerVelocity.y = 0.0;
		playerOnGround = true;
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
			Vec3f playerFeetPos = playerPos;
			playerFeetPos.y += r;

			if(getDotVec3f(up, N) > 0.7){

				Vec3f intersectionPoint;
				bool hit = checkLineToTriangleIntersectionVec3f(playerPos, getAddVec3f(playerPos, up), triangle1, triangle2, triangle3, &intersectionPoint);

				if(hit
				&& intersectionPoint.y > playerPos.y
				&& intersectionPoint.y < lastPlayerPos.y + r){
					playerPos.y = intersectionPoint.y;
					playerVelocity.y = 0.0;
					playerOnGround = true;
				}

			}else{

				bool hit = checkSphereToTriangleCollision(playerFeetPos, r, triangle1, triangle2, triangle3);

				if(hit){

					float distance = r - fabs((getDotVec3f(playerFeetPos, N) - getDotVec3f(triangle1, N)) / getDotVec3f(N, N));

					Vec3f_add(&playerPos, getMulVec3fFloat(N, distance));
					Vec3f_add(&playerVelocity, getMulVec3fFloat(N, distance));
					//Vec3f_add(&playerVelocity, N);
				}
			
			}

			/*
			float r = 0.2;
			Vec3f playerFeetPos = playerPos;
			playerFeetPos.y += r;

			bool hit = checkSphereToTriangleCollision(playerFeetPos, r, t1, t2, t3);
			*/

		}

	}

	cameraPos = playerPos;
	cameraPos.y += playerHeight;

	//set camera direction based on rotation
	cameraDirection.y = sin(cameraRotation.y);
	cameraDirection.x = cos(cameraRotation.x) * cos(cameraRotation.y);
	cameraDirection.z = sin(cameraRotation.x) * cos(cameraRotation.y);
	Vec3f_normalize(&cameraDirection);

	//handle shooting
	if(Engine_pointer.downed){
		
		Bullet bullet;
		bullet.pos = cameraPos;
		bullet.pos.y -= 0.5;

		Vec3f target = cameraPos;
		Vec3f_add(&target, getMulVec3fFloat(cameraDirection, 10));

		bullet.velocity = getSubVec3f(target, bullet.pos);
		Vec3f_normalize(&bullet.velocity);
		Vec3f_mulByFloat(&bullet.velocity, BULLET_SPEED);

		game.bullets.push_back(bullet);

	}

	//update bullets
	for(int i = 0; i < game.bullets.size(); i++){
		
		Bullet *bullet_p = &game.bullets[i];

		//move bullets
		Vec3f_add(&bullet_p->pos, bullet_p->velocity);

		//remove oub bullets
		if(bullet_p->pos.x < 0.0
		|| bullet_p->pos.y < 0.0
		|| bullet_p->pos.z < 0.0
		|| bullet_p->pos.x > TERRAIN_SCALE
		|| bullet_p->pos.y > TERRAIN_SCALE
		|| bullet_p->pos.z > TERRAIN_SCALE){
			game.bullets.erase(game.bullets.begin() + i);
			i--;
			continue;
		}

	}

}

void Engine_draw(){

	glViewport(0.0, 0.0, Engine_clientWidth, Engine_clientHeight);

	glClearColor(0.5, 0.7, 0.9, 1.0);
	glClearDepth(1.0);

	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	Mat4f perspectiveMatrix = getPerspectiveMat4f(fov, (float)Engine_clientWidth / (float)Engine_clientHeight);

	Mat4f cameraMatrix = getLookAtMat4f(cameraPos, cameraDirection);

	/*
	//draw terrain
	{
		Vec3f pos = getVec3f(0.0, 0.0, 0.0);
		float scale = TERRAIN_SCALE;

		Mat4f modelMatrix = getIdentityMat4f();
		
		Mat4f_mulByMat4f(&modelMatrix, getTranslationMat4f(pos.x, pos.y, pos.z));

		Mat4f_mulByMat4f(&modelMatrix, getScalingMat4f(scale));

		unsigned int currentShaderProgram = modelShader;

		glUseProgram(currentShaderProgram);
		
		Model *model_p = Game_getModelPointerByName(&game, "terrain");

		//model_p = &models[0];
		//model_p = &terrainModel;

		glBindBuffer(GL_ARRAY_BUFFER, model_p->VBO);
		glBindVertexArray(model_p->VAO);

		GL3D_uniformMat4f(currentShaderProgram, "modelMatrix", modelMatrix);
		GL3D_uniformMat4f(currentShaderProgram, "perspectiveMatrix", perspectiveMatrix);
		GL3D_uniformMat4f(currentShaderProgram, "cameraMatrix", cameraMatrix);

		GL3D_uniformVec4f(currentShaderProgram, "inputColor", TERRAIN_COLOR);

		glDrawArrays(GL_TRIANGLES, 0, model_p->numberOfTriangles * 3);
	}
	*/

	//draw grass
	{
		glDisable(GL_CULL_FACE);
		
		Vec3f pos = getVec3f(0.0, 4.0, 0.0);
		float scale = 1.0;

		unsigned int currentShaderProgram = grassShader;

		glUseProgram(currentShaderProgram);

		Texture *texture_p = Game_getTexturePointerByName(&game, "grass");

		Model *model_p = Game_getModelPointerByName(&game, "quad");

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

	//draw bullets
	for(int i = 0; i < game.bullets.size(); i++){

		Bullet *bullet_p = &game.bullets[i];

		float scale = BULLET_SCALE;

		Mat4f modelMatrix = getIdentityMat4f();
		
		Mat4f_mulByMat4f(&modelMatrix, getTranslationMat4f(bullet_p->pos.x, bullet_p->pos.y, bullet_p->pos.z));

		Mat4f_mulByMat4f(&modelMatrix, getScalingMat4f(scale));

		unsigned int currentShaderProgram = modelShader;

		glUseProgram(currentShaderProgram);
		
		Model *model_p = Game_getModelPointerByName(&game, "cube");

		glBindBuffer(GL_ARRAY_BUFFER, model_p->VBO);
		glBindVertexArray(model_p->VAO);

		GL3D_uniformMat4f(currentShaderProgram, "modelMatrix", modelMatrix);
		GL3D_uniformMat4f(currentShaderProgram, "perspectiveMatrix", perspectiveMatrix);
		GL3D_uniformMat4f(currentShaderProgram, "cameraMatrix", cameraMatrix);

		GL3D_uniformVec4f(currentShaderProgram, "inputColor", BULLET_COLOR);

		glDrawArrays(GL_TRIANGLES, 0, model_p->numberOfTriangles * 3);

	}

	//draw obstacles
	for(int i = 0; i < game.obstacles.size(); i++){

		Obstacle *obstacle_p = &game.obstacles[i];

		float scale = obstacle_p->scale;

		Mat4f modelMatrix = getIdentityMat4f();
		
		Mat4f_mulByMat4f(&modelMatrix, getTranslationMat4f(obstacle_p->pos.x, obstacle_p->pos.y, obstacle_p->pos.z));

		Mat4f_mulByMat4f(&modelMatrix, getScalingMat4f(scale));

		unsigned int currentShaderProgram = modelShader;

		glUseProgram(currentShaderProgram);
		
		Model *model_p = &game.models[obstacle_p->modelIndex];

		glBindBuffer(GL_ARRAY_BUFFER, model_p->VBO);
		glBindVertexArray(model_p->VAO);

		GL3D_uniformMat4f(currentShaderProgram, "modelMatrix", modelMatrix);
		GL3D_uniformMat4f(currentShaderProgram, "perspectiveMatrix", perspectiveMatrix);
		GL3D_uniformMat4f(currentShaderProgram, "cameraMatrix", cameraMatrix);

		GL3D_uniformVec4f(currentShaderProgram, "inputColor", obstacle_p->color);

		glDrawArrays(GL_TRIANGLES, 0, model_p->numberOfTriangles * 3);

		
	}

}

void Engine_finnish(){

}
