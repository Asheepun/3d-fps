#include "game.h"
#include "math.h"

#include "stdio.h"

int CURRENT_AVAILABLE_ID = 0;

//ENTITY FUNCTIONS

bool Player_World_shoot_common(Player *player_p, World *world_p, std::vector<Player> checkPlayers, Vec3f *output_hitPosition, Vec3f *output_hitNormal, int *output_hitConnectionID){

	Vec3f rayPosition = player_p->pos + getVec3f(0.0, player_p->height, 0.0);
	Vec3f rayDirection = player_p->direction;

	Vec3f intersectionPoint = getVec3f(0.0);
	float minDistance = -1.0;
	Vec3f intersectionNormal;
	int hitConnectionID;
	bool hit = false;

	for(int i = 0; i < checkPlayers.size(); i++){

		Player *checkPlayer_p = &world_p->players[i];

		if(checkPlayer_p->connectionID != player_p->connectionID){

			BoneTriangleMesh *boneTriangleMesh_p = &world_p->boneTriangleMeshes[0];
			BoneRig *boneRig_p = &world_p->boneRigs[0];
			BoneRig *boneRig2_p = &world_p->boneRigs[1];

			std::vector<Mat4f> boneTransformations = getBoneRigTransformations(boneRig_p, boneRig2_p->originBones);

			float scale = 0.5;

			Mat4f modelMatrix = getModelMatrix(checkPlayer_p->pos, getVec3f(scale), IDENTITY_QUATERNION);

			for(int j = 0; j < boneTriangleMesh_p->n_triangles; j++){

				Vec3f checkPoint;

				Vec3f points[3];

				for(int k = 0; k < 3; k++){

					int triangleIndex = j * 3 + k;

					Mat4f jointMatrix = boneTransformations[boneTriangleMesh_p->indices[triangleIndex * 4 + 0]] * boneTriangleMesh_p->weights[triangleIndex].x
						+ boneTransformations[boneTriangleMesh_p->indices[triangleIndex * 4 + 1]] * boneTriangleMesh_p->weights[triangleIndex].y
						+ boneTransformations[boneTriangleMesh_p->indices[triangleIndex * 4 + 2]] * boneTriangleMesh_p->weights[triangleIndex].z
						+ boneTransformations[boneTriangleMesh_p->indices[triangleIndex * 4 + 3]] * boneTriangleMesh_p->weights[triangleIndex].w;

					points[k] = mulVec3fMat4f(boneTriangleMesh_p->triangles[triangleIndex], modelMatrix * jointMatrix, 1.0);

				}

				if(checkLineToTriangleIntersectionVec3f(rayPosition, rayPosition + rayDirection, points[0], points[1], points[2], &checkPoint)
				&& (minDistance < 0.0
				|| length(checkPoint - rayPosition) < minDistance
				&& dot(checkPoint - rayPosition, rayDirection) > 0.0)){
					intersectionPoint = checkPoint;
					minDistance = length(intersectionPoint - rayPosition);
					intersectionNormal = cross(points[0] - points[1], points[0] - points[2]);
					hitConnectionID = checkPlayer_p->connectionID;
					hit = true;
				}
			}
		
		}
	
	}

	for(int i = 0; i < world_p->obstacles.size(); i++){
		
		Obstacle *obstacle_p = &world_p->obstacles[i];
		TriangleMesh *triangleMesh_p = &world_p->triangleMeshes[obstacle_p->triangleMeshIndex];

		Mat4f modelMatrix = getModelMatrix(obstacle_p->pos, getVec3f(obstacle_p->scale), IDENTITY_QUATERNION);

		for(int j = 0; j < triangleMesh_p->n_triangles; j++){
			
			Vec3f p1 = mulVec3fMat4f(triangleMesh_p->triangles[j * 3 + 0], modelMatrix, 1.0);
			Vec3f p2 = mulVec3fMat4f(triangleMesh_p->triangles[j * 3 + 1], modelMatrix, 1.0);
			Vec3f p3 = mulVec3fMat4f(triangleMesh_p->triangles[j * 3 + 2], modelMatrix, 1.0);

			Vec3f checkPoint;

			if(checkLineToTriangleIntersectionVec3f(rayPosition, rayPosition + rayDirection, p1, p2, p3, &checkPoint)
			&& length(checkPoint - rayPosition) < length(intersectionPoint - rayPosition)
			&& dot(checkPoint - rayPosition, rayDirection) > 0.0){
				hit = false;
			}

		}

	}

	*output_hitPosition = intersectionPoint;
	*output_hitNormal = normalize(intersectionNormal);
	*output_hitConnectionID = hitConnectionID;

	return hit;

}

