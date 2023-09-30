#include "game.h"
#include "math.h"

#include "stdio.h"

int CURRENT_AVAILABLE_ID = 0;

//ENTITY FUNCTIONS

int Game_addObstacle(Game *game_p, Vec3f pos, float scale, const char *modelName, const char *textureName, Vec4f color){

	Obstacle obstacle;	

	obstacle.ID = CURRENT_AVAILABLE_ID;
	CURRENT_AVAILABLE_ID++;

	obstacle.pos = pos;
	obstacle.scale = scale;

	obstacle.triangleMeshIndex = Game_getTriangleMeshIndexByName(game_p, modelName);
	obstacle.modelIndex = Game_getModelIndexByName(game_p, modelName);
	obstacle.textureIndex = Game_getTextureIndexByName(game_p, textureName);
	obstacle.color = color;

	game_p->obstacles.push_back(obstacle);

	return obstacle.ID;

}

//MISC FUNCTIONS
Mat4f getSwordMatrix(Vec3f cameraPos, Vec3f cameraDirection, float swingAngle){

	Vec3f targetPos = cameraPos + cameraDirection * 5;
	Vec3f handPos = cameraPos + getVec3f(0.0, -0.7, 0.0);

	Vec3f direction = normalize(targetPos - handPos);

	Vec4f swingOrientation = getQuaternion(getVec3f(0.0, 1.0, 0.0), swingAngle);

	direction = mulVec3fMat4f(direction, getQuaternionMat4f(swingOrientation), 1.0);

	Vec3f pos = handPos + direction * 2.3;
	Vec3f scale = getVec3f(0.15, 1.3, 1.0);

	float horizontalAngle = atan2(direction.x, direction.z);
	float verticalAngle = acos(direction.y);

	Vec4f orientation = IDENTITY_QUATERNION;

	orientation = mulQuaternions(getQuaternion(getVec3f(1.0, 0.0, 0.0), verticalAngle), orientation);
	orientation = mulQuaternions(getQuaternion(getVec3f(0.0, 1.0, 0.0), horizontalAngle), orientation);

	return getModelMatrix(pos, scale, orientation);

}

Mat4f getModelMatrix(Vec3f pos, Vec3f scale, Vec4f orientation){
	return getTranslationMat4f(pos) * getQuaternionMat4f(orientation) * getScalingMat4f(scale);
}

bool checkBoxBoxCollision(Box b1, Box b2){
	return b1.pos.x + b1.size.x >= b2.pos.x - 0.01
		&& b1.pos.x <= b2.pos.x + b2.size.x + 0.01
		&& b1.pos.y + b1.size.y >= b2.pos.y - 0.01
		&& b1.pos.y <= b2.pos.y + b2.size.y + 0.01
		&& b1.pos.z + b1.size.z >= b2.pos.z - 0.01
		&& b1.pos.z <= b2.pos.z + b2.size.z + 0.01;
}

//LOOKUP FUNCTIONS

int Game_getModelIndexByName(Game *game_p, const char *name){

	for(int i = 0; i < game_p->models.size(); i++){
		if(strcmp(game_p->models[i].name, name) == 0){
			return i;
		}
	}

	printf("COULD NOT FIND MODEL: %s\n", name);

	return -1;

}

int Game_getTextureIndexByName(Game *game_p, const char *name){

	for(int i = 0; i < game_p->textures.size(); i++){
		if(strcmp(game_p->textures[i].name, name) == 0){
			return i;
		}
	}

	printf("COULD NOT FIND TEXTURE: %s\n", name);

	return -1;

}

int Game_getTriangleMeshIndexByName(Game *game_p, const char *name){

	for(int i = 0; i < game_p->triangleMeshes.size(); i++){
		if(strcmp(game_p->triangleMeshes[i].name, name) == 0){
			return i;
		}
	}

	printf("COULD NOT FIND TRIANGLE MESH: %s\n", name);

	return -1;

}

int Game_getShaderIndexByName(Game *game_p, const char *name){

	for(int i = 0; i < game_p->shaders.size(); i++){
		if(strcmp(game_p->shaders[i].name, name) == 0){
			return i;
		}
	}

	printf("COULD NOT FIND SHADER: %s\n", name);

	return -1;

}

Model *Game_getModelPointerByName(Game *game_p, const char *name){

	int index = Game_getModelIndexByName(game_p, name);

	if(index == -1){
		return NULL;
	}

	return &game_p->models[index];

}

Texture *Game_getTexturePointerByName(Game *game_p, const char *name){

	int index = Game_getTextureIndexByName(game_p, name);

	if(index == -1){
		return NULL;
	}

	return &game_p->textures[index];

}

TriangleMesh *Game_getTriangleMeshPointerByName(Game *game_p, const char *name){

	int index = Game_getTriangleMeshIndexByName(game_p, name);

	if(index == -1){
		return NULL;
	}

	return &game_p->triangleMeshes[index];

}

Shader *Game_getShaderPointerByName(Game *game_p, const char *name){

	int index = Game_getShaderIndexByName(game_p, name);

	if(index == -1){
		return NULL;
	}

	return &game_p->shaders[index];

}

/*
Obstacle *Game_getObstacleByID(Game *game_p, int ID){

	for(int i = 0; i < game_p->sprites.size(); i++){
		if(game_p->sprites[i].ID == ID){
			return &game_p->sprites[i];
		}
	}

	printf("Could not find sprite with ID: %i\n", ID);

	return NULL;

}
*/
