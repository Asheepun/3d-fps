#ifndef GAME_H_
#define GAME_H_

#include "engine/engine.h"
#include "engine/geometry.h"
#include "engine/3d.h"
#include "engine/strings.h"
#include "engine/files.h"
#include "engine/renderer2d.h"
#include "engine/text.h"

#include "server/connection.h"

#include "pthread.h"
#include <cstring>
#include <vector>

#define RUN_OFFLINE

enum GameState{
	GAME_STATE_LOBBY,
	GAME_STATE_LEVEL,
};

enum RenderStage{
	RENDER_STAGE_BIG_SHADOWS,
	RENDER_STAGE_SHADOWS,
	RENDER_STAGE_GRASS_SHADOWS,
	RENDER_STAGE_SCENE,
	N_RENDER_STAGES,
};

enum Weapon{
	WEAPON_GUN,
	WEAPON_SWORD,
	N_WEAPONS,
};

struct Box{
	Vec3f pos;
	Vec3f size;
};

struct RigidBody{
	Box boundingBox;
	Vec3f pos;
	Vec3f lastPos;
	Vec3f velocity;
	Vec4f orientation;
	Vec4f lastOrientation;
	Vec3f angularVelocity;
	int modelIndex;
	int textureIndex;
	int triangleMeshIndex;
	int framesSinceHit;
	Vec3f lastTorque;
};

struct Collision{
	int index1;
	int index2;
	Vec3f normal;
	Vec3f pos;
};

struct Bullet{
	Vec3f pos;
	Vec3f velocity;
};

struct Particle{
	Vec3f pos;
	Vec3f velocity;
	Vec4f orientation;
	Vec4f color;
	Vec3f scale;
	float textureY;
	float textureSizeY;
	int frames;
	int spriteID;
};

struct Obstacle{
	int ID;
	Box boundingBox;
	Vec4f color;
	Vec3f pos;
	float scale;
	int modelIndex;
	int textureIndex;
	int alphaTextureIndex;
	int triangleMeshIndex;
	int spriteID;
};

struct Player{
	Vec3f pos;
	Vec3f lastPos;
	Vec3f velocity;
	Vec3f direction;
	enum Weapon weapon;
	float height;
	bool onGround;
	int health;
	float walkAngle;
	float walkAngleSpeed;
	int connectionID;
};

struct Tree{
	float scale;
	TextureBuffer leafTransformationsTextureBuffer;
	std::vector<Mat4f> leafTransformations;
	Mat4f *sortedLeafTransformations;
};

struct World{
	std::vector<Player> players;
	std::vector<Obstacle> obstacles;
	std::vector<TriangleMesh> triangleMeshes;
	std::vector<BoneTriangleMesh> boneTriangleMeshes;
	std::vector<BoneRig> boneRigs;
};

struct Game{

	GameState currentState;

	World world;
	
	std::vector<Bullet> bullets;
	std::vector<Particle> bloodParticles;
	std::vector<Particle> grassParticles;
	std::vector<RigidBody> rigidBodies;
	std::vector<Tree> trees;

	std::vector<Model> models;
	std::vector<BoneModel> boneModels;
	std::vector<Texture> textures;
	std::vector<PointMesh> pointMeshes;
	std::vector<Shader> shaders;

	unsigned int shadowMapFBO;
	Texture shadowMapDepthTexture;

	unsigned int grassShadowMapFBO;
	Texture grassShadowMapDepthTexture;

	unsigned int bigShadowMapFBO;
	Texture bigShadowMapDepthTexture;

	Texture paintMapTexture;
	unsigned char *paintMap;

	unsigned char *terrainTextureData;
	float terrainMaxHeight;

	TextureBuffer grassPositionsTextureBuffer;
	std::vector<Vec4f> grassPositions;
	Vec4f *sortedGrassPositions;

	std::vector<Box> boundingBoxes;
	std::vector<bool> boundingBoxesCulled;

	Renderer2D_Renderer renderer2D;
	Font font;

	Client client;

	bool startLevel;
	bool dead;

};

//GLOBAL VARIABLES

static int WIDTH = 1920;
static int HEIGHT = 1080;

static int SHADOW_MAP_WIDTH = 500;
static int SHADOW_MAP_HEIGHT = 500;
static float shadowMapScale = 10.0;

static int GRASS_SHADOW_MAP_WIDTH = 500;
static int GRASS_SHADOW_MAP_HEIGHT = 500;

static int BIG_SHADOW_MAP_WIDTH = 500;
static int BIG_SHADOW_MAP_HEIGHT = 500;
static float bigShadowMapScale = 70.0;