void Player_World_moveAndCollideBasedOnInputs_common(Player *player_p, World *world_p, Inputs inputs){

	//apply inputs
	{
		Vec2f cameraDirectionXZ = getVec2f(inputs.cameraDirection.x, inputs.cameraDirection.z);
		Vec2f_normalize(&cameraDirectionXZ);

		float speed = PLAYER_SPEED;
		if(inputs.crouch == 1){
			speed = PLAYER_CROUCH_SPEED;
		}

		if(inputs.forwards == 1){
			player_p->velocity.x += cameraDirectionXZ.x * speed;
			player_p->velocity.z += cameraDirectionXZ.y * speed;
		}
		if(inputs.backwards == 1){
			player_p->velocity.x += -cameraDirectionXZ.x * speed;
			player_p->velocity.z += -cameraDirectionXZ.y * speed;
		}
		if(inputs.left == 1){
			Vec3f left = getCrossVec3f(inputs.cameraDirection, getVec3f(0, 1.0, 0));
			Vec3f_normalize(&left);
			player_p->velocity.x += left.x * speed;
			player_p->velocity.z += left.z * speed;
		}
		if(inputs.right == 1){
			Vec3f right = getCrossVec3f(getVec3f(0, 1.0, 0), inputs.cameraDirection);
			Vec3f_normalize(&right);
			player_p->velocity.x += right.x * speed;
			player_p->velocity.z += right.z * speed;
		}
		if(inputs.jump == 1
		&& player_p->onGround){
			player_p->velocity.y += PLAYER_JUMP_SPEED;
		}
		if(inputs.jump == 0
		&& player_p->velocity.y > 0.0){
			player_p->velocity.y *= 0.9;
		}
		if(inputs.crouch == 1){
			if(player_p->height > PLAYER_HEIGHT_CROUCHING){
				player_p->height -= PLAYER_HEIGHT_SPEED;
			}
		}else{
			if(player_p->height < PLAYER_HEIGHT_STANDING){
				player_p->height += PLAYER_HEIGHT_SPEED * 1.2;
			}
		}
	}

	//handle physics and move
	{
		player_p->lastPos = player_p->pos;

		player_p->velocity.y += -PLAYER_GRAVITY;

		player_p->velocity.x *= PLAYER_WALK_RESISTANCE;
		player_p->velocity.z *= PLAYER_WALK_RESISTANCE;

		Vec3f_add(&player_p->pos, player_p->velocity);

		player_p->onGround = false;
	}

	//handle oub
	{
		if(player_p->pos.y < 0.0){
			player_p->pos.y = 0.0;
			player_p->velocity.y = 0.0;
		}
		if(player_p->pos.x < 0.0){
			player_p->pos.x = 0.0;
			player_p->velocity.x = 0.0;
		}
		if(player_p->pos.x > TERRAIN_SCALE){
			player_p->pos.x = TERRAIN_SCALE;
			player_p->velocity.x = 0.0;
		}
		if(player_p->pos.z < 0.0){
			player_p->pos.z = 0.0;
			player_p->velocity.z = 0.0;
		}
		if(player_p->pos.z > TERRAIN_SCALE){
			player_p->pos.z = TERRAIN_SCALE;
			player_p->velocity.z = 0.0;
		}
	}

	//handle obstacle collisions
	{
		float radius = 0.5;

		Mat4f cylinderMatrix = getScalingMat4f(1.0 / radius, 1.0 / player_p->height, 1.0 / radius) * getTranslationMat4f(-player_p->pos.x, -(player_p->pos.y), -player_p->pos.z);

		for(int j = 0; j < world_p->obstacles.size(); j++){

			Obstacle *obstacle_p = &world_p->obstacles[j];
			TriangleMesh *triangleMesh_p = &world_p->triangleMeshes[obstacle_p->triangleMeshIndex];

			Mat4f triangleMatrix = cylinderMatrix * getModelMatrix(obstacle_p->pos, getVec3f(obstacle_p->scale), IDENTITY_QUATERNION);

			Vec3f intersectionPoint;
			Vec3f collisionNormal;
			bool hit = false;

			for(int k = 0; k < triangleMesh_p->n_triangles; k++){

				Vec3f p1 = mulVec3fMat4f(triangleMesh_p->triangles[k * 3 + 0], triangleMatrix, 1.0);
				Vec3f p2 = mulVec3fMat4f(triangleMesh_p->triangles[k * 3 + 1], triangleMatrix, 1.0);
				Vec3f p3 = mulVec3fMat4f(triangleMesh_p->triangles[k * 3 + 2], triangleMatrix, 1.0);

				collisionNormal = normalize(cross(p1 - p2, p1 - p3));

				if(checkLineToTriangleIntersectionVec3f(getVec3f(collisionNormal.x, 0.0, collisionNormal.z), getVec3f(collisionNormal.x, 1.0, collisionNormal.z), p1, p2, p3, &intersectionPoint)
				&& intersectionPoint.y > 0.0 && intersectionPoint.y < 1.0){
					hit = true;
					break;
				}

				float maxY = fmax(fmax(p1.y, p2.y), p3.y);
				float minY = fmin(fmin(p1.y, p2.y), p3.y);

				if(maxY > 1.0
				&& minY < 0.0){
					bool checkHit = checkLineToTriangleIntersectionVec3f(getVec3f(0.0, 0.0, 0.0), getVec3f(-collisionNormal.x, 0.0, -collisionNormal.z), p1, p2, p3, &intersectionPoint);

					if(checkHit
					&& dot(intersectionPoint, intersectionPoint) < 1.0){
						hit = true;
						break;
					}

				}

			}

			if(hit){
				if(acos(dot(collisionNormal, getVec3f(0.0, 1.0, 0.0))) < M_PI / 4.0){
					player_p->pos.y += intersectionPoint.y;
					player_p->velocity.y = 0.0;
					player_p->onGround = true;
				}else{
					player_p->pos += collisionNormal * (radius - dot(getVec3f(intersectionPoint.x * radius, intersectionPoint.y, intersectionPoint.z * radius), collisionNormal * -1.0));
				}
			}

		}
	}

}

