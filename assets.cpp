#include "game.h"

#include "engine/shaders.h"

void Game_loadAssets(Game *game_p){

	//load textures
	{
		const char *names[] = {
			"blank",
			"grass",
			"small-grass",
			"leaves",
			"bark",
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
			"dude-bones2",
		};

		int n_names = sizeof(names) / sizeof(char *);

		for(int i = 0; i < n_names; i++){

			char meshPath[STRING_SIZE];
			String_set(meshPath, "assets/models/", STRING_SIZE);
			String_append(meshPath, names[i]);
			String_append(meshPath, ".bonemesh");

			char bonesPath[STRING_SIZE];
			String_set(bonesPath, "assets/models/", STRING_SIZE);
			String_append(bonesPath, names[i]);
			String_append(bonesPath, ".bones");

			BoneModel model;

			BoneModel_initFromFile(&model, meshPath, bonesPath);

			String_set(model.name, names[i], STRING_SIZE);

			game_p->boneModels.push_back(model);

		}
	}

	//load triangle meshes
	{
		const char *names[] = {
			"cube",
		};

		int n_names = sizeof(names) / sizeof(char *);

		for(int i = 0; i < n_names; i++){

			char path[STRING_SIZE];
			String_set(path, "assets/models/", STRING_SIZE);
			String_append(path, names[i]);
			String_append(path, ".mesh");

			TriangleMesh triangleMesh;

			TriangleMesh_initFromFile_mesh(&triangleMesh, path);

			String_set(triangleMesh.name, names[i], STRING_SIZE);

			game_p->triangleMeshes.push_back(triangleMesh);

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
