#include "game.h"
#include "math.h"

float gameTime = 0.0;
float windTime = 0.0;

float fov = M_PI / 4;
float farPlane = 100.0;
float nearPlane = 0.1;
Vec3f cameraPos = getVec3f(4.0, 5.0, 0.0);
Vec3f cameraDirection = getVec3f(0.0, 0.0, 1.0);
Vec2f cameraRotation = getVec2f(M_PI / 2.0, 0.0);

Vec3f lightPos = { 0.0, 20.0, 0.0 };
Vec3f lightDirection = { 0.7, -1.0, 0.5 };

float SWING_ANGLE_RANGE = M_PI / 4.0;
float swingAngle = SWING_ANGLE_RANGE;

int currentlyHeldRigidBodyIndex = -1;
float holdingDistance = 0.0;

void levelState(Game *game_p){

#ifndef RUN_OFFLINE
	if(!game_p->client.receivedGameState){
		return;
	}
#endif

	//if(game_p->dead){
		//return;
	//}

	//printf("---\n");

	if(Engine_keys[ENGINE_KEY_G].downed){
		game_p->world.players[0].weapon = (Weapon)((int)game_p->world.players[0].weapon + 1);
		if(game_p->world.players[0].weapon >= N_WEAPONS){
			game_p->world.players[0].weapon = (Weapon)0;
		}
	}

	//printf("player: %i\n", game_p->world.players.size());

	//check if game is over
	if(game_p->client.gameOver){
		printf("game over!\n");

		game_p->client.ready = false;
		game_p->client.startLevel_mutexed = false;
		game_p->currentState = GAME_STATE_LOBBY;

		Engine_fpsModeOn = false;

		return;
	}

	//printf("---\n");

	//get latest server game state
#ifndef RUN_OFFLINE
	pthread_mutex_lock(&game_p->client.serverGameStateMutex);

	ServerGameState serverGameState = game_p->client.latestServerGameState_mutexed;

	pthread_mutex_unlock(&game_p->client.serverGameStateMutex);
#endif

	//get server shots
#ifndef RUN_OFFLINE
	pthread_mutex_lock(&game_p->client.latestServerShotsMutex);

	int n_serverShots = game_p->client.latestServerShots_mutexed.size();
	ShotData serverShots[n_serverShots];
	memcpy(serverShots, &game_p->client.latestServerShots_mutexed[0], n_serverShots * sizeof(ShotData));

	game_p->client.latestServerShots_mutexed.clear();

	pthread_mutex_unlock(&game_p->client.latestServerShotsMutex);
#endif

	//if(n_serverShots > 0){
		//printf("shot!\n");
	//}

	Inputs inputs;
	memset(&inputs, 0, sizeof(Inputs));

	//handle keyboard inputs
	if(Engine_keys[ENGINE_KEY_W].down){
		inputs.forwards = 1;
	}
	if(Engine_keys[ENGINE_KEY_S].down){
		inputs.backwards = 1;
	}
	if(Engine_keys[ENGINE_KEY_A].down){
		inputs.left = 1;
	}
	if(Engine_keys[ENGINE_KEY_D].down){
		inputs.right = 1;
	}
	if(Engine_keys[ENGINE_KEY_SPACE].down){
		inputs.jump = 1;
	}
	if(Engine_keys[ENGINE_KEY_CONTROL].down){
		inputs.crouch = 1;
	}
	if(Engine_pointer.downed){
		inputs.shoot = 1;
	}

#ifndef RUN_OFFLINE
	inputs.sendingNumber = game_p->client.n_sentInputs;
	inputs.gameTime = serverGameState.gameTime;
#endif

	//handle pointer intputs
	cameraRotation.x += -Engine_pointer.movement.x * PLAYER_LOOK_SPEED;
	cameraRotation.y += -Engine_pointer.movement.y * PLAYER_LOOK_SPEED;

	if(cameraRotation.y > M_PI / 2 - 0.01){
		cameraRotation.y = M_PI / 2 - 0.01;
	}
	if(cameraRotation.y < -(M_PI / 2 - 0.01)){
		cameraRotation.y = -(M_PI / 2 - 0.01);
	}

	cameraDirection.y = sin(cameraRotation.y);
	cameraDirection.x = cos(cameraRotation.x) * cos(cameraRotation.y);
	cameraDirection.z = sin(cameraRotation.x) * cos(cameraRotation.y);
	Vec3f_normalize(&cameraDirection);

	inputs.cameraDirection = cameraDirection;

#ifndef RUN_OFFLINE
	//send inputs to server
	Client_sendInputsToServer(&game_p->client, inputs);

	//update world based on latest server game state
	for(int i = 0; i < game_p->world.players.size(); i++){

		Player *player_p = &game_p->world.players[i];

		PlayerData *serverPlayerData_p = NULL;
		for(int j = 0; j < serverGameState.n_players; j++){
			if(serverGameState.players[j].connectionID == player_p->connectionID){
				serverPlayerData_p = &serverGameState.players[j];
			}
		}

		if(serverPlayerData_p == NULL){

			//check if player is dead
			if(player_p->connectionID == game_p->client.connectionID){
				player_p->health = 0;
				game_p->dead = true;
				continue;
			}

			game_p->world.players.erase(game_p->world.players.begin() + i);
			i--;
			continue;
		}

		player_p->health = serverPlayerData_p->health;

		if(player_p->connectionID != game_p->client.connectionID
		|| length(player_p->pos - serverPlayerData_p->pos) > 5.0){
			player_p->pos = serverPlayerData_p->pos;
			player_p->direction = serverPlayerData_p->direction;
			player_p->onGround = serverPlayerData_p->onGround;
			player_p->height = serverPlayerData_p->height;
			player_p->walkAngle = serverPlayerData_p->walkAngle;
		}
	
	}

	//store player positions in buffer
	game_p->world.pastPlayersBuffer.push_back(game_p->world.players);

	if(game_p->world.pastPlayersBuffer.size() > PAST_PLAYERS_BUFFER_SIZE){
		game_p->world.pastPlayersBuffer.erase(game_p->world.pastPlayersBuffer.begin());
	}

#endif

	//handle inputs on client
	if(!game_p->dead){
		Player *player_p = World_getPlayerPointerByConnectionID(&game_p->world, game_p->client.connectionID);
		//Player *player_p = &game_p->world.players[1];

		player_p->direction = inputs.cameraDirection;

		if(inputs.shoot){
			if(player_p->weapon == WEAPON_GUN){

				Vec3f hitPosition;
				Vec3f hitNormal;
				int hitConnectionID;
				bool hit = Player_World_shoot_common(player_p, &game_p->world, game_p->world.players, &hitPosition, &hitNormal, &hitConnectionID);

				Vec3f startPos = player_p->pos + getVec3f(0.0, player_p->height, 0.0) + player_p->direction * 2.0;
				Vec3f endPos = startPos + player_p->direction * 100.0;

				Vec3f cameraRight = cross(player_p->direction, getVec3f(0.0, 1.0, 0.0));
				Vec3f cameraUp = normalize(cross(cameraRight, player_p->direction));
				startPos -= cameraUp * 0.3;

				if(hit){

					int n_particles = 17 + getRandom() * 5;

					for(int j = 0; j < n_particles; j++){

						Particle particle;
						particle.pos = hitPosition;
						particle.velocity = hitNormal * 0.1;
						particle.velocity.y += 0.1;
						particle.velocity += normalize(getVec3f(getRandom() - 0.5, getRandom() - 0.5, getRandom() - 0.5)) * 0.05;
						particle.scale = getVec3f(0.1 * (0.8 + 0.4 * getRandom()));
						
						game_p->bloodParticles.push_back(particle);

					}

					endPos = hitPosition;

				}

				BulletTrace bulletTrace;
				bulletTrace.startPos = startPos;
				bulletTrace.endPos = endPos;
				bulletTrace.alpha = 1.0;

				game_p->bulletTraces.push_back(bulletTrace);

			}
			if(player_p->weapon == WEAPON_SWORD){

				swingAngle = -SWING_ANGLE_RANGE;
			
			}
		}
	
		Player_World_moveAndCollideBasedOnInputs_common(player_p, &game_p->world, inputs);
	}

#ifndef RUN_OFFLINE
	//handle shots on client
	for(int i = 0; i < n_serverShots; i++){

		ShotData *shot_p = &serverShots[i];

		int timeDifference = serverGameState.gameTime - inputs.gameTime;
		int pastPlayersBufferIndex = max(game_p->world.pastPlayersBuffer.size() - 1 - timeDifference, 0);

		if(shot_p->connectionID != game_p->client.connectionID){

			Player *player_p = World_getPlayerPointerByConnectionID(&game_p->world, shot_p->connectionID);

			Vec3f hitPosition;
			Vec3f hitNormal;
			int hitConnectionID;
			bool hit = Player_World_shoot_common(player_p, &game_p->world, game_p->world.pastPlayersBuffer[pastPlayersBufferIndex], &hitPosition, &hitNormal, &hitConnectionID);
			Vec3f startPos = player_p->pos + getVec3f(0.0, player_p->height, 0.0) + player_p->direction * 2.0;
			Vec3f endPos = startPos + player_p->direction * 100.0;

			Vec3f cameraRight = cross(player_p->direction, getVec3f(0.0, 1.0, 0.0));
			Vec3f cameraUp = normalize(cross(cameraRight, player_p->direction));
			startPos -= cameraUp * 0.3;

			if(hit){

				int n_particles = 17 + getRandom() * 5;

				for(int j = 0; j < n_particles; j++){

					Particle particle;
					particle.pos = hitPosition;
					particle.velocity = hitNormal * 0.1;
					particle.velocity.y += 0.1;
					particle.velocity += normalize(getVec3f(getRandom() - 0.5, getRandom() - 0.5, getRandom() - 0.5)) * 0.05;
					particle.scale = getVec3f(0.1 * (0.8 + 0.4 * getRandom()));
					
					game_p->bloodParticles.push_back(particle);

				}

				endPos = hitPosition;

			}

			BulletTrace bulletTrace;
			bulletTrace.startPos = startPos;
			bulletTrace.endPos = endPos;
			bulletTrace.alpha = 1.0;

			game_p->bulletTraces.push_back(bulletTrace);

		}

	}
#endif

	/*
	//check if rigid bodies are dragged
	for(int i = 0; i < game_p->rigidBodies.size(); i++){
		
		RigidBody *rigidBody_p = game_p->rigidBodies[i];
		TriangleMesh *triangleMesh_p = game_p->triangleMeshes[rigidBody_p->triangleMeshIndex];

		rigidBody_p->lastPos = rigidBody_p->pos;
		rigidBody_p->lastOrientation = rigidBody_p->orientation;

		if(Engine_keys[ENGINE_KEY_U].downed){
			rigidBody_p->velocity.y += 0.5;
			rigidBody_p->angularVelocity = getVec3f(getRandom(), getRandom(), getRandom());
		}

		float scale = 1.0;
		Mat4f modelMatrix = getIdentityMat4f();
		modelMatrix *= getScalingMat4f(scale);
		modelMatrix *= getQuaternionMat4f(rigidBody_p->orientation);
		modelMatrix *= getTranslationMat4f(rigidBody_p->pos);

		if(Engine_pointer.downed){

			Vec3f intersectionPoint;
			bool hit = false;

			for(int j = 0; j < triangleMesh_p->n_triangles; j++){
				Vec3f p1 = mulVec3fMat4f(triangleMesh_p->triangles[j * 3 + 0], modelMatrix, 1.0);
				Vec3f p2 = mulVec3fMat4f(triangleMesh_p->triangles[j * 3 + 1], modelMatrix, 1.0);
				Vec3f p3 = mulVec3fMat4f(triangleMesh_p->triangles[j * 3 + 2], modelMatrix, 1.0);


				if(checkLineToTriangleIntersectionVec3f(cameraPos, cameraPos + cameraDirection, p1, p2, p3, &intersectionPoint)){
					hit = true;
				}
			}

			if(hit){
				currentlyHeldRigidBodyIndex = i;
				holdingDistance = getMagVec3f(cameraPos - rigidBody_p->pos);
			}
		}

		if(Engine_pointer.down
		&& currentlyHeldRigidBodyIndex == i){
			rigidBody_p->pos = cameraPos + cameraDirection * holdingDistance;
			rigidBody_p->velocity = getVec3f(0.0, 0.0, 0.0);
			rigidBody_p->angularVelocity = getVec3f(0.0, 0.0, 0.0);
		}

		if(Engine_pointer.upped
		&& currentlyHeldRigidBodyIndex == i){
			rigidBody_p->velocity = cameraPos + cameraDirection * holdingDistance - rigidBody_p->pos;
			if(getMagVec3f(rigidBody_p->velocity) > 0.01){
				rigidBody_p->velocity = normalize(rigidBody_p->velocity) * 0.3;
			}
		}

	}

	if(!Engine_pointer.down){
		currentlyHeldRigidBodyIndex = -1;
	}

	//move rigid bodies
	for(int i = 0; i < game_p->rigidBodies.size(); i++){
		
		RigidBody *rigidBody_p = game_p->rigidBodies[i];

		float resistance = 0.97;

		rigidBody_p->velocity.y += -0.01;

		rigidBody_p->velocity *= resistance;
		rigidBody_p->angularVelocity *= resistance;

		//Vec3f lastPos = rigidBody_p->pos;
		Vec4f lastOrientation = rigidBody_p->orientation;

		rigidBody_p->pos += rigidBody_p->velocity;
		if(getMagVec3f(rigidBody_p->angularVelocity) > 0.0){
			rigidBody_p->orientation = mulQuaternions(getQuaternion(rigidBody_p->angularVelocity, getMagVec3f(rigidBody_p->angularVelocity)), rigidBody_p->orientation);
		}

		rigidBody_p->orientation = normalize(rigidBody_p->orientation);

		//calculate bounding box
		float scale = 1.0;
		Mat4f modelMatrix = getIdentityMat4f();
		modelMatrix *= getScalingMat4f(scale);
		modelMatrix *= getQuaternionMat4f(rigidBody_p->orientation);
		modelMatrix *= getTranslationMat4f(rigidBody_p->pos);

		PointMesh *pointMesh_p = game_p->pointMeshes[rigidBody_p->triangleMeshIndex];

		for(int j = 0; j < pointMesh_p->n_points; j++){

			Vec3f point = mulVec3fMat4f(pointMesh_p->points[j], modelMatrix, 1.0);

			if(j == 0){
				rigidBody_p->boundingBox.pos = point;
				rigidBody_p->boundingBox.size = point;
			}

			rigidBody_p->boundingBox.pos.x = fmin(rigidBody_p->boundingBox.pos.x, point.x);
			rigidBody_p->boundingBox.pos.y = fmin(rigidBody_p->boundingBox.pos.y, point.y);
			rigidBody_p->boundingBox.pos.z = fmin(rigidBody_p->boundingBox.pos.z, point.z);

			rigidBody_p->boundingBox.size.x = fmax(rigidBody_p->boundingBox.size.x, point.x);
			rigidBody_p->boundingBox.size.y = fmax(rigidBody_p->boundingBox.size.y, point.y);
			rigidBody_p->boundingBox.size.z = fmax(rigidBody_p->boundingBox.size.z, point.z);

		}

		rigidBody_p->boundingBox.size = rigidBody_p->boundingBox.size - rigidBody_p->boundingBox.pos;

	}

	//check rigid body collisions
	std::vector<Collision> rigidBodyObstacleCollisions;
	std::vector<Collision> rigidBodyRigidBodyCollisions;

	int n_triangleChecks = 0;

	for(int i = 0; i < game_p->rigidBodies.size(); i++){
		
		RigidBody *rigidBody_p = game_p->rigidBodies[i];
		TriangleMesh *triangleMesh_p = game_p->triangleMeshes[rigidBody_p->triangleMeshIndex];
		PointMesh *pointMesh_p = game_p->pointMeshes[rigidBody_p->triangleMeshIndex];

		//calculate model matrix
		float scale = 1.0;
		Mat4f modelMatrix = getIdentityMat4f();
		modelMatrix *= getScalingMat4f(scale);
		modelMatrix *= getQuaternionMat4f(rigidBody_p->orientation);
		modelMatrix *= getTranslationMat4f(rigidBody_p->pos);

		//check collision vs obstacles
		for(int j = 0; j < game_p->obstacles.size(); j++){

			Obstacle *obstacle_p = game_p->obstacles[j];

			if(!checkBoxBoxCollision(rigidBody_p->boundingBox, obstacle_p->boundingBox)){
				continue;
			}

			Mat4f obstacleMatrix = getIdentityMat4f();
			obstacleMatrix *= getScalingMat4f(obstacle_p->scale);
			obstacleMatrix *= getTranslationMat4f(obstacle_p->pos);

			TriangleMesh *obstacleTriangleMesh_p = game_p->triangleMeshes[obstacle_p->triangleMeshIndex];
			PointMesh *obstaclePointMesh_p = game_p->pointMeshes[obstacle_p->triangleMeshIndex];

			Collision collision;
			collision.pos = getVec3f(0.0, 0.0, 0.0);
			collision.normal = getVec3f(0.0, 0.0, 0.0);
			int n_hits = 0;

			//float distance = getMagVec3f(rigidBody_p->pos - obstacle_p->pos);

			for(int k = 0; k < obstacleTriangleMesh_p->n_triangles; k++){

				Vec3f a1 = mulVec3fMat4f(obstacleTriangleMesh_p->triangles[k * 3 + 0], obstacleMatrix, 1.0);
				Vec3f a2 = mulVec3fMat4f(obstacleTriangleMesh_p->triangles[k * 3 + 1], obstacleMatrix, 1.0);
				Vec3f a3 = mulVec3fMat4f(obstacleTriangleMesh_p->triangles[k * 3 + 2], obstacleMatrix, 1.0);
				Vec3f aNormal = normalize(cross(a2 - a1, a3 - a1));

				float aRadius = max(dot(a1 - a2, a1 - a2), dot(a1 - a3, a1 - a3));

				if(dot(a1 - rigidBody_p->pos, a1 - rigidBody_p->pos) > aRadius + 2.0 * 2.0){
					continue;
				}

				for(int l = 0; l < triangleMesh_p->n_triangles; l++){

					Vec3f b1 = mulVec3fMat4f(triangleMesh_p->triangles[l * 3 + 0], modelMatrix, 1.0);
					Vec3f b2 = mulVec3fMat4f(triangleMesh_p->triangles[l * 3 + 1], modelMatrix, 1.0);
					Vec3f b3 = mulVec3fMat4f(triangleMesh_p->triangles[l * 3 + 2], modelMatrix, 1.0);

					n_triangleChecks++;

					Vec3f collisionPoint;

					if(checkTriangleTriangleCollisionVec3f(a1, a2, a3, b1, b2, b3, &collisionPoint)){

						collision.pos += collisionPoint;

						if(dot(rigidBody_p->velocity * -1.0, aNormal) > dot(rigidBody_p->velocity * -1.0, collision.normal)
						|| getMagVec3f(collision.normal) < 0.01){
							collision.normal = aNormal;
						}

						n_hits++;

					}

				}
				
			}

			if(n_hits > 0){
				collision.pos /= (float)n_hits;
				collision.normal = normalize(collision.normal);
				collision.index1 = i;
				collision.index2 = j;
				rigidBodyObstacleCollisions.push_back(collision);
			}

		}

		//check collision vs other rigid bodies
		for(int j = 0; j < game_p->rigidBodies.size(); j++){

			RigidBody *rigidBody2_p = game_p->rigidBodies[j];
			TriangleMesh *triangleMesh2_p = game_p->triangleMeshes[rigidBody2_p->triangleMeshIndex];

			float scale = 1.0;
			Mat4f modelMatrix2 = getIdentityMat4f();
			modelMatrix2 *= getScalingMat4f(scale);
			modelMatrix2 *= getQuaternionMat4f(rigidBody2_p->orientation);
			modelMatrix2 *= getTranslationMat4f(rigidBody2_p->pos);

			Collision collision;
			collision.pos = getVec3f(0.0, 0.0, 0.0);
			collision.normal = getVec3f(0.0, 0.0, 0.0);
			int n_hits = 0;

			if(i != j
			&& checkBoxBoxCollision(rigidBody_p->boundingBox, rigidBody2_p->boundingBox)){
				for(int k = 0; k < triangleMesh2_p->n_triangles; k++){

					Vec3f a1 = mulVec3fMat4f(triangleMesh2_p->triangles[k * 3 + 0], modelMatrix2, 1.0);
					Vec3f a2 = mulVec3fMat4f(triangleMesh2_p->triangles[k * 3 + 1], modelMatrix2, 1.0);
					Vec3f a3 = mulVec3fMat4f(triangleMesh2_p->triangles[k * 3 + 2], modelMatrix2, 1.0);
					Vec3f aNormal = normalize(cross(a2 - a1, a3 - a1));

					float aRadius = max(dot(a1 - a2, a1 - a2), dot(a1 - a3, a1 - a3));

					if(dot(a1 - rigidBody_p->pos, a1 - rigidBody_p->pos) > aRadius + 2.0 * 2.0){
						continue;
					}

					for(int l = 0; l < triangleMesh_p->n_triangles; l++){

						Vec3f b1 = mulVec3fMat4f(triangleMesh_p->triangles[l * 3 + 0], modelMatrix, 1.0);
						Vec3f b2 = mulVec3fMat4f(triangleMesh_p->triangles[l * 3 + 1], modelMatrix, 1.0);
						Vec3f b3 = mulVec3fMat4f(triangleMesh_p->triangles[l * 3 + 2], modelMatrix, 1.0);

						n_triangleChecks++;

						Vec3f collisionPoint;

						if(checkTriangleTriangleCollisionVec3f(a1, a2, a3, b1, b2, b3, &collisionPoint)){

							collision.pos += collisionPoint;

							if(dot(rigidBody_p->velocity * -1.0, aNormal) > dot(rigidBody_p->velocity * -1.0, collision.normal)
							|| getMagVec3f(collision.normal) < 0.01){
								collision.normal = aNormal;
							}

							n_hits++;

						}
					
					}

				}
			}

			if(n_hits > 0){
				collision.pos /= (float)n_hits;
				collision.normal = normalize(collision.normal);
				collision.index1 = i;
				collision.index2 = j;
				rigidBodyRigidBodyCollisions.push_back(collision);
			
			}
			
		}

	}

	//printf("triangle checks: %i\n", n_triangleChecks);

	//resolve rigid body collisions
	for(int i = 0; i < game_p->rigidBodies.size(); i++){
		
		RigidBody *rigidBody_p = game_p->rigidBodies[i];
			
		float elasticity = 0.1;
		float torqueFactor = 0.5;

		Vec3f collisionNormal = getVec3f(0.0, 0.0, 0.0);
		Vec3f collisionPoint = getVec3f(0.0, 0.0, 0.0);
		int n_hits = 0;
		
		for(int j = 0; j < rigidBodyObstacleCollisions.size(); j++){

			Collision *collision_p = &rigidBodyObstacleCollisions[j];

			if(collision_p->index1 == i){
				collisionNormal += collision_p->normal;
				collisionPoint += collision_p->pos;
				n_hits++;
			}

		}

		Vec3f rigidBodyForces = getVec3f(0.0, 0.0, 0.0);
		Vec3f rigidBodyTorque = getVec3f(0.0, 0.0, 0.0);
		bool hitByRigidBody = false;

		for(int j = 0; j < rigidBodyRigidBodyCollisions.size(); j++){

			Collision *collision_p = &rigidBodyRigidBodyCollisions[j];

			Vec3f collisionDiff = rigidBody_p->pos - collision_p->pos;

			if(collision_p->index1 == i){

				RigidBody *rigidBody2_p = game_p->rigidBodies[collision_p->index2];
				Vec3f force = collision_p->normal * getMagVec3f(rigidBody2_p->velocity) * elasticity;
				Vec3f torque = cross(force, collisionDiff);

				rigidBodyForces += force;
				//rigidBodyTorque += getVec3f(0.0, 1.0, 0.0);
				rigidBodyTorque += torque;
				//hitByRigidBody = true;

				collisionNormal += collision_p->normal;
				collisionPoint += collision_p->pos;
				n_hits++;

			}

		}

		if(n_hits > 0){

			//rigidBody_p->pos += collisionNormal * fabs(dot(rigidBody_p->velocity, collisionNormal));
			rigidBody_p->pos = rigidBody_p->lastPos;
			rigidBody_p->orientation = rigidBody_p->lastOrientation;
			//rigidBody_p->angularVelocity *= 0.9;
			//rigidBody_p->velocity *= 0.9;

			collisionPoint /= (float)n_hits;
			collisionNormal = normalize(collisionNormal);

			Vec3f collisionDiff = rigidBody_p->pos - collisionPoint;
			Vec3f force = collisionNormal * getMagVec3f(rigidBody_p->velocity) * (1.0 + elasticity);
			Vec3f torque = cross(force, collisionDiff);

			if(dot(rigidBody_p->lastTorque, torque) < 0.0
			&& dot(rigidBody_p->velocity, collisionNormal) < 0.0
			&& rigidBody_p->framesSinceHit < 2){
				torque *= 0.1;
				rigidBody_p->angularVelocity *= 0.1;
				rigidBody_p->velocity *= 0.1;
				force *= 0.1;
			}

			rigidBody_p->angularVelocity += torque * torqueFactor;

			rigidBody_p->velocity += cross(torque, collisionDiff) * torqueFactor;
			rigidBody_p->velocity += force;

			rigidBody_p->lastTorque = torque;
			rigidBody_p->framesSinceHit = 0;
		
		}

		if(hitByRigidBody){
			//rigidBody_p->pos = rigidBody_p->lastPos;
			//rigidBody_p->orientation = rigidBody_p->lastOrientation;
			rigidBody_p->velocity += rigidBodyForces;
			//rigidBody_p->angularVelocity += rigidBodyTorque * torqueFactor;
		}

		rigidBody_p->framesSinceHit++;

	}
	*/

	//PERFORMANCE TEST CAMERA
	//game_p->player.pos = getVec3f(90.0, 5.0, 23.0);
	//cameraDirection = getVec3f(-0.9, -0.33, 0.26);

	//update camera position
	if(!game_p->dead){
		Player *player_p = World_getPlayerPointerByConnectionID(&game_p->world, game_p->client.connectionID);

		cameraPos = player_p->pos;
		cameraPos.y += player_p->height;
	}

	//update blood particles
	for(int i = 0; i < game_p->bloodParticles.size(); i++){
		
		Particle *particle_p = &game_p->bloodParticles[i];

		particle_p->velocity.y -= BLOOD_PARTICLE_GRAVITY;

		particle_p->pos += particle_p->velocity;

		//check distance to ground to see if further checks are necessary
		if(particle_p->pos.y <= game_p->terrainMaxHeight){

			//check collision with grass
			Vec4f hitPosition;
			bool hitGrass = false;

			for(int j = 0; j < game_p->grassPositions.size(); j++){
				if(square(game_p->grassPositions[j].x - particle_p->pos.x) + square(game_p->grassPositions[j].z - particle_p->pos.z) < 1.0
				&& particle_p->pos.y < game_p->grassPositions[j].y - 1.2 + 2.0 * (game_p->grassPositions[j].w / 100.0)){
					hitGrass = true;
					hitPosition = game_p->grassPositions[j];
					break;
				}
			}

			if(hitGrass){

				Vec2f paintMapPos = getVec2f(particle_p->pos.x * PAINT_MAP_WIDTH, particle_p->pos.z * PAINT_MAP_HEIGHT) / PAINT_MAP_SCALE;

				int index = (int)paintMapPos.y * PAINT_MAP_WIDTH + (int)paintMapPos.x;

				game_p->paintMap[index] = (unsigned char)(255.0 * (fmax(floor(hitPosition.w) / 100.0 - 0.2 + getRandom() * 0.2, 0.0)));

				game_p->bloodParticles.erase(game_p->bloodParticles.begin() + i);
				i--;
				continue;
			
			}

			//check collision with ground
			TriangleMesh *triangleMesh_p = World_getTriangleMeshPointerByName(&game_p->world, "terrain");

			Vec3f intersectionPoint;
			checkClosestLineTriangleMeshIntersection(particle_p->pos, getVec3f(0.0, -1.0, 0.0), *triangleMesh_p, getVec3f(0.0, 0.0, 0.0), TERRAIN_SCALE, &intersectionPoint, NULL);

			if(particle_p->pos.y < intersectionPoint.y){

				for(int x = 0; x < 3; x++){
					for(int z = 0; z < 3; z++){

						if(getRandom() > 0.3){
							continue;
						}

						int terrainTextureIndex = TERRAIN_TEXTURE_WIDTH * (int)((float)TERRAIN_TEXTURE_WIDTH * particle_p->pos.z / TERRAIN_SCALE + z) + (int)((float)TERRAIN_TEXTURE_WIDTH * particle_p->pos.x / TERRAIN_SCALE + x);

						if(terrainTextureIndex > 0
						&& terrainTextureIndex < TERRAIN_TEXTURE_WIDTH * TERRAIN_TEXTURE_WIDTH){
							game_p->terrainTextureData[terrainTextureIndex * 4 + 0] = (unsigned char)(255.0 * 0.9);
							game_p->terrainTextureData[terrainTextureIndex * 4 + 1] = 0;
							game_p->terrainTextureData[terrainTextureIndex * 4 + 2] = 0;
							game_p->terrainTextureData[terrainTextureIndex * 4 + 3] = 255;
						}

					}
				}

			}

		}

		if(particle_p->pos.y < 0.0){
			game_p->bloodParticles.erase(game_p->bloodParticles.begin() + i);
			i--;
			continue;
		}

	}

	//update grass particles
	for(int i = 0; i < game_p->grassParticles.size(); i++){

		Particle *particle_p = &game_p->grassParticles[i];

		particle_p->velocity.y -= 0.01;

		particle_p->pos += particle_p->velocity;

		particle_p->color.w -= 0.04;

		if(particle_p->color.w < 0.0){
			game_p->grassParticles.erase(game_p->grassParticles.begin() + i);
			i--;
			continue;
		}
	
	}

	//update bullet traces
	for(int i = 0; i < game_p->bulletTraces.size(); i++){

		BulletTrace *bulletTrace_p = &game_p->bulletTraces[i];

		bulletTrace_p->alpha -= 0.03;

		if(bulletTrace_p->alpha < 0.0){
			game_p->bulletTraces.erase(game_p->bulletTraces.begin() + i);
			i--;
			continue;
		}
	
	}

	//update sword
	{
		if(swingAngle < SWING_ANGLE_RANGE){

			TriangleMesh *triangleMesh_p = World_getTriangleMeshPointerByName(&game_p->world, "quad");

			Mat4f swordMatrix = getSwordMatrix(cameraPos, cameraDirection, swingAngle);

			for(int i = 0; i < triangleMesh_p->n_triangles; i++){

				Vec3f p1 = mulVec3fMat4f(triangleMesh_p->triangles[i * 3 + 0], swordMatrix, 1.0);
				Vec3f p2 = mulVec3fMat4f(triangleMesh_p->triangles[i * 3 + 1], swordMatrix, 1.0);
				Vec3f p3 = mulVec3fMat4f(triangleMesh_p->triangles[i * 3 + 2], swordMatrix, 1.0);

				for(int j = 0; j < game_p->grassPositions.size(); j++){

					if(square(game_p->grassPositions[j].x - cameraPos.x) + square(game_p->grassPositions[j].z - cameraPos.z) > 10.0){
						continue;
					}

					Vec3f intersectionPoint;

					Vec3f checkPos = getVec3f(game_p->grassPositions[j].x, 0.0, game_p->grassPositions[j].z);

					if(checkLineToTriangleIntersectionVec3f(checkPos, checkPos + getVec3f(0.0, 1.0, 0.0), p1, p2, p3, &intersectionPoint)){

						Vec4f *position_p = &game_p->grassPositions[j];

						float grassHeight = floor(position_p->w);
						float cutHeight = fmax(fmin((intersectionPoint.y - (position_p->y - 1.0)) * 100.0 / 2.0, 100.0), 0.0);
						position_p->w = position_p->w - grassHeight + fmin(cutHeight, grassHeight);

						//add grass particles
						if(grassHeight > cutHeight){
							for(int j = 0; j < 3; j++){

								float height = (grassHeight - cutHeight) / (100.0);
								float posY = -1.0 + height + (cutHeight / 100.0) * 2.0;

								Particle particle;
								particle.pos = getVec3f(position_p->x, position_p->y + posY, position_p->z);
								particle.velocity = getVec3f((getRandom() - 0.5) * 0.01, 0.08 + getRandom() * 0.05, (getRandom() - 0.5) * 0.01);
								particle.scale = getVec3f(1.0, height, 1.0);
								particle.orientation = getQuaternion(getVec3f(0.0, 1.0, 0.0), -(float)j * M_PI / 3.0 + 0.0);
								particle.color = getVec4f(1.0, 1.0, 1.0, 1.0);
								particle.textureY = 1.0 - grassHeight / 100.0;
								particle.textureSizeY = height;

								game_p->grassParticles.push_back(particle);

							}
						}

					}

				}

			}

			swingAngle += 0.09;
			//swingAngle += 0.012;
		}
	}

	//sort grass positions by depth
	int n_buckets = 200;
	int bucketSizes[n_buckets];
	int bucketOffsets[n_buckets];
	int bucketCounts[n_buckets];
	memset(bucketSizes, 0, sizeof(int) * n_buckets);
	memset(bucketOffsets, 0, sizeof(int) * n_buckets);
	memset(bucketCounts, 0, sizeof(int) * n_buckets);

	unsigned char depths[game_p->grassPositions.size()];
	memset(depths, 0, sizeof(unsigned char) * game_p->grassPositions.size());

	Mat4f cameraMatrix = getLookAtMat4f(cameraPos, cameraDirection);

	for(int i = 0; i < game_p->grassPositions.size(); i++){

		Vec4f projectedPos = cameraMatrix * game_p->grassPositions[i];

		unsigned char depth = (unsigned char)fabs(projectedPos.z);

		depths[i] = depth * (depth < n_buckets);

		bucketSizes[depths[i]]++;

	}

	for(int i = 1; i < n_buckets; i++){
		bucketOffsets[i] = bucketOffsets[i - 1] + bucketSizes[i - 1];
	}

	for(int i = 0; i < game_p->grassPositions.size(); i++){
		game_p->sortedGrassPositions[bucketOffsets[depths[i]] + bucketCounts[depths[i]]] = game_p->grassPositions[i];
		bucketCounts[depths[i]]++;
	}

	//sort leaf transformations by depth
	for(int i = 0; i < game_p->trees.size(); i++){

		Tree *tree_p = &game_p->trees[i];

		int n_buckets = 200;
		int bucketSizes[n_buckets];
		int bucketOffsets[n_buckets];
		int bucketCounts[n_buckets];
		memset(bucketSizes, 0, sizeof(int) * n_buckets);
		memset(bucketOffsets, 0, sizeof(int) * n_buckets);
		memset(bucketCounts, 0, sizeof(int) * n_buckets);

		unsigned char depths[tree_p->leafTransformations.size()];
		memset(depths, 0, sizeof(unsigned char) * tree_p->leafTransformations.size());
		
		for(int j = 0; j < tree_p->leafTransformations.size(); j++){
			
			Vec4f projectedPos = getVec4f(tree_p->leafTransformations[i][0][3], tree_p->leafTransformations[i][1][3], tree_p->leafTransformations[i][2][3], tree_p->leafTransformations[i][3][3]);
			projectedPos = cameraMatrix * projectedPos;

			unsigned char depth = (unsigned char)fabs(projectedPos.z);
			//Vec4f_log(pos);
			//plog(tree_p->leafTransformations[j]);

		}

		//printf("tree :)\n");

	}

}