void World_addPlayer(World *world_p, Vec3f pos, int connectionID){

	Player player;

	player.pos = pos;
	player.connectionID = connectionID;

	player.lastPos = player.pos;
	player.velocity = getVec3f(0.0);
	player.onGround = false;
	player.height = PLAYER_HEIGHT_STANDING;
	player.weapon = WEAPON_GUN;
	player.health = 100;

	world_p->players.push_back(player);

}

int World_addObstacle(World *world_p, Vec3f pos, float scale, int triangleMeshIndex, int modelIndex, int textureIndex, Vec4f color){

	Obstacle obstacle;	

	obstacle.ID = CURRENT_AVAILABLE_ID;
	CURRENT_AVAILABLE_ID++;

	obstacle.pos = pos;
	obstacle.scale = scale;
	obstacle.triangleMeshIndex = triangleMeshIndex;
	obstacle.modelIndex = modelIndex;
	obstacle.textureIndex = textureIndex;
	obstacle.color = color;

	world_p->obstacles.push_back(obstacle);

	return obstacle.ID;

}

int World_getPlayerIndexByConnectionID(World *world_p, int connectionID){

	for(int i = 0; i < world_p->players.size(); i++){
		if(world_p->players[i].connectionID == connectionID){
			return i;
		}
	}

	printf("Could not find player with connection ID: %i\n", connectionID);

	return -1;
}

