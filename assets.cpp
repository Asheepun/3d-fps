#include "engine/shaders.h"

#include "game.h"

#include "math.h"

void Game_loadAssets(Game *game_p){

	//load textures
	{
		const char *names[] = {
			"blank",
			"grass",
			"small-grass",
			"leaves",
			"bark",
			"voronoi",
			"voronoi-smooth",
			"voronoi-smooth-normals",
			"water-normals",
			"ripple-noise",
		};

		int n_names = sizeof(names) / sizeof(char *);

		//load as color textures
		for(int i = 0; i < n_names; i++){

			char path[STRING_SIZE];
			String_set(path, "assets/textures/", STRING_SIZE);
			String_append(path, names[i]);
			String_append(path, ".png");

			Texture texture;
			Texture_initFromFile(&texture, path, names[i]);
			game_p->textures.push_back(texture);

		}

		//load as alpha textures
		for(int i = 0; i < n_names; i++){

			char path[STRING_SIZE];
			String_set(path, "assets/textures/", STRING_SIZE);
			String_append(path, names[i]);
			String_append(path, ".png");

			char name[STRING_SIZE];
			String_set(name, names[i], STRING_SIZE);
			String_append(name, "-alpha");

			Texture texture;
			Texture_initFromFileAsAlphaMap(&texture, path, name);
			game_p->textures.push_back(texture);

		}

	}

	//load static models
	{
		const char *names[] = {
			"quad",
			"cube",
			"sword",
			"gun",
		};

		int n_names = sizeof(names) / sizeof(char *);

		for(int i = 0; i < n_names; i++){

			char path[STRING_SIZE];
			String_set(path, "assets/models/", STRING_SIZE);
			String_append(path, names[i]);
			String_append(path, ".mesh");

			Model model;

			Model_initFromFile_mesh(&model, path);

			String_set(model.name, names[i], STRING_SIZE);

			game_p->models.push_back(model);

		}
	}

	//load bone models
	{
		const char *names[] = {
			"dude-bones",
			"gubbe",
			//"dude-bones2",
		};

		int n_names = sizeof(names) / sizeof(char *);

		for(int i = 0; i < n_names; i++){

			char path[STRING_SIZE];
			String_set(path, "assets/models/", STRING_SIZE);
			String_append(path, names[i]);
			String_append(path, ".bonemesh");

			BoneModel model;

			BoneModel_initFromFile(&model, path);

			String_set(model.name, names[i], STRING_SIZE);

			game_p->boneModels.push_back(model);

		}
	}

	//load shaders
	{
		const char *names[] = {

			"model", "vertex-shader", "fragment-shader",
			"model-shadow", "vertex-shader", "shadow-map-opaque-fragment-shader",

			"bone", "bone-model-vertex-shader", "bone-model-fragment-shader",
			"bone-shadow", "bone-model-vertex-shader", "shadow-map-fragment-shader",

			"grass", "grass-vertex-shader", "grass-fragment-shader",
			"grass-shadow", "grass-vertex-shader", "shadow-map-fragment-shader",

			"leaf", "leaf-vertex-shader", "leaf-fragment-shader",
			"leaf-shadow", "leaf-vertex-shader", "shadow-map-fragment-shader",

			"water", "water-vertex-shader", "water-fragment-shader",

			"grass-particle", "grass-particle-vertex-shader", "grass-particle-fragment-shader",

		};

		int n_names = (sizeof(names) / sizeof(char *)) / 3;

		for(int i = 0; i < n_names; i++){
			
			Shader shader;

			char vertexShaderPath[STRING_SIZE];
			String_set(vertexShaderPath, "shaders/", STRING_SIZE);
			String_append(vertexShaderPath, names[i * 3 + 1]);
			String_append(vertexShaderPath, ".glsl");

			char fragmentShaderPath[STRING_SIZE];
			String_set(fragmentShaderPath, "shaders/", STRING_SIZE);
			String_append(fragmentShaderPath, names[i * 3 + 2]);
			String_append(fragmentShaderPath, ".glsl");

			Shader_init(&shader, names[i * 3 + 0], vertexShaderPath, fragmentShaderPath);

			game_p->shaders.push_back(shader);

		}
	}

}

void World_loadAssets(World *world_p, const char *rootPath){

	//load triangle meshes
	{
		const char *names[] = {
			"quad",
			"cube",
		};

		int n_names = sizeof(names) / sizeof(char *);

		for(int i = 0; i < n_names; i++){

			char path[STRING_SIZE];
			String_set(path, rootPath, STRING_SIZE);
			String_append(path, "assets/models/");
			String_append(path, names[i]);
			String_append(path, ".mesh");

			TriangleMesh triangleMesh;

			TriangleMesh_initFromFile_mesh(&triangleMesh, path);

			String_set(triangleMesh.name, names[i], STRING_SIZE);

			world_p->triangleMeshes.push_back(triangleMesh);

		}
	
	}
	
	//load bone triangle meshes
	{
		const char *names[] = {
			"dude-bones",
			"gubbe",
		};

		int n_names = sizeof(names) / sizeof(char *);

		for(int i = 0; i < n_names; i++){

			char path[STRING_SIZE];
			String_set(path, rootPath, STRING_SIZE);
			String_append(path, "assets/models/");
			String_append(path, names[i]);
			String_append(path, ".bonemesh");

			BoneTriangleMesh boneTriangleMesh;

			BoneTriangleMesh_initFromFile(&boneTriangleMesh, path);

			String_set(boneTriangleMesh.name, names[i], STRING_SIZE);

			world_p->boneTriangleMeshes.push_back(boneTriangleMesh);

		}
	
	}

	//load bone rigs
	{
		const char *names[] = {
			"dude-bones",
			"dude-bones2",
			"gubbe",
		};

		int n_names = sizeof(names) / sizeof(char *);

		for(int i = 0; i < n_names; i++){

			char path[STRING_SIZE];
			String_set(path, rootPath, STRING_SIZE);
			String_append(path, "assets/models/");
			String_append(path, names[i]);
			String_append(path, ".bones");

			BoneRig rig;

			BoneRig_initFromFile(&rig, path);

			String_set(rig.name, names[i], STRING_SIZE);

			world_p->boneRigs.push_back(rig);

		}
	
	}

}

void *loadAssetsAndGenerateStuffThreaded(void *gamePointer){

	Game *game_p = (Game *)gamePointer;

	printf("began loading assets\n");

	Game_loadAssets(game_p);
	World_loadAssets(&game_p->world, "./");

	printf("loaded assets\n");

	printf("done generating stuff\n");

	return NULL;

}
