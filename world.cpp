#include "game.h"

#include "stdio.h"

void Game_simulateInputsOnClient(Game *game_p, Inputs inputs){

	Vec2f cameraDirectionXZ = getVec2f(inputs.cameraDirection.x, inputs.cameraDirection.z);
	Vec2f_normalize(&cameraDirectionXZ);

	if(inputs.forwards == 1){
		game_p->player.velocity.x += cameraDirectionXZ.x * PLAYER_SPEED;
		game_p->player.velocity.z += cameraDirectionXZ.y * PLAYER_SPEED;
	}
	if(inputs.backwards == 1){
		game_p->player.velocity.x += -cameraDirectionXZ.x * PLAYER_SPEED;
		game_p->player.velocity.z += -cameraDirectionXZ.y * PLAYER_SPEED;
	}
	if(inputs.left == 1){
		Vec3f left = getCrossVec3f(inputs.cameraDirection, getVec3f(0, 1.0, 0));
		Vec3f_normalize(&left);
		game_p->player.velocity.x += left.x * PLAYER_SPEED;
		game_p->player.velocity.z += left.z * PLAYER_SPEED;
	}
	if(inputs.right == 1){
		Vec3f right = getCrossVec3f(getVec3f(0, 1.0, 0), inputs.cameraDirection);
		Vec3f_normalize(&right);
		game_p->player.velocity.x += right.x * PLAYER_SPEED;
		game_p->player.velocity.z += right.z * PLAYER_SPEED;
	}

	game_p->player.velocity.x *= PLAYER_WALK_RESISTANCE;
	game_p->player.velocity.z *= PLAYER_WALK_RESISTANCE;

	Vec3f_add(&game_p->player.pos, game_p->player.velocity);

}

TriangleMesh generateTerrainTriangleMesh(int width, float heightScale){

	Vec3f *points = (Vec3f *)malloc(sizeof(Vec3f) * width * width);

	for(int x = 0; x < width; x++){
		for(int y = 0; y < width; y++){

			int index = y * width + x;

			points[index].x = x;
			points[index].z = y;
			//terrainPoints[index].y = getRandom() / TERRAIN_SCALE * 2.0;
			points[index].y = getRandom() * heightScale;

			points[index].x /= (float)width;
			points[index].z /= (float)width;
		
		}
	}

	int n_triangles = 2 * (width - 1) * (width - 1);

	Vec3f *triangles = (Vec3f *)malloc(sizeof(Vec3f) * 3 * n_triangles);

	int triangleIndex = 0;

	for(int x = 0; x < width - 1; x++){
		for(int y = 0; y < width - 1; y++){

			int pointsIndex1 = y * width + x;
			int pointsIndex2 = (y + 1) * width + x;
			int pointsIndex3 = y * width + (x + 1);
			int pointsIndex4 = (y + 1) * width + (x + 1);

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

			triangleIndex++;

		}
	}

	free(points);
	
	TriangleMesh triangleMesh;
	triangleMesh.triangles = triangles;
	triangleMesh.n_triangles = n_triangles;
	String_set(triangleMesh.name, "terrain", STRING_SIZE);

	return triangleMesh;
	
}

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
