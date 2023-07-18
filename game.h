#ifndef GAME_H_
#define GAME_H_

#include "engine/engine.h"
#include "engine/geometry.h"
#include "engine/3d.h"
#include "engine/strings.h"
#include "engine/files.h"

#include "server/connection.h"

#include "pthread.h"
#include <cstring>
#include <vector>

struct Bullet{
	Vec3f pos;
	Vec3f velocity;
};

struct Particle{
	Vec3f pos;
	Vec3f velocity;
	Vec3f rotation;
	int frames;
};

struct Obstacle{
	Vec4f color;
	Vec3f pos;
	float scale;
	int modelIndex;
	int triangleMeshIndex;
};

struct Player{
	Vec3f pos;
	Vec3f lastPos;
	Vec3f velocity;
	Vec3f direction;
	float height;
	bool onGround;
	int connectionID;
};

struct Game{

	Player player;

	std::vector<Player> otherPlayers;
	
	std::vector<Bullet> bullets;
	std::vector<Obstacle> obstacles;
	std::vector<Particle> particles;

	std::vector<Model> models;
	std::vector<BoneModel> boneModels;
	std::vector<Texture> textures;
	std::vector<TriangleMesh> triangleMeshes;

	ServerGameState latestServerGameState_mutexed;
	pthread_mutex_t serverStateMutex;
	int n_sentInputs;
	std::vector<Inputs> inputsBuffer;

	int connectionID;

};

//GLOBAL VARIABLES

static int TERRAIN_WIDTH = 20;
static float TERRAIN_SCALE = 100.0;
static Vec4f TERRAIN_COLOR = { 0.14, 0.55, 0.17, 1.0 };

static float BULLET_SCALE = 0.2;
static float BULLET_SPEED = 1.0;
static Vec4f BULLET_COLOR = { 0.5, 0.5, 0.4, 1.0 };

static float PLAYER_HEIGHT_STANDING = 2.0;
static float PLAYER_HEIGHT_CROUCHING = 1.0;
static float PLAYER_HEIGHT_SPEED = 0.1;
static float PLAYER_GRAVITY = 0.005;
static float PLAYER_JUMP_SPEED = 0.2;
static float PLAYER_WALK_RESISTANCE = 0.75;
static float PLAYER_LOOK_SPEED = 0.0015;
static float PLAYER_SPEED = 0.075;

//FUNCTION_DEFINITIONS

void Game_simulateInputsOnClient(Game *, Inputs);

TriangleMesh generateTerrainTriangleMesh(int, float);

int Game_getModelIndexByName(Game *, const char *);
int Game_getTextureIndexByName(Game *, const char *);
int Game_getTriangleMeshIndexByName(Game *, const char *);

Model *Game_getModelPointerByName(Game *, const char *);
Texture *Game_getTexturePointerByName(Game *, const char *);
TriangleMesh *Game_getTriangleMeshPointerByName(Game *, const char *);

void Game_initClient(Game *);
void Game_sendInputsToServer(Game *, Inputs);
void *receiveServerMessages(void *);

#endif