void drawLevelState(Game *game_p){

#ifndef RUN_OFFLINE
	if(!game_p->client.receivedGameState){
		return;
	}
#endif

	glBindTexture(GL_TEXTURE_2D, game_p->paintMapTexture.ID);
	glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, PAINT_MAP_WIDTH, PAINT_MAP_HEIGHT, GL_RED, GL_UNSIGNED_BYTE, game_p->paintMap);

	{
		Texture *texture_p = Game_getTexturePointerByName(game_p, "terrain");
		glBindTexture(GL_TEXTURE_2D, texture_p->ID);
		glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, TERRAIN_TEXTURE_WIDTH, TERRAIN_TEXTURE_WIDTH, GL_RGBA, GL_UNSIGNED_BYTE, game_p->terrainTextureData);
	}

	TextureBuffer_update(&game_p->grassPositionsTextureBuffer, 0, game_p->grassPositions.size() * sizeof(Vec4f), game_p->sortedGrassPositions);

	bool renderFromLightPerspective = false;
	for(int renderStage = 0; renderStage < N_RENDER_STAGES; renderStage++){

		if(renderStage == RENDER_STAGE_BIG_SHADOWS){
			glViewport(0.0, 0.0, BIG_SHADOW_MAP_WIDTH, BIG_SHADOW_MAP_HEIGHT);
			glBindFramebuffer(GL_FRAMEBUFFER, game_p->bigShadowMapFBO);
			//glColorMask(true, true, true, true);
			glColorMask(false, false, false, false);
			glDepthMask(true);
			//glViewport(0.0, 0.0, Engine_clientWidth, Engine_clientHeight);
			//glBindFramebuffer(GL_FRAMEBUFFER, 0);  
		}else{
			//continue;
		}

		if(renderStage == RENDER_STAGE_GRASS_SHADOWS){
			glViewport(0.0, 0.0, GRASS_SHADOW_MAP_WIDTH, GRASS_SHADOW_MAP_HEIGHT);
			glBindFramebuffer(GL_FRAMEBUFFER, game_p->grassShadowMapFBO);
			glColorMask(false, false, false, false);
			glDepthMask(true);
		}
		if(renderStage == RENDER_STAGE_SHADOWS){
			glViewport(0.0, 0.0, SHADOW_MAP_WIDTH, SHADOW_MAP_HEIGHT);
			glBindFramebuffer(GL_FRAMEBUFFER, game_p->shadowMapFBO);
			glColorMask(false, false, false, false);
			//glColorMask(true, true, true, true);
			glDepthMask(true);
			//glViewport(0.0, 0.0, Engine_clientWidth, Engine_clientHeight);
			//glBindFramebuffer(GL_FRAMEBUFFER, 0);  
		}else{
			//continue;
		}
		if(renderStage == RENDER_STAGE_SCENE){
			glViewport(0.0, 0.0, Engine_clientWidth, Engine_clientHeight);
			glBindFramebuffer(GL_FRAMEBUFFER, 0);  
			glColorMask(true, true, true, true);
			//glDepthMask(false);
		}

		//printf("%i\n", renderStage);

		glClearColor(0.5, 0.7, 0.9, 1.0);
		glClearDepth(1.0);

		if(renderStage == RENDER_STAGE_SCENE){
			glClear(GL_COLOR_BUFFER_BIT);
			glClear(GL_DEPTH_BUFFER_BIT);
			//glClear(GL_DEPTH_BUFFER_BIT);
		}else{
			//printf("cleared buffer\n");
			glClear(GL_COLOR_BUFFER_BIT);
			glClear(GL_DEPTH_BUFFER_BIT);
		}


		Mat4f viewMatrix = getLookAtMat4f(cameraPos, cameraDirection);
		viewMatrix *= getPerspectiveMat4f(fov, (float)Engine_clientWidth / (float)Engine_clientHeight, farPlane, nearPlane);

		//Mat4f perspectiveMatrix = getPerspectiveMat4f(fov, (float)Engine_clientWidth / (float)Engine_clientHeight, farPlane, nearPlane);

		//cameraMatrix *= perspectiveMatrix;

		//perspectiveMatrix = getIdentityMat4f();

		Mat4f lightViewMatrix = getTranslationMat4f(round(-cameraPos.x), round(-cameraPos.y), round(-cameraPos.z));
		lightViewMatrix *= getTranslationMat4f(getVec3f(lightDirection.x, 0.0, lightDirection.z) * lightPos.y);
		lightViewMatrix *= getTranslationMat4f(getVec3f(round(-cameraDirection.x * shadowMapScale), 0.0, round(-cameraDirection.z * shadowMapScale)));
		lightViewMatrix *= getLookAtMat4f(lightPos, lightDirection);
		lightViewMatrix *= getOrthographicMat4f(shadowMapScale, 1.0, farPlane, nearPlane);

		//Mat4f lightPerspectiveMatrix = getOrthographicMat4f(shadowMapScale, 1.0, farPlane, nearPlane);

		//lightCameraMatrix *= lightPerspectiveMatrix;
		//lightPerspectiveMatrix = getIdentityMat4f();

		Mat4f bigLightViewMatrix = getTranslationMat4f(getVec3f(-17.0, 0.0, -22.0));
		bigLightViewMatrix *= getLookAtMat4f(lightPos, lightDirection);
		bigLightViewMatrix *= getOrthographicMat4f(bigShadowMapScale, 1.0, farPlane, nearPlane);

		//Mat4f bigLightPerspectiveMatrix = getOrthographicMat4f(bigShadowMapScale, 1.0, farPlane, nearPlane);

		//bigLightCameraMatrix *= bigLightPerspectiveMatrix;
		//bigLightPerspectiveMatrix = getIdentityMat4f();

		if(renderStage == RENDER_STAGE_BIG_SHADOWS){
			viewMatrix = bigLightViewMatrix;
			//cameraMatrix = bigLightCameraMatrix;
			//perspectiveMatrix = bigLightPerspectiveMatrix;
		}

		if(renderStage == RENDER_STAGE_GRASS_SHADOWS
		|| renderStage == RENDER_STAGE_SHADOWS){
			viewMatrix = lightViewMatrix;
			//cameraMatrix = lightCameraMatrix;
			//perspectiveMatrix = lightPerspectiveMatrix;
		}

		//draw opaque meshes with standard shader
		if(renderStage == RENDER_STAGE_BIG_SHADOWS
		|| renderStage == RENDER_STAGE_SHADOWS
		|| renderStage == RENDER_STAGE_SCENE){

			Shader *shader_p = Game_getShaderPointerByName(game_p, "model");
			if(renderStage == RENDER_STAGE_SHADOWS
			|| renderStage == RENDER_STAGE_BIG_SHADOWS){
				shader_p = Game_getShaderPointerByName(game_p, "model-shadow");
			}

			glUseProgram(shader_p->ID);

			GL3D_uniformFloat(shader_p->ID, "grassShadowStrength", GRASS_SHADOW_STRENGTH);
			GL3D_uniformFloat(shader_p->ID, "ambientLightFactor", 0.3);
			GL3D_uniformFloat(shader_p->ID, "diffuseLightFactor", 0.7);

			GL3D_uniformTexture(shader_p->ID, "shadowMapDepthTexture", 1, game_p->shadowMapDepthTexture.ID);
			GL3D_uniformTexture(shader_p->ID, "grassShadowMapDepthTexture", 2, game_p->grassShadowMapDepthTexture.ID);
			GL3D_uniformTexture(shader_p->ID, "bigShadowMapDepthTexture", 3, game_p->bigShadowMapDepthTexture.ID);

			GL3D_uniformMat4f(shader_p->ID, "viewMatrix", viewMatrix);
			GL3D_uniformMat4f(shader_p->ID, "lightViewMatrix", lightViewMatrix);
			GL3D_uniformMat4f(shader_p->ID, "bigLightViewMatrix", bigLightViewMatrix);

			//draw obstacles
			for(int i = 0; i < game_p->world.obstacles.size(); i++){

				Obstacle *obstacle_p = &game_p->world.obstacles[i];

				Mat4f modelMatrix = getModelMatrix(obstacle_p->pos, getVec3f(obstacle_p->scale), IDENTITY_QUATERNION);

				Model *model_p = &game_p->models[obstacle_p->modelIndex];
				Texture *texture_p = &game_p->textures[obstacle_p->textureIndex];

				glBindBuffer(GL_ARRAY_BUFFER, model_p->VBO);
				glBindVertexArray(model_p->VAO);

				GL3D_uniformTexture(shader_p->ID, "colorTexture", 0, texture_p->ID);

				GL3D_uniformMat4f(shader_p->ID, "modelMatrix", modelMatrix);

				GL3D_uniformVec4f(shader_p->ID, "inputColor", obstacle_p->color);

				glDrawArrays(GL_TRIANGLES, 0, model_p->numberOfTriangles * 3);

			}

			//draw blood particles
			for(int i = 0; i < game_p->bloodParticles.size(); i++){

				Particle *particle_p = &game_p->bloodParticles[i];

				Mat4f modelMatrix = getModelMatrix(particle_p->pos, particle_p->scale, IDENTITY_QUATERNION);

				Model *model_p = Game_getModelPointerByName(game_p, "cube");
				Texture *texture_p = Game_getTexturePointerByName(game_p, "blank");

				glBindBuffer(GL_ARRAY_BUFFER, model_p->VBO);
				glBindVertexArray(model_p->VAO);

				GL3D_uniformTexture(shader_p->ID, "colorTexture", 0, texture_p->ID);

				GL3D_uniformMat4f(shader_p->ID, "modelMatrix", modelMatrix);

				GL3D_uniformVec4f(shader_p->ID, "inputColor", getVec4f(0.9, 0.0, 0.0, 1.0));

				glDrawArrays(GL_TRIANGLES, 0, model_p->numberOfTriangles * 3);

			}

			//draw player guns
			for(int i = 0; i < game_p->world.players.size(); i++){
				
				Player *player_p = &game_p->world.players[i];

				if(player_p->connectionID == game_p->client.connectionID){
					//continue;
				}

				float horizontalAngle = atan2(player_p->direction.x, player_p->direction.z);
				float verticalAngle = asin(player_p->direction.y);

				//verticalAngle = windTime;

				Vec4f orientation = IDENTITY_QUATERNION;
				orientation = mulQuaternions(getQuaternion(getVec3f(0.0, 1.0, 0.0), M_PI / 2.0), orientation);
				orientation = mulQuaternions(getQuaternion(getVec3f(1.0, 0.0, 0.0), -verticalAngle), orientation);
				orientation = mulQuaternions(getQuaternion(getVec3f(0.0, 1.0, 0.0), horizontalAngle), orientation);

				Vec3f cameraRight = cross(player_p->direction, getVec3f(0.0, 1.0, 0.0));
				Vec3f cameraUp = normalize(cross(cameraRight, player_p->direction));

				Vec3f pos = player_p->pos + getVec3f(0.0, player_p->height, 0.0) + player_p->direction * 1.0 - cameraUp * 0.3;

				Mat4f modelMatrix = getModelMatrix(pos, getVec3f(GUN_SCALE), orientation);

				Model *model_p = Game_getModelPointerByName(game_p, "gun");
				Texture *texture_p = Game_getTexturePointerByName(game_p, "blank");

				glBindBuffer(GL_ARRAY_BUFFER, model_p->VBO);
				glBindVertexArray(model_p->VAO);

				GL3D_uniformTexture(shader_p->ID, "colorTexture", 0, texture_p->ID);

				GL3D_uniformMat4f(shader_p->ID, "modelMatrix", modelMatrix);

				GL3D_uniformVec4f(shader_p->ID, "inputColor", getVec4f(0.7, 0.7, 0.7, 1.0));

				glDrawArrays(GL_TRIANGLES, 0, model_p->numberOfTriangles * 3);

			}

		}

		//draw grass
		if((renderStage == RENDER_STAGE_GRASS_SHADOWS
		|| renderStage == RENDER_STAGE_SCENE) && true){
			glDisable(GL_CULL_FACE);

			Vec3f pos = getVec3f(0.0, 4.0, 0.0);
			float scale = 1.0;

			Shader *shader_p = Game_getShaderPointerByName(game_p, "grass");
			if(renderStage == RENDER_STAGE_GRASS_SHADOWS){
				shader_p = Game_getShaderPointerByName(game_p, "grass-shadow");
			}

			glUseProgram(shader_p->ID);

			Texture *texture_p = Game_getTexturePointerByName(game_p, "grass");
			Texture *alphaTexture_p = Game_getTexturePointerByName(game_p, "grass-alpha");
			//Texture *texture_p = Game_getTexturePointerByName(game_p, "small-grass");
			//Texture *alphaTexture_p = Game_getTexturePointerByName(game_p, "small-grass-alpha");

			Model *model_p = Game_getModelPointerByName(game_p, "quad");

			glBindBuffer(GL_ARRAY_BUFFER, model_p->VBO);
			glBindVertexArray(model_p->VAO);

			GL3D_uniformTexture(shader_p->ID, "colorTexture", 0, texture_p->ID);
			GL3D_uniformTexture(shader_p->ID, "alphaTexture", 1, alphaTexture_p->ID);
			GL3D_uniformTextureBuffer(shader_p->ID, "grassPositions", 2, game_p->grassPositionsTextureBuffer.TB);
			GL3D_uniformTexture(shader_p->ID, "shadowMapDepthTexture", 3, game_p->shadowMapDepthTexture.ID);
			GL3D_uniformTexture(shader_p->ID, "grassShadowMapDepthTexture", 4, game_p->grassShadowMapDepthTexture.ID);
			GL3D_uniformTexture(shader_p->ID, "bigShadowMapDepthTexture", 5, game_p->bigShadowMapDepthTexture.ID);
			GL3D_uniformTexture(shader_p->ID, "paintMapTexture", 6, game_p->paintMapTexture.ID);

			GL3D_uniformMat4f(shader_p->ID, "viewMatrix", viewMatrix);
			GL3D_uniformMat4f(shader_p->ID, "lightViewMatrix", lightViewMatrix);
			GL3D_uniformMat4f(shader_p->ID, "bigLightViewMatrix", bigLightViewMatrix);

			GL3D_uniformFloat(shader_p->ID, "windTime", windTime);

			GL3D_uniformFloat(shader_p->ID, "shadowStrength", 0.3);
			GL3D_uniformFloat(shader_p->ID, "grassShadowStrength", GRASS_SHADOW_STRENGTH);

			GL3D_uniformInt(shader_p->ID, "n_grassPositions", game_p->grassPositions.size());

			Vec2f cameraPos2 = getVec2f(cameraPos.x, cameraPos.z);
			GL3D_uniformVec2f(shader_p->ID, "cameraPos", cameraPos2);

			float rotation = 0.0;

			Mat2f rotationMatrix1 = getRotationMat2f(rotation);
			Mat2f rotationMatrix2 = getRotationMat2f(rotation + M_PI * (1.0 / 3.0));
			//Mat2f rotationMatrix2 = getRotationMat2f(rotation + M_PI / 2.0);
			Mat2f rotationMatrix3 = getRotationMat2f(rotation + M_PI * (2.0 / 3.0));

			GL3D_uniformMat2f(shader_p->ID, "rotationMatrix", rotationMatrix1);

			glDrawArraysInstanced(GL_TRIANGLES, 0, model_p->numberOfTriangles * 3, game_p->grassPositions.size());

			GL3D_uniformMat2f(shader_p->ID, "rotationMatrix", rotationMatrix2);

			glDrawArraysInstanced(GL_TRIANGLES, 0, model_p->numberOfTriangles * 3, game_p->grassPositions.size());

			GL3D_uniformMat2f(shader_p->ID, "rotationMatrix", rotationMatrix3);

			glDrawArraysInstanced(GL_TRIANGLES, 0, model_p->numberOfTriangles * 3, game_p->grassPositions.size());

			glEnable(GL_CULL_FACE);
		}

		//draw leaves
		if((renderStage == RENDER_STAGE_BIG_SHADOWS
		|| renderStage == RENDER_STAGE_SHADOWS
		|| renderStage == RENDER_STAGE_SCENE) && true){

			glDisable(GL_CULL_FACE);

			for(int i = 0; i < game_p->trees.size(); i++){

				Tree *tree_p = &game_p->trees[i];

				Shader *shader_p = Game_getShaderPointerByName(game_p, "leaf");
				if(renderStage == RENDER_STAGE_SHADOWS
				|| renderStage == RENDER_STAGE_BIG_SHADOWS){
					shader_p = Game_getShaderPointerByName(game_p, "leaf-shadow");
				}

				glUseProgram(shader_p->ID);

				Texture *texture_p = Game_getTexturePointerByName(game_p, "leaves");
				Texture *alphaTexture_p = Game_getTexturePointerByName(game_p, "leaves-alpha");

				Model *model_p = Game_getModelPointerByName(game_p, "quad");

				glBindBuffer(GL_ARRAY_BUFFER, model_p->VBO);
				glBindVertexArray(model_p->VAO);

				GL3D_uniformTexture(shader_p->ID, "colorTexture", 0, texture_p->ID);
				GL3D_uniformTexture(shader_p->ID, "alphaTexture", 1, alphaTexture_p->ID);
				GL3D_uniformTexture(shader_p->ID, "shadowMapDepthTexture", 2, game_p->shadowMapDepthTexture.ID);
				GL3D_uniformTexture(shader_p->ID, "grassShadowMapDepthTexture", 3, game_p->grassShadowMapDepthTexture.ID);
				GL3D_uniformTexture(shader_p->ID, "bigShadowMapDepthTexture", 4, game_p->bigShadowMapDepthTexture.ID);
				GL3D_uniformTextureBuffer(shader_p->ID, "modelTransformationsBuffer", 5, tree_p->leafTransformationsTextureBuffer.TB);

				GL3D_uniformFloat(shader_p->ID, "windTime", windTime);

				GL3D_uniformMat4f(shader_p->ID, "viewMatrix", viewMatrix);
				GL3D_uniformMat4f(shader_p->ID, "lightViewMatrix", lightViewMatrix);
				GL3D_uniformMat4f(shader_p->ID, "bigLightViewMatrix", bigLightViewMatrix);

				glDrawArraysInstanced(GL_TRIANGLES, 0, model_p->numberOfTriangles * 3, tree_p->leafTransformationsTextureBuffer.n_elements);

			}

			glEnable(GL_CULL_FACE);
		
		}

		/*
		float t = (1.0 + sin(gameTime * 0.1)) * 0.5;

		//std::vector<Bone> interpolatedBones = getInterpolatedBones(game_p->boneModels[0].bones, game_p->boneModels[1].bones, t);

		std::vector<Mat4f> bindMatrices = getBindMatricesFromBones(interpolatedBones);

		std::vector<Mat4f> boneTransformations;

		for(int i = 0; i < bindMatrices.size(); i++){
			Mat4f transformation = game_p->boneModels[0].inverseBindMatrices[i];
			transformation *= bindMatrices[i];
			boneTransformations.push_back(transformation);
		}
		*/

		//draw players
		if(renderStage == RENDER_STAGE_SHADOWS
		|| renderStage == RENDER_STAGE_BIG_SHADOWS
		|| renderStage == RENDER_STAGE_SCENE){

			Shader *shader_p = Game_getShaderPointerByName(game_p, "bone");

			glUseProgram(shader_p->ID);

			GL3D_uniformMat4f(shader_p->ID, "viewMatrix", viewMatrix);

			for(int i = 0; i < game_p->world.players.size(); i++){

				Player *player_p = &game_p->world.players[i];

				if(player_p->connectionID == game_p->client.connectionID
				&& renderStage == RENDER_STAGE_SCENE){
					continue;
				}

				Mat4f modelMatrix = getModelMatrix(player_p->pos, getVec3f(PLAYER_SCALE, PLAYER_SCALE * (player_p->height / PLAYER_HEIGHT_STANDING), PLAYER_SCALE), IDENTITY_QUATERNION);

				BoneModel *model_p = Game_getBoneModelPointerByName(game_p, "gubbe");
				BoneRig *boneRig_p = World_getBoneRigPointerByName(&game_p->world, "gubbe");

				std::vector<Bone> newBones = boneRig_p->originBones;

				//player_p->direction = game_p->world.players[0].direction * -1.0;
				//player_p->height = PLAYER_HEIGHT_CROUCHING;

				std::vector<Mat4f> boneTransformations = World_Player_getBoneTransformations(&game_p->world, player_p);

				glBindBuffer(GL_ARRAY_BUFFER, model_p->VBO);
				glBindVertexArray(model_p->VAO);

				GL3D_uniformMat4f(shader_p->ID, "modelMatrix", getTranslationMat4f(getVec3f(0.0, 0.0, 0.0)) * modelMatrix);

				GL3D_uniformVec4f(shader_p->ID, "inputColor", TERRAIN_COLOR);

				GL3D_uniformMat4fArray(shader_p->ID, "boneTransformations", &boneTransformations[0], boneTransformations.size());

				glDrawArrays(GL_TRIANGLES, 0, model_p->n_triangles * 3);

				/*
				Shader *shader_p = Game_getShaderPointerByName(game_p, "model");
				if(renderStage == RENDER_STAGE_SHADOWS
				|| renderStage == RENDER_STAGE_BIG_SHADOWS){
					shader_p = Game_getShaderPointerByName(game_p, "model-shadow");
				}

				glUseProgram(shader_p->ID);

				GL3D_uniformFloat(shader_p->ID, "grassShadowStrength", GRASS_SHADOW_STRENGTH);

				GL3D_uniformTexture(shader_p->ID, "shadowMapDepthTexture", 1, game_p->shadowMapDepthTexture.ID);
				GL3D_uniformTexture(shader_p->ID, "grassShadowMapDepthTexture", 2, game_p->grassShadowMapDepthTexture.ID);
				GL3D_uniformTexture(shader_p->ID, "bigShadowMapDepthTexture", 3, game_p->bigShadowMapDepthTexture.ID);

				GL3D_uniformMat4f(shader_p->ID, "viewMatrix", viewMatrix);
				GL3D_uniformMat4f(shader_p->ID, "lightViewMatrix", lightViewMatrix);
				GL3D_uniformMat4f(shader_p->ID, "bigLightViewMatrix", bigLightViewMatrix);

				std::vector<Mat4f> bindMatrices = getBindMatricesFromBones(newBones, NULL);

				for(int j = 0; j < bindMatrices.size(); j++){

					Model *model_p = Game_getModelPointerByName(game_p, "cube");
					Texture *texture_p = Game_getTexturePointerByName(game_p, "blank");

					glBindBuffer(GL_ARRAY_BUFFER, model_p->VBO);
					glBindVertexArray(model_p->VAO);

					GL3D_uniformTexture(shader_p->ID, "colorTexture", 0, texture_p->ID);

					GL3D_uniformMat4f(shader_p->ID, "modelMatrix", modelMatrix * bindMatrices[j] * getScalingMat4f(getVec3f(0.1)));

					GL3D_uniformVec4f(shader_p->ID, "inputColor", getVec4f(0.9, 0.0, 0.0, 1.0));

					glDrawArrays(GL_TRIANGLES, 0, model_p->numberOfTriangles * 3);

				}
				*/
			
			}

		}

		/*
		//draw bounding boxes
		if(renderStage == 1 && false){

			glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
			glDisable(GL_CULL_FACE);
			glLineWidth(5.0);

			for(int i = 0; i < game_p->boundingBoxes.size(); i++){

				Box *boundingBox_p = game_p->boundingBoxes[i];

				Mat4f modelMatrix = getIdentityMat4f();

				modelMatrix *= getTranslationMat4f(1.0, 1.0, 1.0);

				modelMatrix *= getScalingMat4f(boundingBox_p->size);

				modelMatrix *= getTranslationMat4f(boundingBox_p->pos);

				//unsigned int currentShaderProgram = game_p->modelShader;
				Shader *shader_p = Game_getShaderPointerByName(game_p, "model");

				glUseProgram(shader_p->ID);
				
				Model *model_p = Game_getModelPointerByName(game_p, "cube");

				glBindBuffer(GL_ARRAY_BUFFER, model_p->VBO);
				glBindVertexArray(model_p->VAO);

				GL3D_uniformMat4f(shader_p->ID, "modelMatrix", modelMatrix);
				GL3D_uniformMat4f(shader_p->ID, "perspectiveMatrix", perspectiveMatrix);
				GL3D_uniformMat4f(shader_p->ID, "cameraMatrix", cameraMatrix);

				GL3D_uniformVec4f(shader_p->ID, "inputColor", getVec4f(1.0, 0.0, 0.0, 1.0));

				glDrawArrays(GL_TRIANGLES, 0, model_p->numberOfTriangles * 3);
				
			}

			glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
			glEnable(GL_CULL_FACE);

		}
		*/

		/*
		//draw camera
		if(renderFromLightPerspective){

			float scale = 1.0;

			Mat4f modelMatrix = getIdentityMat4f();

			modelMatrix *= getScalingMat4f(scale);

			modelMatrix *= getTranslationMat4f(cameraPos);

			//unsigned int currentShaderProgram = game_p->modelShader;
			Shader *shader_p = Game_getShaderPointerByName(game_p, "model");

			glUseProgram(shader_p->ID);
			
			Model *model_p = game_p->models[Game_getModelIndexByName(game_p, "cube")];

			glBindBuffer(GL_ARRAY_BUFFER, model_p->VBO);
			glBindVertexArray(model_p->VAO);

			GL3D_uniformTexture(shader_p->ID, "shadowMapDepthTexture", 0, shadowMapDepthTexture.ID);
			GL3D_uniformTexture(shader_p->ID, "grassShadowMapDepthTexture", 1, grassShadowMapDepthTexture.ID);

			GL3D_uniformMat4f(shader_p->ID, "modelMatrix", modelMatrix);
			GL3D_uniformMat4f(shader_p->ID, "perspectiveMatrix", perspectiveMatrix);
			GL3D_uniformMat4f(shader_p->ID, "cameraMatrix", cameraMatrix);
			GL3D_uniformMat4f(shader_p->ID, "lightPerspectiveMatrix", lightPerspectiveMatrix);
			GL3D_uniformMat4f(shader_p->ID, "lightCameraMatrix", lightCameraMatrix);

			GL3D_uniformVec4f(shader_p->ID, "inputColor", getVec4f(1.0, 0.0, 0.0, 1.0));

			glDrawArrays(GL_TRIANGLES, 0, model_p->numberOfTriangles * 3);

		}
		*/

		//draw rigid bodies
		if(renderStage == RENDER_STAGE_SCENE){
			for(int i = 0; i < game_p->rigidBodies.size(); i++){

				RigidBody *rigidBody_p = &game_p->rigidBodies[i];

				float scale = 1.0;

				Mat4f modelMatrix = getModelMatrix(rigidBody_p->pos, getVec3f(1.0), rigidBody_p->orientation);

				Shader *shader_p = Game_getShaderPointerByName(game_p, "model");
				if(renderStage == RENDER_STAGE_SHADOWS
				|| renderStage == RENDER_STAGE_BIG_SHADOWS){
					shader_p = Game_getShaderPointerByName(game_p, "model-shadow");
				}

				glUseProgram(shader_p->ID);
				
				Model *model_p = &game_p->models[rigidBody_p->modelIndex];

				Texture *texture_p = &game_p->textures[rigidBody_p->textureIndex];
				//Texture *alphaTexture_p = game_p->textures[rigidBody_p->alphaTextureIndex];

				glBindBuffer(GL_ARRAY_BUFFER, model_p->VBO);
				glBindVertexArray(model_p->VAO);

				GL3D_uniformFloat(shader_p->ID, "grassShadowStrength", GRASS_SHADOW_STRENGTH);
				GL3D_uniformFloat(shader_p->ID, "ambientLightFactor", 0.3);
				GL3D_uniformFloat(shader_p->ID, "diffuseLightFactor", 0.7);

				GL3D_uniformTexture(shader_p->ID, "colorTexture", 0, texture_p->ID);
				//GL3D_uniformTexture(shader_p->ID, "alphaTexture", 1, alphaTexture_p->ID);
				GL3D_uniformTexture(shader_p->ID, "shadowMapDepthTexture", 1, game_p->shadowMapDepthTexture.ID);
				GL3D_uniformTexture(shader_p->ID, "grassShadowMapDepthTexture", 2, game_p->grassShadowMapDepthTexture.ID);
				GL3D_uniformTexture(shader_p->ID, "bigShadowMapDepthTexture", 3, game_p->bigShadowMapDepthTexture.ID);

				GL3D_uniformMat4f(shader_p->ID, "viewMatrix", viewMatrix);
				GL3D_uniformMat4f(shader_p->ID, "lightViewMatrix", lightViewMatrix);
				GL3D_uniformMat4f(shader_p->ID, "bigLightViewMatrix", bigLightViewMatrix);
				GL3D_uniformMat4f(shader_p->ID, "modelMatrix", modelMatrix);

				GL3D_uniformVec4f(shader_p->ID, "inputColor", getVec4f(0.7, 0.7, 0.7, 1.0));

				glDrawArrays(GL_TRIANGLES, 0, model_p->numberOfTriangles * 3);

				
			}
		
		}

		//draw grass particles
		if(renderStage == RENDER_STAGE_SCENE){

			glDisable(GL_CULL_FACE);

			Shader *shader_p = Game_getShaderPointerByName(game_p, "grass-particle");

			glUseProgram(shader_p->ID);

			GL3D_uniformFloat(shader_p->ID, "grassShadowStrength", GRASS_SHADOW_STRENGTH);

			GL3D_uniformTexture(shader_p->ID, "shadowMapDepthTexture", 1, game_p->shadowMapDepthTexture.ID);
			GL3D_uniformTexture(shader_p->ID, "grassShadowMapDepthTexture", 2, game_p->grassShadowMapDepthTexture.ID);
			GL3D_uniformTexture(shader_p->ID, "bigShadowMapDepthTexture", 3, game_p->bigShadowMapDepthTexture.ID);
			GL3D_uniformTexture(shader_p->ID, "paintMapTexture", 4, game_p->paintMapTexture.ID);

			GL3D_uniformMat4f(shader_p->ID, "viewMatrix", viewMatrix);
			GL3D_uniformMat4f(shader_p->ID, "lightViewMatrix", lightViewMatrix);
			GL3D_uniformMat4f(shader_p->ID, "bigLightViewMatrix", bigLightViewMatrix);

			for(int i = 0; i < game_p->grassParticles.size(); i++){

				Particle *particle_p = &game_p->grassParticles[i];

				float scale = 1.0;

				Mat4f modelMatrix = getModelMatrix(particle_p->pos, particle_p->scale, particle_p->orientation);

				Model *model_p = Game_getModelPointerByName(game_p, "quad");

				Texture *texture_p = Game_getTexturePointerByName(game_p, "grass");

				glBindBuffer(GL_ARRAY_BUFFER, model_p->VBO);
				glBindVertexArray(model_p->VAO);

				GL3D_uniformTexture(shader_p->ID, "colorTexture", 0, texture_p->ID);

				GL3D_uniformMat4f(shader_p->ID, "modelMatrix", modelMatrix);

				GL3D_uniformVec4f(shader_p->ID, "inputColor", particle_p->color);

				GL3D_uniformFloat(shader_p->ID, "textureY", particle_p->textureY);
				GL3D_uniformFloat(shader_p->ID, "textureSizeY", particle_p->textureSizeY);

				glDrawArrays(GL_TRIANGLES, 0, model_p->numberOfTriangles * 3);

				
			}

			glEnable(GL_CULL_FACE);

		}

		//draw sword
		if(renderStage == RENDER_STAGE_SCENE
		&& swingAngle < SWING_ANGLE_RANGE){

			Shader *shader_p = Game_getShaderPointerByName(game_p, "model");
			if(renderStage == RENDER_STAGE_SHADOWS
			|| renderStage == RENDER_STAGE_BIG_SHADOWS){
				shader_p = Game_getShaderPointerByName(game_p, "model-shadow");
			}

			glUseProgram(shader_p->ID);

			GL3D_uniformFloat(shader_p->ID, "grassShadowStrength", GRASS_SHADOW_STRENGTH);
			GL3D_uniformFloat(shader_p->ID, "ambientLightFactor", 0.3);
			GL3D_uniformFloat(shader_p->ID, "diffuseLightFactor", 0.7);

			GL3D_uniformTexture(shader_p->ID, "shadowMapDepthTexture", 1, game_p->shadowMapDepthTexture.ID);
			GL3D_uniformTexture(shader_p->ID, "grassShadowMapDepthTexture", 2, game_p->grassShadowMapDepthTexture.ID);
			GL3D_uniformTexture(shader_p->ID, "bigShadowMapDepthTexture", 3, game_p->bigShadowMapDepthTexture.ID);

			GL3D_uniformMat4f(shader_p->ID, "viewMatrix", viewMatrix);
			GL3D_uniformMat4f(shader_p->ID, "lightViewMatrix", lightViewMatrix);
			GL3D_uniformMat4f(shader_p->ID, "bigLightViewMatrix", bigLightViewMatrix);

			Mat4f modelMatrix = getSwordMatrix(cameraPos, cameraDirection, swingAngle);

			Model *model_p = Game_getModelPointerByName(game_p, "sword");
			Texture *texture_p = Game_getTexturePointerByName(game_p, "blank");

			glBindBuffer(GL_ARRAY_BUFFER, model_p->VBO);
			glBindVertexArray(model_p->VAO);

			GL3D_uniformTexture(shader_p->ID, "colorTexture", 0, texture_p->ID);

			GL3D_uniformMat4f(shader_p->ID, "modelMatrix", modelMatrix);

			GL3D_uniformVec4f(shader_p->ID, "inputColor", getVec4f(0.7, 0.7, 0.7, 1.0));

			glDrawArrays(GL_TRIANGLES, 0, model_p->numberOfTriangles * 3);
			
		}

		//draw water
		if(renderStage == RENDER_STAGE_SCENE && true){

			float scale = 100.0;

			Mat4f rotationMatrix = getQuaternionMat4f(getQuaternion(getVec3f(1.0, 0.0, 0.0), 0.0));

			Mat4f scalingMatrix = getScalingMat4f(scale);

			Mat4f translationMatrix = getTranslationMat4f(getVec3f(-scale, 0.0, 0.0));
			//Mat4f translationMatrix = getTranslationMat4f(getVec3f(5.0, 5.0, 5.0));

			Shader *shader_p = Game_getShaderPointerByName(game_p, "water");

			glUseProgram(shader_p->ID);

			Model *model_p = &game_p->models[Game_getModelIndexByName(game_p, "water")];

			//Texture *voronoiTexture_p = Game_getTexturePointerByName(game_p, "voronoi-smooth-normals");
			Texture *voronoiTexture_p = Game_getTexturePointerByName(game_p, "voronoi");
			Texture *normalMap_p = Game_getTexturePointerByName(game_p, "water-normals");
			Texture *noiseMap_p = Game_getTexturePointerByName(game_p, "ripple-noise");

			glBindBuffer(GL_ARRAY_BUFFER, model_p->VBO);
			glBindVertexArray(model_p->VAO);

			GL3D_uniformTexture(shader_p->ID, "voronoiTexture", 0, voronoiTexture_p->ID);
			GL3D_uniformTexture(shader_p->ID, "normalMap", 1, normalMap_p->ID);
			GL3D_uniformTexture(shader_p->ID, "noiseMap", 2, noiseMap_p->ID);

			GL3D_uniformMat4f(shader_p->ID, "viewMatrix", viewMatrix);
			GL3D_uniformMat4f(shader_p->ID, "rotationMatrix", rotationMatrix);
			GL3D_uniformMat4f(shader_p->ID, "scalingMatrix", scalingMatrix);
			GL3D_uniformMat4f(shader_p->ID, "translationMatrix", translationMatrix);

			GL3D_uniformFloat(shader_p->ID, "inputT", windTime);
			GL3D_uniformFloat(shader_p->ID, "scale", scale);

			GL3D_uniformVec3f(shader_p->ID, "cameraDirection", cameraDirection);

			glDrawArrays(GL_TRIANGLES, 0, model_p->numberOfTriangles * 3);
			
		}

		//draw bullet traces
		if(renderStage == RENDER_STAGE_SCENE){

			glDisable(GL_CULL_FACE);

			Shader *shader_p = Game_getShaderPointerByName(game_p, "model");
			if(renderStage == RENDER_STAGE_SHADOWS
			|| renderStage == RENDER_STAGE_BIG_SHADOWS){
				shader_p = Game_getShaderPointerByName(game_p, "model-shadow");
			}

			glUseProgram(shader_p->ID);

			GL3D_uniformFloat(shader_p->ID, "grassShadowStrength", GRASS_SHADOW_STRENGTH);
			GL3D_uniformFloat(shader_p->ID, "ambientLightFactor", 1.0);
			GL3D_uniformFloat(shader_p->ID, "diffuseLightFactor", 0.0);

			GL3D_uniformTexture(shader_p->ID, "shadowMapDepthTexture", 1, game_p->shadowMapDepthTexture.ID);
			GL3D_uniformTexture(shader_p->ID, "grassShadowMapDepthTexture", 2, game_p->grassShadowMapDepthTexture.ID);
			GL3D_uniformTexture(shader_p->ID, "bigShadowMapDepthTexture", 3, game_p->bigShadowMapDepthTexture.ID);

			GL3D_uniformMat4f(shader_p->ID, "viewMatrix", viewMatrix);
			GL3D_uniformMat4f(shader_p->ID, "lightViewMatrix", lightViewMatrix);
			GL3D_uniformMat4f(shader_p->ID, "bigLightViewMatrix", bigLightViewMatrix);

			for(int i = 0; i < game_p->bulletTraces.size(); i++){

				BulletTrace *bulletTrace_p = &game_p->bulletTraces[i];

				Vec3f direction = bulletTrace_p->endPos - bulletTrace_p->startPos;

				Vec3f axis = normalize(cross(getVec3f(0.0, 1.0, 0.0), direction));
				float angle = acos(dot(normalize(direction), getVec3f(0.0, 1.0, 0.0)));

				Mat4f modelMatrix = getModelMatrix((bulletTrace_p->startPos + bulletTrace_p->endPos) / 2.0, getVec3f(0.03, length(direction) / 2.0, 1.0), getQuaternion(axis, angle));

				Model *model_p = Game_getModelPointerByName(game_p, "quad");
				Texture *texture_p = Game_getTexturePointerByName(game_p, "blank");

				glBindBuffer(GL_ARRAY_BUFFER, model_p->VBO);
				glBindVertexArray(model_p->VAO);

				GL3D_uniformTexture(shader_p->ID, "colorTexture", 0, texture_p->ID);

				GL3D_uniformMat4f(shader_p->ID, "modelMatrix", modelMatrix);

				GL3D_uniformVec4f(shader_p->ID, "inputColor", getVec4f(1.0, 0.9, 0.8, bulletTrace_p->alpha));

				glDrawArrays(GL_TRIANGLES, 0, model_p->numberOfTriangles * 3);
			
			}

			glEnable(GL_CULL_FACE);

		}
	
	}

	//draw hud
	{
		glDisable(GL_DEPTH_TEST);

		Renderer2D_setShader(&game_p->renderer2D, game_p->renderer2D.colorShader);

		Renderer2D_setColor(&game_p->renderer2D, getVec4f(1.0, 0.0, 1.0, 1.0));

		Renderer2D_drawRectangle(&game_p->renderer2D, WIDTH / 2 - 4, HEIGHT / 2 - 4, 8, 8);

		//draw hud
		{
			Player *player_p = World_getPlayerPointerByConnectionID(&game_p->world, game_p->client.connectionID);

			Renderer2D_setShader(&game_p->renderer2D, game_p->renderer2D.textureColorShader);

			Renderer2D_setColor(&game_p->renderer2D, getVec4f(1.0, 1.0, 1.0, 1.0));

			char healthText[STRING_SIZE];
			String_set(healthText, "", STRING_SIZE);
			sprintf(healthText, "%i HP", player_p->health);

			Renderer2D_drawText(&game_p->renderer2D, healthText, 40, 40, 60, game_p->font);


			if(player_p->weapon == WEAPON_GUN){
				Renderer2D_drawText(&game_p->renderer2D, "Gun", 40, 100, 60, game_p->font);
			}
			if(player_p->weapon == WEAPON_SWORD){
				Renderer2D_drawText(&game_p->renderer2D, "Sword", 40, 100, 60, game_p->font);
			}

			if(game_p->dead){
				Renderer2D_drawText(&game_p->renderer2D, "Dead", 500, 500, 200, game_p->font);
			}

		}
	
		glEnable(GL_DEPTH_TEST);
	}

	//glBindFramebuffer(GL_READ_FRAMEBUFFER, shadowMapFBO);
	//glBlitFramebuffer(0, 0, SHADOW_MAP_WIDTH, SHADOW_MAP_HEIGHT, 0, 0, Engine_clientWidth, Engine_clientHeight, GL_COLOR_BUFFER_BIT, GL_NEAREST);
	//glBindFramebuffer(GL_READ_FRAMEBUFFER, bigShadowMapFBO);
	//glBlitFramebuffer(0, 0, BIG_SHADOW_MAP_WIDTH, BIG_SHADOW_MAP_HEIGHT, 0, 0, Engine_clientWidth, Engine_clientHeight, GL_COLOR_BUFFER_BIT, GL_NEAREST);

	gameTime += 0.1;
	windTime += 0.01;


}
