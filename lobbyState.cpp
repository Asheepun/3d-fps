#include "game.h"
#include "math.h"

enum ElementType{
	ELEMENT_TYPE_RECT,
	ELEMENT_TYPE_TEXT,
};

struct Element{
	enum ElementType type;
	int x;
	int y;
	int width;
	int height;
	unsigned int shader;
	char textureName[STRING_SIZE];
	Vec4f color;
};

std::vector<Renderer2D_Texture2D> cachedTextTextures;
std::vector<Element> elements;

//LobbyState serverLobbyState = { 2, 0 };

void addTextElement(int x, int y, const char *text, int fontSize, Font font, Vec4f color){

	Renderer2D_Texture2D texture;

	bool foundTexture = false;
	for(int i = 0; i < cachedTextTextures.size(); i++){
		if(strcmp(text, cachedTextTextures[i].texture.name) == 0){
			texture = cachedTextTextures[i];
			foundTexture = true;
			break;
		}
	}

	if(!foundTexture){
		Renderer2D_Texture2D_initFromText(&texture, text, font);
		cachedTextTextures.push_back(texture);
	}

	int width = texture.width * ((float)fontSize / (float)texture.height);
	int height = fontSize;

	{
		Element element;
		element.type = ELEMENT_TYPE_TEXT;
		element.x = x;
		element.y = y;
		element.width = width;
		element.height = height;
		element.color = color;
		String_set(element.textureName, texture.texture.name, STRING_SIZE);
		elements.push_back(element);
	}
	
}

bool textButtonDowned(int x, int y, const char *text, int fontSize, Font font, bool active){

	int borderSize = fontSize / 10;
	int margin = fontSize / 10;
	float alpha = 1.0;

	//if(active){
		//alpha -= 0.3;
	//}

	Renderer2D_Texture2D texture;

	bool foundTexture = false;
	for(int i = 0; i < cachedTextTextures.size(); i++){
		if(strcmp(text, cachedTextTextures[i].texture.name) == 0){
			texture = cachedTextTextures[i];
			foundTexture = true;
			break;
		}
	}

	if(!foundTexture){
		Renderer2D_Texture2D_initFromText(&texture, text, font);
		cachedTextTextures.push_back(texture);
	}

	int width = texture.width * ((float)fontSize / (float)texture.height);
	int height = fontSize;

	Vec2f pointerPos = Engine_pointer.pos * ((float)WIDTH / (float)Engine_clientWidth);

	bool hovered = false;
	if(pointerPos.x > x && pointerPos.x < x + width + borderSize * 2 + margin * 2
	&& pointerPos.y > y && pointerPos.y < y + height + borderSize * 2 + margin * 2){
		alpha -= 0.5;

		if(Engine_pointer.downed){
			hovered = true;
		}
	}

	{
		Element element;
		element.type = ELEMENT_TYPE_RECT;
		element.x = x;
		element.y = y;
		element.width = width + margin * 2 + borderSize * 2;
		element.height = height + margin * 2 + borderSize * 2;
		element.color = getVec4f(0.3, 0.3, 0.3, alpha);
		String_set(element.textureName, texture.texture.name, STRING_SIZE);
		elements.push_back(element);
	}
	{
		Element element;
		element.type = ELEMENT_TYPE_RECT;
		element.x = x + borderSize;
		element.y = y + borderSize;
		element.width = width + margin * 2;
		element.height = height + margin * 2;
		element.color = getVec4f(0.7, 0.7, 0.7, alpha);
		String_set(element.textureName, texture.texture.name, STRING_SIZE);
		elements.push_back(element);
	}
	{
		Element element;
		element.type = ELEMENT_TYPE_TEXT;
		element.x = x + margin + borderSize;
		element.y = y + margin + borderSize;
		element.width = width;
		element.height = height;
		element.color = getVec4f(0.3, 0.3, 0.3, alpha);
		String_set(element.textureName, texture.texture.name, STRING_SIZE);
		elements.push_back(element);
	}


	return hovered && Engine_pointer.downed;

}

