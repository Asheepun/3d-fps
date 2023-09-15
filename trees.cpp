#include "game.h"

#include "stdio.h"
#include "math.h"

float TREE_SCALE = 1.5;

struct TreeNode{
	int parent;
	Vec3f pos;
	float radius;
};

void Game_addTree(Game *game_p, Vec3f treePos){

	setRandomSeed(getRandom() * 100.0);

	Tree tree;

	std::vector<TreeNode> nodes;

	//main branch
	{
		TreeNode node;
		node.parent = -1;
		node.pos = getVec3f(0.0, 0.0, 0.0);
		node.radius = 0.4;
		nodes.push_back(node);
	}
	{
		TreeNode node;
		node.parent = 0;
		node.pos = getVec3f(0.0, 2.0 + getRandom() * 1, 0.0);
		node.radius = 0.3;
		nodes.push_back(node);
	
	}

	int rootNodeIndex = 1;
	TreeNode *rootNode_p = &nodes[rootNodeIndex];

	std::vector<Vec3f> targetPoints;
	float angle = 0.0;
	int n_targetPoints = 3 + int(getRandom() * 3.0);

	for(int i = 0; i < n_targetPoints; i++){

		float length = 3.5 + getRandom() * 1.0;
		angle += (0.75 + getRandom() * 0.5) * 2 * M_PI / (float)n_targetPoints;
		
		Vec4f point = getVec4f(length, 0.0, 0.0, 1.0);

		//printf("point\n");

		//Vec4f_log(point);

		point = getQuaternionMat4f(getQuaternion(getVec3f(0.0, 1.0, 0.0), angle)) * point;

		//Vec4f_log(point);
		//Vec4f_mulByMat4f(&point, getQuaternionMat4f(getQuaternion(getVec3f(0.0, 1.0, 0.0), angle)));

		point.y += rootNode_p->pos.y + 2.0 + getRandom() * 3.0;

		targetPoints.push_back(getVec3f(point.x, point.y, point.z));

	}

	bool addedDelta = false;
	float divergence = 1.0;

	for(int i = 0; i < targetPoints.size(); i++){

		float n_steps = 3.0 + int(getRandom() * 3.0);

		int baseIndex;

		for(float j = 0; j < n_steps; j += 1.0){

			TreeNode node;
			node.parent = nodes.size() - 1;
			if(j < 0.1){
				node.parent = rootNodeIndex;
				baseIndex = nodes.size();
			}

			TreeNode *parentNode_p = &nodes[node.parent];

			//printf("%f\n", parentNode_p->radius);

			int searchIndex = parentNode_p->parent;
			Vec3f rootPos = parentNode_p->pos;
			while(searchIndex != -1){
				rootPos += nodes[searchIndex].pos;
				searchIndex = nodes[searchIndex].parent;
			}

			node.pos = (targetPoints[i] - rootPos) / n_steps;
			node.pos.x *= (1 - j / n_steps) * divergence;
			node.pos.z *= (1 - j / n_steps) * divergence;
			//node.radius = lerp(rootNode_p->radius * 0.7, 0.0, j / n_steps);
			node.radius = parentNode_p->radius * 0.7;
			if(j + 0.01 > n_steps - 1.0){
				node.radius = 0.05;
			}
			//node.radius = fmax(nodes[node.parent].radius - 0.05, 0.0);
			nodes.push_back(node);
			
		}

		{
			TreeNode node;
			node.parent = nodes.size() - 1;
			node.pos = getVec3f(0.0, 1.0 + getRandom() * 0.5, 0.0);
			node.radius = 0.0;
			nodes.push_back(node);
		}	

		if(i < targetPoints.size() - 1
		&& targetPoints[i].y > targetPoints[i + 1].y
		&& !addedDelta){
			//targetPoints[i + 1] += diff * 0.5;
			rootNodeIndex = baseIndex + int(getRandom() * 2.0);
			rootNodeIndex = baseIndex;
			rootNode_p = &nodes[rootNodeIndex];
			addedDelta = true;
			divergence = 0.5;

			Vec3f diff = targetPoints[i] - targetPoints[i + 1];
			targetPoints[i + 1].x += diff.x * 0.5;
			targetPoints[i + 1].z += diff.z * 0.5;
			//nodes[rootNodeIndex + 1].pos += diff;
			nodes[rootNodeIndex + 1].pos.x = diff.x * 0.2;
			nodes[rootNodeIndex + 1].pos.z = diff.z * 0.2;
		}else{
			rootNodeIndex = 1;
			rootNode_p = &nodes[rootNodeIndex];
			addedDelta = false;
			divergence = 1.0;
		}

	}

	/*
	{
		TreeNode node;
		node.parent = 1;
		node.pos = getVec3f(0.0, 1.0, 0.3);
		node.radius = 0.3;
		nodes.push_back(node);
	
	}
	{
		TreeNode node;
		node.parent = 2;
		node.pos = getVec3f(-0.2, 1.5, 1.0);
		node.radius = 0.2;
		nodes.push_back(node);
	}
	{
		TreeNode node;
		node.parent = 3;
		node.pos = getVec3f(0.0, 1.0, 0.0);
		node.radius = 0.1;
		nodes.push_back(node);
	}
	{
		TreeNode node;
		node.parent = 4;
		node.pos = getVec3f(-1.0, 1.5, 0.0);
		node.radius = 0.05;
		nodes.push_back(node);
	}
	{
		TreeNode node;
		node.parent = 5;
		node.pos = getVec3f(-0.1, 1.0, 0.0);
		node.radius = 0.0;
		nodes.push_back(node);
	}

	//second branch
	{
		TreeNode node;
		node.parent = 1;
		node.pos = getVec3f(0.0, 1.0, -1.0);
		node.radius = 0.2;
		nodes.push_back(node);
	}
	int secondBranchIndex = nodes.size() - 1;
	{
		TreeNode node;
		node.parent = secondBranchIndex;
		node.pos = getVec3f(-0.5, 0.7, -0.7);
		node.radius = 0.1;
		nodes.push_back(node);
	}
	{
		TreeNode node;
		node.parent = nodes.size() - 1;
		node.pos = getVec3f(-0.3, 1.0, -0.3);
		node.radius = 0.05;
		nodes.push_back(node);
	}
	{
		TreeNode node;
		node.parent = nodes.size() - 1;
		node.pos = getVec3f(-0.1, 1.0, -0.1);
		node.radius = 0.0;
		nodes.push_back(node);
	}
	{
		TreeNode node;
		node.parent = secondBranchIndex;
		node.pos = getVec3f(0.5, 0.7, -0.7);
		node.radius = 0.1;
		nodes.push_back(node);
	}
	{
		TreeNode node;
		node.parent = nodes.size() - 1;
		node.pos = getVec3f(0.3, 1.0, -0.3);
		node.radius = 0.05;
		nodes.push_back(node);
	}
	{
		TreeNode node;
		node.parent = nodes.size() - 1;
		node.pos = getVec3f(0.1, 1.0, -0.1);
		node.radius = 0.0;
		nodes.push_back(node);
	}

	//third branch
	{
		TreeNode node;
		node.parent = 2;
		node.pos = getVec3f(0.7, 0.7, 0.0);
		node.radius = 0.2;
		nodes.push_back(node);
	}
	{
		TreeNode node;
		node.parent = nodes.size() - 1;
		node.pos = getVec3f(0.7, 1.0, 0.0);
		node.radius = 0.1;
		nodes.push_back(node);
	}
	{
		TreeNode node;
		node.parent = nodes.size() - 1;
		node.pos = getVec3f(0.3, 1.0, 0.0);
		node.radius = 0.05;
		nodes.push_back(node);
	}
	{
		TreeNode node;
		node.parent = nodes.size() - 1;
		node.pos = getVec3f(0.1, 1.0, 0.0);
		node.radius = 0.0;
		nodes.push_back(node);
	}
	*/

	//create triangle mesh
	std::vector<Vec3f> triangles;
	std::vector<Vec2f> textureCoords;

	for(int i = 0; i < nodes.size(); i++){

		TreeNode *node1_p = &nodes[i];

		for(int j = 0; j < nodes.size(); j++){

			TreeNode *node2_p = &nodes[j];

			if(i == node2_p->parent){

				node2_p->pos += node1_p->pos;

				Vec3f right2 = getVec3f(1.0, 0.0, 0.0);
				Vec3f forward2 = normalize(cross(right2, node2_p->pos - node1_p->pos));
				right2 = normalize(cross(forward2, node2_p->pos - node1_p->pos));

				Vec3f right1 = right2;
				Vec3f forward1 = forward2;

				if(node1_p->parent != -1){
					TreeNode *node3_p = &nodes[node1_p->parent];
					right1 = getVec3f(1.0, 0.0, 0.0);
					forward1 = normalize(cross(right1, node1_p->pos - node3_p->pos));
					right1 = normalize(cross(forward1, node1_p->pos - node3_p->pos));
				}

				std::vector<Vec3f> bottomPoints;
				std::vector<Vec3f> topPoints;

				//float r1 = 0.3;
				//float r2 = 0.3;

				int edges = 5;

				for(int i = 0; i < edges; i++){
					float angle = 2.0 * M_PI * ((float)i / float(edges));
					bottomPoints.push_back(node1_p->pos + right1 * cos(angle) * node1_p->radius + forward1 * sin(angle) * node1_p->radius);
					topPoints.push_back(node2_p->pos + right2 * cos(angle) * node2_p->radius + forward2 * sin(angle) * node2_p->radius);
				}

				for(int i = 0; i < edges; i++){
					
					Vec3f p1 = bottomPoints[i % edges];
					Vec3f p2 = bottomPoints[(i + 1) % edges];
					Vec3f p3 = topPoints[i % edges];
					Vec3f p4 = topPoints[(i + 1) % edges];

					triangles.push_back(p1);
					triangles.push_back(p2);
					triangles.push_back(p3);

					triangles.push_back(p3);
					triangles.push_back(p2);
					triangles.push_back(p4);

					Vec2f t1 = getVec2f((float)(i % edges) / (float)edges, 0.0);
					Vec2f t2 = getVec2f((float)((i + 1) % edges) / (float)edges, 0.0);
					Vec2f t3 = getVec2f((float)(i % edges) / (float)edges, 1.0);
					Vec2f t4 = getVec2f((float)((i + 1) % edges) / (float)edges, 1.0);

					textureCoords.push_back(t1);
					textureCoords.push_back(t2);
					textureCoords.push_back(t3);

					textureCoords.push_back(t3);
					textureCoords.push_back(t2);
					textureCoords.push_back(t4);

				}
				
			}

		}
		
	}

	TriangleMesh triangleMesh;

	//String_set(triangleMesh.name, "tree", STRING_SIZE);

	triangleMesh.n_triangles = triangles.size() / 3;
	triangleMesh.triangles = (Vec3f *)malloc(sizeof(Vec3f) * 3 * triangleMesh.n_triangles);
	memcpy(triangleMesh.triangles, &triangles[0], sizeof(Vec3f) * 3 * triangleMesh.n_triangles);

	unsigned char *meshData = generateMeshDataFromTriangleMesh(triangleMesh, &textureCoords[0]);

	Model model;

	Model_initFromMeshData(&model, meshData, triangleMesh.n_triangles);

	//String_set(model.name, "tree", STRING_SIZE);

	game_p->triangleMeshes.push_back(triangleMesh);
	game_p->models.push_back(model);

	int triangleMeshIndex = game_p->triangleMeshes.size() - 1;
	int modelIndex = game_p->models.size() - 1;

	free(meshData);

	//create leaves
	for(int i = 0; i < nodes.size(); i++){

		TreeNode *node1_p = &nodes[i];
		TreeNode *node2_p = &nodes[node1_p->parent];

		if(node1_p->radius < 0.06){

			int heightSegments = 5;
			int radialSegments = 5;
			//int heightSegments = 1;
			//int radialSegments = 1;

			//generate leaves pointing downards along branch
			for(int j = 0; j < heightSegments; j++){
				for(int n = 0; n < radialSegments; n++){

					float h = (float)j / (float)heightSegments;
					float t = (float)n / (float)radialSegments;

					Mat4f transformations = getIdentityMat4f();

					//transformations *= getTranslationMat4f(getVec3f(0.0, 1.0, 0.0));

					transformations *= getScalingMat4f((0.5 + 0.5 * sin(h * M_PI)) * TREE_SCALE);

					transformations *= getQuaternionMat4f(getQuaternion(getVec3f(1.0, 0.0, 0.0), M_PI / 2.0 + 0.2 + getRandom() * 0.3));

					transformations *= getQuaternionMat4f(getQuaternion(getVec3f(0.0, 1.0, 0.0), t * M_PI * 2.0 + h));

					transformations *= getTranslationMat4f(treePos + (node2_p->pos + (node1_p->pos - node2_p->pos) * h) * TREE_SCALE);

					tree.leafTransformations.push_back(transformations);
				
				}
			}

			heightSegments = 4;
			radialSegments = 5;
			//heightSegments = 1;
			//radialSegments = 1;

			//generate leaves pointing upwards on top
			for(int j = 0; j < heightSegments; j++){
				for(int n = 0; n < radialSegments; n++){

					float h = (float)j / (float)heightSegments;
					float t = (float)n / (float)radialSegments;

					Mat4f transformations = getIdentityMat4f();

					transformations *= getScalingMat4f((0.7 - 0.4 * h) * TREE_SCALE);

					transformations *= getQuaternionMat4f(getQuaternion(getVec3f(1.0, 0.0, 0.0), M_PI / 2.0 - h * M_PI / 3.0 + 0.1 + 0.3 * getRandom()));

					transformations *= getQuaternionMat4f(getQuaternion(getVec3f(0.0, 1.0, 0.0), t * M_PI * 2.0 + h));

					transformations *= getTranslationMat4f(treePos + (node1_p->pos + getVec3f(0.0, -0.2, 0.0)) * TREE_SCALE);

					tree.leafTransformations.push_back(transformations);
				
				}
			}
		
		}

	}

	TextureBuffer_initAsMat4fArray(&tree.leafTransformationsTextureBuffer, &tree.leafTransformations[0], tree.leafTransformations.size(), false);

	tree.sortedLeafTransformations = (Mat4f *)malloc(sizeof(Mat4f) * tree.leafTransformations.size());

	game_p->trees.push_back(tree);

	//add obstacle
	{
		Obstacle obstacle;	
		obstacle.pos = treePos;
		obstacle.scale = TREE_SCALE;
		obstacle.modelIndex = modelIndex;
		obstacle.textureIndex = Game_getTextureIndexByName(game_p, "bark");
		obstacle.alphaTextureIndex = Game_getTextureIndexByName(game_p, "bark-alpha");
		obstacle.triangleMeshIndex = triangleMeshIndex;
		obstacle.color = getVec4f(1.0, 1.0, 1.0, 1.0);

		game_p->obstacles.push_back(obstacle);
		
	}

}
