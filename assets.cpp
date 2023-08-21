#include "game.h"

#include "engine/shaders.h"

void Game_loadAssets(Game *game_p){

	//load textures
	{
		const char *names[] = {
			"blank",
			"grass",
			"leaves",
			"bark",
		};

		int n_names = sizeof(names) / sizeof(char *);

		for(int i = 0; i < n_names; i++){

			char path[STRING_SIZE];
			String_set(path, "assets/textures/", STRING_SIZE);
			String_append(path, names[i]);
			String_append(path, ".png");

			Texture texture;
			Texture_initFromFile(&texture, path, names[i]);
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
		unsigned int vertexShader = getCompiledShader("shaders/vertex-shader.glsl", GL_VERTEX_SHADER);
		unsigned int fragmentShader = getCompiledShader("shaders/fragment-shader.glsl", GL_FRAGMENT_SHADER);

		unsigned int shaderProgram = glCreateProgram();
		glAttachShader(shaderProgram, vertexShader);
		glAttachShader(shaderProgram, fragmentShader);
		glLinkProgram(shaderProgram);

		game_p->modelShader = shaderProgram;
	}
	{
		unsigned int vertexShader = getCompiledShader("shaders/bone-model-vertex-shader.glsl", GL_VERTEX_SHADER);
		unsigned int fragmentShader = getCompiledShader("shaders/bone-model-fragment-shader.glsl", GL_FRAGMENT_SHADER);

		unsigned int shaderProgram = glCreateProgram();
		glAttachShader(shaderProgram, vertexShader);
		glAttachShader(shaderProgram, fragmentShader);
		glLinkProgram(shaderProgram);

		game_p->boneModelShader = shaderProgram;
	}
	{
		unsigned int vertexShader = getCompiledShader("shaders/grass-vertex-shader.glsl", GL_VERTEX_SHADER);
		unsigned int fragmentShader = getCompiledShader("shaders/grass-fragment-shader.glsl", GL_FRAGMENT_SHADER);

		unsigned int shaderProgram = glCreateProgram();
		glAttachShader(shaderProgram, vertexShader);
		glAttachShader(shaderProgram, fragmentShader);
		glLinkProgram(shaderProgram);

		game_p->grassShader = shaderProgram;
	}
	{
		unsigned int vertexShader = getCompiledShader("shaders/leaf-vertex-shader.glsl", GL_VERTEX_SHADER);
		unsigned int fragmentShader = getCompiledShader("shaders/leaf-fragment-shader.glsl", GL_FRAGMENT_SHADER);

		unsigned int shaderProgram = glCreateProgram();
		glAttachShader(shaderProgram, vertexShader);
		glAttachShader(shaderProgram, fragmentShader);
		glLinkProgram(shaderProgram);

		game_p->leafShader = shaderProgram;
	}

}