void lobbyState(Game *game_p){

	elements.clear();

	pthread_mutex_lock(&game_p->client.serverLobbyStateMutex);

	ServerLobbyState serverLobbyState = game_p->client.latestServerLobbyState_mutexed;

	pthread_mutex_unlock(&game_p->client.serverLobbyStateMutex);

	//check if level should start
	pthread_mutex_lock(&game_p->client.startLevelMutex);

	bool startLevel = game_p->client.startLevel;
	StartLevelData startLevelData = game_p->client.startLevelData;

	pthread_mutex_unlock(&game_p->client.startLevelMutex);

	if(startLevel){

		srand(startLevelData.seed);

		//setup world
		World_clear(&game_p->world);

		for(int i = 0; i < game_p->trees.size(); i++){
			TextureBuffer_free(&game_p->trees[i].leafTransformationsTextureBuffer);
			free(game_p->trees[i].sortedLeafTransformations);
		}
		game_p->trees.clear();

		for(int i = 0; i < game_p->world.triangleMeshes.size(); i++){
			if(strcmp(game_p->world.triangleMeshes[i].name, "generated-tree") == 0){
				TriangleMesh_free(&game_p->world.triangleMeshes[i]);
				game_p->world.triangleMeshes.erase(game_p->world.triangleMeshes.begin() + i);
				i--;
				continue;
			}
		}

		for(int i = 0; i < game_p->models.size(); i++){
			if(strcmp(game_p->models[i].name, "generated-tree") == 0){
				Model_free(&game_p->models[i]);
				game_p->models.erase(game_p->models.begin() + i);
				i--;
				continue;
			}
		}

		//add obstacles
		World_addObstacle(
			&game_p->world,
			getVec3f(0.0, 0.0, 0.0),
			TERRAIN_SCALE,
			World_getTriangleMeshIndexByName(&game_p->world, "terrain"),
			Game_getModelIndexByName(game_p, "terrain"),
			Game_getTextureIndexByName(game_p, "terrain"),
			getVec4f(1.0, 1.0, 1.0, 1.0)
		);

		//add trees
		for(int i = 0; i < startLevelData.n_trees; i++){

			Tree tree;
			Model model;
			TriangleMesh triangleMesh;

			generateTree(startLevelData.treePositions[i], &tree, &model, &triangleMesh);

			game_p->trees.push_back(tree);

			game_p->models.push_back(model);
			game_p->world.triangleMeshes.push_back(triangleMesh);

			World_addObstacle(
				&game_p->world,
				startLevelData.treePositions[i],
				TREE_SCALE,
				game_p->world.triangleMeshes.size() - 1,
				game_p->models.size() - 1,
				Game_getTextureIndexByName(game_p, "bark"),
				getVec4f(0.7, 0.7, 0.7, 1.0)
			);
		
		}

		//add players
		for(int i = 0; i < startLevelData.n_players; i++){
			World_addPlayer(&game_p->world, startLevelData.playerPositions[i], startLevelData.playerConnectionIDs[i]);
		}

		//change state
		game_p->currentState = GAME_STATE_LEVEL;
		game_p->client.gameOver = false;
		game_p->dead = false;
		game_p->client.receivedGameState = false;

		Engine_fpsModeOn = true;

		return;

	}

	char text[STRING_SIZE];
	String_set(text, "", STRING_SIZE);
	sprintf(text, "Ready players: %i/%i", serverLobbyState.n_readyPlayers, serverLobbyState.n_players);
	addTextElement(300, 100, text, 100, game_p->font, getVec4f(0.3, 0.3, 0.3, 1.0));

	if(!game_p->client.ready && textButtonDowned(300, 300, "Ready", 100, game_p->font, false)
	|| game_p->client.ready && textButtonDowned(300, 300, "Not Ready", 100, game_p->font, false)){
		game_p->client.ready = !game_p->client.ready;
	}


	Client_sendReadyToServer(&game_p->client, game_p->client.ready);

}

void drawLobbyState(Game *game_p){

	glDisable(GL_DEPTH_TEST);

	Renderer2D_setShader(&game_p->renderer2D, game_p->renderer2D.colorShader);

	Renderer2D_setColor(&game_p->renderer2D, getVec4f(0.9, 0.9, 0.7, 1.0));

	Renderer2D_drawRectangle(&game_p->renderer2D, 0, 0, WIDTH, HEIGHT);

	//draw elements
	for(int i = 0; i < elements.size(); i++){

		Element *element_p = &elements[i];
		
		if(element_p->type == ELEMENT_TYPE_RECT){

			Renderer2D_setShader(&game_p->renderer2D, game_p->renderer2D.colorShader);

			Renderer2D_setColor(&game_p->renderer2D, element_p->color);
			Renderer2D_drawRectangle(&game_p->renderer2D, element_p->x, element_p->y, element_p->width, element_p->height);

		}
		
		if(element_p->type == ELEMENT_TYPE_TEXT){

			Renderer2D_Texture2D *texture_p = NULL;
			for(int j = 0; j < cachedTextTextures.size(); j++){
				if(strcmp(cachedTextTextures[j].texture.name, element_p->textureName) == 0){
					texture_p = &cachedTextTextures[j];
					break;
				}
			}

			Renderer2D_setShader(&game_p->renderer2D, game_p->renderer2D.textureColorShader);
			//Renderer2D_setShader(&game_p->renderer2D, game_p->renderer2D.colorShader);

			Renderer2D_setColor(&game_p->renderer2D, element_p->color);
			Renderer2D_setTexture(&game_p->renderer2D, texture_p->texture);
			Renderer2D_drawRectangle(&game_p->renderer2D, element_p->x, element_p->y, element_p->width, element_p->height);

		}

	}

	glEnable(GL_DEPTH_TEST);
}