Player *World_getPlayerPointerByConnectionID(World *world_p, int connectionID){

	int index = World_getPlayerIndexByConnectionID(world_p, connectionID);

	if(index != -1){
		return &world_p->players[index];
	}

	return NULL;

}

void World_clear(World *world_p){
	world_p->obstacles.clear();
	world_p->players.clear();
}

void generateTerrainTriangles(Vec3f **output_triangles, Vec2f **output_textureCoords, int *output_n_triangles){

	int width = 20;

	Vec3f *points = (Vec3f *)malloc(sizeof(Vec3f) * (width + 1) * (width + 1));

	for(int x = 0; x < width + 1; x++){
		for(int y = 0; y < width + 1; y++){

			int index = y * (width + 1) + x;

			points[index].x = x;
			points[index].z = y;
			points[index].y = getRandom() / TERRAIN_SCALE * 2.0;
			//points[index].y = 0.0;

			if(y == 0 || x == 0 || y == width || x == width){
				points[index].y = 0.0;
			}

			points[index].x /= (float)width;
			points[index].z /= (float)width;
		
		}
	}

	//printf("%f\n", terrainMaxHeight);

	int n_triangles = 2 * (width) * (width);

	if(output_triangles != NULL){
		*output_triangles = (Vec3f *)malloc(sizeof(Vec3f) * 3 * n_triangles);
	}
	if(output_textureCoords != NULL){
		*output_textureCoords = (Vec2f *)malloc(sizeof(Vec2f) * 3 * n_triangles);
	}
	if(output_n_triangles != NULL){
		*output_n_triangles = n_triangles;
	}

	int triangleIndex = 0;

	for(int x = 0; x < width; x++){
		for(int y = 0; y < width; y++){

			int pointsIndex1 = y * (width + 1) + x;
			int pointsIndex2 = (y + 1) * (width + 1) + x;
			int pointsIndex3 = y * (width + 1) + (x + 1);
			int pointsIndex4 = (y + 1) * (width + 1) + (x + 1);

			Vec3f point1 = points[pointsIndex1];
			Vec3f point2 = points[pointsIndex2];
			Vec3f point3 = points[pointsIndex3];
			Vec3f point4 = points[pointsIndex4];

			if(output_triangles != NULL){
				(*output_triangles)[triangleIndex * 3 * 2 + 0] = point1;
				(*output_triangles)[triangleIndex * 3 * 2 + 1] = point2;
				(*output_triangles)[triangleIndex * 3 * 2 + 2] = point4;
				(*output_triangles)[triangleIndex * 3 * 2 + 3] = point1;
				(*output_triangles)[triangleIndex * 3 * 2 + 4] = point4;
				(*output_triangles)[triangleIndex * 3 * 2 + 5] = point3;
			}

			Vec2f t1 = getVec2f(x, y) / (float)width;
			Vec2f t2 = getVec2f(x, y + 1.0) / (float)width;
			Vec2f t3 = getVec2f(x + 1.0, y) / (float)width;
			Vec2f t4 = getVec2f(x + 1.0, y + 1.0) / (float)width;

			if(output_textureCoords != NULL){
				(*output_textureCoords)[triangleIndex * 3 * 2 + 0] = t1;
				(*output_textureCoords)[triangleIndex * 3 * 2 + 1] = t2;
				(*output_textureCoords)[triangleIndex * 3 * 2 + 2] = t4;
				(*output_textureCoords)[triangleIndex * 3 * 2 + 3] = t1;
				(*output_textureCoords)[triangleIndex * 3 * 2 + 4] = t4;
				(*output_textureCoords)[triangleIndex * 3 * 2 + 5] = t3;
			}

			triangleIndex++;

		}
	}

	free(points);
	
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

int World_getTriangleMeshIndexByName(World *world_p, const char *name){

	for(int i = 0; i < world_p->triangleMeshes.size(); i++){
		if(strcmp(world_p->triangleMeshes[i].name, name) == 0){
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

TriangleMesh *World_getTriangleMeshPointerByName(World *world_p, const char *name){

	int index = World_getTriangleMeshIndexByName(world_p, name);

	if(index == -1){
		return NULL;
	}

	return &world_p->triangleMeshes[index];

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