static int PAINT_MAP_WIDTH = 300;
static int PAINT_MAP_HEIGHT = 300;
static float PAINT_MAP_SCALE = 100.0;

static int TERRAIN_TEXTURE_WIDTH = 1000;

//static int TERRAIN_WIDTH = 20;
//static float TERRAIN_SCALE = 100.0;
static int TERRAIN_WIDTH = 20;
static float TERRAIN_SCALE = 100.0;
static Vec4f TERRAIN_COLOR = { 0.14, 0.55, 0.17, 1.0 };

static const int GRASS_GRID_WIDTH = 10;
static const float GRASS_DISTANCE = 1.1;
static const float GRASS_MARGIN = 3.0;
static const float GRASS_SHADOW_STRENGTH = 0.3;

static const float TREE_SCALE = 1.5;
static float TREE_TERRAIN_MARGIN = 5.0;
static float TREE_MIN_DISTANCE = 7.0;

static float BULLET_SCALE = 0.2;
static float BULLET_SPEED = 1.0;
static Vec4f BULLET_COLOR = { 0.5, 0.5, 0.4, 1.0 };

static float BLOOD_PARTICLE_GRAVITY = 0.01;

static float PLAYER_HEIGHT_STANDING = 2.0;
static float PLAYER_HEIGHT_CROUCHING = 1.0;
static float PLAYER_HEIGHT_SPEED = 0.1;
static float PLAYER_GRAVITY = 0.005;
//static float PLAYER_JUMP_SPEED = 0.2;
static float PLAYER_JUMP_SPEED = 0.15;
static float PLAYER_WALK_RESISTANCE = 0.75;
static float PLAYER_LOOK_SPEED = 0.0015;
static float PLAYER_SPEED = 0.065;
static float PLAYER_CROUCH_SPEED = 0.025;
static float PLAYER_SCALE = 0.6;

static float GUN_SCALE = 0.4;

//FUNCTION_DEFINITIONS

//FILE: world.cpp

bool Player_World_shoot_common(Player *, World *, std::vector<Player>, Vec3f *, Vec3f *, int *);
void Player_World_moveAndCollideBasedOnInputs_common(Player *, World *, Inputs);

std::vector<Mat4f> World_Player_getBoneTransformations(World *, Player *);

//void World_updatePlayersAndObstaclesCommon(World *);

void World_addPlayer(World *, Vec3f, int);
int World_addObstacle(World *, Vec3f, float, int, int, int, Vec4f);

int World_getPlayerIndexByConnectionID(World *, int);
Player *World_getPlayerPointerByConnectionID(World *, int);

void World_clear(World *);

void generateTerrainTriangles(Vec3f **, Vec2f **, int *);

Mat4f getSwordMatrix(Vec3f, Vec3f, float);

Mat4f getModelMatrix(Vec3f, Vec3f, Vec4f);

bool checkBoxBoxCollision(Box, Box);

int Game_getModelIndexByName(Game *, const char *);
int Game_getTextureIndexByName(Game *, const char *);
int World_getTriangleMeshIndexByName(World *, const char *);
int World_getBoneTriangleMeshIndexByName(World *, const char *);
int Game_getShaderIndexByName(Game *, const char *);
int Game_getBoneModelIndexByName(Game *, const char *);
int World_getBoneRigIndexByName(World *, const char *);

Model *Game_getModelPointerByName(Game *, const char *);
Texture *Game_getTexturePointerByName(Game *, const char *);
TriangleMesh *World_getTriangleMeshPointerByName(World *, const char *);
BoneTriangleMesh *World_getBoneTriangleMeshPointerByName(World *, const char *);
Shader *Game_getShaderPointerByName(Game *, const char *);
BoneModel *Game_getBoneModelPointerByName(Game *, const char *);
BoneRig *World_getBoneRigPointerByName(World *, const char *);

Obstacle *Game_getObstacleByID(Game *, int);

//FILE: client.cpp

void Client_init(Client *);

void Client_sendInputsToServer(Client *, Inputs);
void Client_sendReadyToServer(Client *, bool);

//void Game_initClient(Game *);
//void Game_sendInputsToServer(Game *, Inputs);
//void *receiveServerMessages(void *);

//FILE: assets.cpp

void Game_loadAssets(Game *);
void World_loadAssets(World *, const char *);

void *loadAssetsAndGenerateStuffThreaded(void *);

//FILE: trees.cpp

//void Game_addTree(Game *, Vec3f);
void generateTree(Vec3f, Tree *, Model *, TriangleMesh *);

//FILE: levelState.cpp
void levelState(Game *);

void drawLevelState(Game *);

//FILE: lobbyState.cpp
void lobbyState(Game *);

void drawLobbyState(Game *);

#endif
