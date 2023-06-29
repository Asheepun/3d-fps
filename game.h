#include "engine/engine.h"
#include "engine/geometry.h"
#include "engine/3d.h"
#include "engine/strings.h"
#include "engine/files.h"

#include <cstring>
#include <vector>

struct Bullet{
	Vec3f pos;
	Vec3f velocity;
};

struct Obstacle{
	Vec4f color;
	Vec3f pos;
	float scale;
	int modelIndex;
	int triangleMeshIndex;
};

struct Game{
	
	std::vector<Bullet> bullets;
	std::vector<Obstacle> obstacles;

	std::vector<Model> models;
	std::vector<Texture> textures;
	std::vector<TriangleMesh> triangleMeshes;

};

int Game_getModelIndexByName(Game *, const char *);
int Game_getTextureIndexByName(Game *, const char *);
int Game_getTriangleMeshIndexByName(Game *, const char *);

Model *Game_getModelPointerByName(Game *, const char *);
Texture *Game_getTexturePointerByName(Game *, const char *);
TriangleMesh *Game_getTriangleMeshPointerByName(Game *, const char *);
