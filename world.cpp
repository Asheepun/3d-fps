#include "game.h"

#include "stdio.h"

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
