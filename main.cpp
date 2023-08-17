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

float gameTime = 0.0;

Game game;

//Vec3f game.player.pos = { 5.0, 3.0, 5.0 };
//Vec3f lastPlayerPos = game.player.pos;
//Vec3f game.player.velocity = { 0.0, 0.0, 0.0 };
//bool playerOnGround = false;
//float game.player.height = PLAYER_HEIGHT_STANDING;

float windTime = 0.0;

//THIS VALUE (N_GRASS) WILL BE MODIFIED DANGER DANGER!!!
int N_GRASS = 10000;
TextureBuffer grassPositionsTextureBuffer;

int WIDTH = 1920;
int HEIGHT = 1080;

unsigned int modelShader;
unsigned int boneModelShader;
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

#ifndef RUN_OFFLINE
	Game_initClient(&game);
#endif

	//generate terrain
	{
		setRandomSeed(1);
		TriangleMesh triangleMesh = generateTerrainTriangleMesh(TERRAIN_WIDTH, 1.0 / (TERRAIN_SCALE / 2.0));
		game.triangleMeshes.push_back(triangleMesh);

		unsigned char *meshData = generateMeshDataFromTriangleMesh(triangleMesh);

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
		BoneModel model;

		BoneModel_initFromFile(&model, "assets/models/dude-bones.bonemesh", "assets/models/dude-bones.bones");

		String_set(model.name, "dude", STRING_SIZE);

		game.boneModels.push_back(model);
	}
	{
		BoneModel model;

		BoneModel_initFromFile(&model, "assets/models/dude-bones.bonemesh", "assets/models/dude-bones2.bones");

		String_set(model.name, "dude", STRING_SIZE);

		game.boneModels.push_back(model);
	}

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
	{
		unsigned int vertexShader = getCompiledShader("shaders/bone-model-vertex-shader.glsl", GL_VERTEX_SHADER);
		unsigned int fragmentShader = getCompiledShader("shaders/bone-model-fragment-shader.glsl", GL_FRAGMENT_SHADER);

		unsigned int shaderProgram = glCreateProgram();
		glAttachShader(shaderProgram, vertexShader);
		glAttachShader(shaderProgram, fragmentShader);
		glLinkProgram(shaderProgram);

		boneModelShader = shaderProgram;
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
	pthread_mutex_init(&game.serverStateMutex, NULL);

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

	glViewport(0.0, 0.0, Engine_clientWidth, Engine_clientHeight);

	glClearColor(0.5, 0.7, 0.9, 1.0);
	glClearDepth(1.0);

	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	Mat4f perspectiveMatrix = getPerspectiveMat4f(fov, (float)Engine_clientWidth / (float)Engine_clientHeight);

	Mat4f cameraMatrix = getLookAtMat4f(cameraPos, cameraDirection);

	/*
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
	*/

	//draw particles
	glDisable(GL_CULL_FACE);

	for(int i = 0; i < game.particles.size(); i++){

		Particle *particle_p = &game.particles[i];

		float scale = 0.05;

		Mat4f modelMatrix = getIdentityMat4f();

		//modelMatrix *= getRotationMat4f(particle_p->rotation.x, particle_p->rotation.y, particle_p->rotation.z);

		modelMatrix *= getScalingMat4f(scale);
		
		modelMatrix *= getTranslationMat4f(particle_p->pos);

		unsigned int currentShaderProgram = modelShader;

		glUseProgram(currentShaderProgram);
		
		Model *model_p = Game_getModelPointerByName(&game, "quad");

		glBindBuffer(GL_ARRAY_BUFFER, model_p->VBO);
		glBindVertexArray(model_p->VAO);

		GL3D_uniformMat4f(currentShaderProgram, "modelMatrix", modelMatrix);
		GL3D_uniformMat4f(currentShaderProgram, "perspectiveMatrix", perspectiveMatrix);
		GL3D_uniformMat4f(currentShaderProgram, "cameraMatrix", cameraMatrix);

		GL3D_uniformVec4f(currentShaderProgram, "inputColor", getVec4f(1.0, 0.7, 0.0, 1.0));

		glDrawArrays(GL_TRIANGLES, 0, model_p->numberOfTriangles * 3);

	}

	glEnable(GL_CULL_FACE);

	//draw obstacles
	for(int i = 0; i < game.obstacles.size(); i++){

		Obstacle *obstacle_p = &game.obstacles[i];

		float scale = obstacle_p->scale;

		Mat4f modelMatrix = getIdentityMat4f();

		modelMatrix *= getScalingMat4f(scale);

		modelMatrix *= getTranslationMat4f(obstacle_p->pos);

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

	//draw other players
	for(int i = 0; i < game.otherPlayers.size(); i++){

		Player *player_p = &game.otherPlayers[i];

		float scale = 1.0;

		Mat4f modelMatrix = getIdentityMat4f();

		modelMatrix *= getScalingMat4f(scale);

		modelMatrix *= getTranslationMat4f(player_p->pos);

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

	std::vector<Bone> bones0 = game.boneModels[0].bones;
	std::vector<Bone> bones1 = game.boneModels[1].bones;

	std::vector<Bone> interpolatedBones;

	for(int i = 0; i < bones0.size(); i++){

		Bone interpolatedBone = bones0[i];

		float t = (1.0 + sin(gameTime * 0.1)) * 0.5;

		interpolatedBone.rotation = lerp(bones0[i].rotation, bones1[i].rotation, t);
		interpolatedBone.scale = lerp(bones0[i].scale, bones1[i].scale, t);
		interpolatedBone.translation = lerp(bones0[i].translation, bones1[i].translation, t);

		interpolatedBones.push_back(interpolatedBone);

	}

	std::vector<Mat4f> bindMatrices = getBindMatricesFromBones(interpolatedBones);

	std::vector<Mat4f> boneTransformations;

	for(int i = 0; i < bindMatrices.size(); i++){
		Mat4f transformation = game.boneModels[0].inverseBindMatrices[i];
		transformation *= bindMatrices[i];
		boneTransformations.push_back(transformation);
	}

	//draw bone model
	{

		Vec3f pos = getVec3f(10.0, 3.0, 10.0);
		float scale = 0.5;

		Mat4f modelMatrix = getIdentityMat4f();

		modelMatrix *= getScalingMat4f(scale);

		modelMatrix *= getTranslationMat4f(pos);

		unsigned int currentShaderProgram = boneModelShader;

		glUseProgram(currentShaderProgram);

		BoneModel *model_p = &game.boneModels[0];

		glBindBuffer(GL_ARRAY_BUFFER, model_p->VBO);
		glBindVertexArray(model_p->VAO);

		GL3D_uniformMat4f(currentShaderProgram, "modelMatrix", modelMatrix);
		GL3D_uniformMat4f(currentShaderProgram, "perspectiveMatrix", perspectiveMatrix);
		GL3D_uniformMat4f(currentShaderProgram, "cameraMatrix", cameraMatrix);

		GL3D_uniformVec4f(currentShaderProgram, "inputColor", getVec4f(0.7, 0.7, 0.7, 1.0));

		GL3D_uniformMat4fArray(currentShaderProgram, "boneTransformations", &boneTransformations[0], boneTransformations.size());
		//GL3D_uniformMat4fArray(currentShaderProgram, "inverseBoneTransformations", &model_p->inverseBoneTransformations[0], model_p->inverseBoneTransformations.size());

		glDrawArrays(GL_TRIANGLES, 0, model_p->n_triangles * 3);

	}

	for(int i = 0; i < game.boneModels[0].bones.size(); i++){
		//printf("bone: %i\n", i);

		Vec3f pos = getVec3f(5.0, 3.0, 10.0);

		float scale = 0.1;

		Mat4f modelMatrix = getIdentityMat4f();

		modelMatrix *= getTranslationMat4f(0.0, 1.0, 0.0);

		modelMatrix *= getScalingMat4f(0.05, 0.1, 0.05);
		
		modelMatrix *= bindMatrices[i];

		modelMatrix *= getTranslationMat4f(pos);

		//Mat4f_mulByMat4f(&modelMatrix, getScalingMat4f(scale));

		unsigned int currentShaderProgram = modelShader;

		glUseProgram(currentShaderProgram);
		
		Model *model_p = Game_getModelPointerByName(&game, "cube");

		glBindBuffer(GL_ARRAY_BUFFER, model_p->VBO);
		glBindVertexArray(model_p->VAO);

		GL3D_uniformMat4f(currentShaderProgram, "modelMatrix", modelMatrix);
		GL3D_uniformMat4f(currentShaderProgram, "perspectiveMatrix", perspectiveMatrix);
		GL3D_uniformMat4f(currentShaderProgram, "cameraMatrix", cameraMatrix);

		//if(i == index1){
			//GL3D_uniformVec4f(currentShaderProgram, "inputColor", getVec4f(0.0, 1.0, 0.0, 1.0));
		//}else{
		GL3D_uniformVec4f(currentShaderProgram, "inputColor", getVec4f(1.0, 0.0, 0.0, 1.0));
		//}

		glDrawArrays(GL_TRIANGLES, 0, model_p->numberOfTriangles * 3);

	}

	gameTime += 0.1;

}

void Engine_finnish(){

}
