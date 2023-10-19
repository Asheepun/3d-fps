#include "engine/3d-no-driver.h"
#include "engine/files.h"

void TriangleMesh_initFromFile_mesh(TriangleMesh *triangleMesh_p, const char *path){

	long int fileSize;
	char *data = getFileData_mustFree(path, &fileSize);

	triangleMesh_p->n_triangles = fileSize / (sizeof(float) * 8 * 3);
	triangleMesh_p->triangles = (Vec3f *)malloc(triangleMesh_p->n_triangles * 3 * sizeof(Vec3f));
	
	for(int i = 0; i < triangleMesh_p->n_triangles * 3; i++){
		
		memcpy(triangleMesh_p->triangles + i, data + i * 8 * sizeof(float), sizeof(Vec3f));

	}

	free(data);

}

void BoneTriangleMesh_initFromFile(BoneTriangleMesh *boneTriangleMesh_p, const char *path){

	long int fileSize;
	char *data = getFileData_mustFree(path, &fileSize);

	int n_triangles = fileSize / (BONE_MODEL_COMPONENT_SIZE * 3);

	boneTriangleMesh_p->triangles = (Vec3f *)malloc(sizeof(Vec3f) * 3 * n_triangles);
	boneTriangleMesh_p->indices = (unsigned char *)malloc(sizeof(unsigned char) * 4 * 3 * n_triangles);
	boneTriangleMesh_p->weights = (Vec4f *)malloc(sizeof(Vec4f) * 3 * n_triangles);

	for(int i = 0; i < n_triangles * 3; i++){

		memcpy(boneTriangleMesh_p->triangles + i, data + i * BONE_MODEL_COMPONENT_SIZE, sizeof(Vec3f));

		memcpy(boneTriangleMesh_p->indices + i * 4, data + i * BONE_MODEL_COMPONENT_SIZE + 8 * sizeof(float), 4 * sizeof(unsigned char));

		memcpy(boneTriangleMesh_p->weights + i, data + i * BONE_MODEL_COMPONENT_SIZE + 8 * sizeof(float) + 4 * sizeof(unsigned char), sizeof(Vec4f));

	}

	boneTriangleMesh_p->n_triangles = n_triangles;

	free(data);

}

void BoneRig_initFromFile(BoneRig *boneRig_p, const char *path){

	boneRig_p->originBones = getBonesFromFile(path);

	std::vector<Mat4f> bindMatrices = getBindMatricesFromBones(boneRig_p->originBones);

	boneRig_p->inverseBindMatrices.clear();

	for(int i = 0; i < bindMatrices.size(); i++){
		boneRig_p->inverseBindMatrices.push_back(inverse(bindMatrices[i]));
	}

}

void TriangleMesh_free(TriangleMesh *triangleMesh_p){
	free(triangleMesh_p->triangles);
}

std::vector<Bone> getBonesFromFile(const char *path){

	std::vector<Bone> bones;

	int n_lines;
	FileLine *fileLines = getFileLines_mustFree(path, &n_lines);

	for(int i = 0; i < n_lines; i++){

		if(strcmp(fileLines[i], ":BONE") == 0){
			Bone bone;

			String_set(bone.name, fileLines[i + 1], STRING_SIZE);

			char *ptr = fileLines[i + 2];
			bone.parent = strtol(ptr, &ptr, 10);

			ptr = fileLines[i + 3];
			bone.rotation.x = strtof(ptr, &ptr);
			bone.rotation.y = strtof(ptr + 1, &ptr);
			bone.rotation.z = strtof(ptr + 1, &ptr);
			bone.rotation.w = strtof(ptr + 1, &ptr);

			ptr = fileLines[i + 4];
			bone.scale.x = strtof(ptr, &ptr);
			bone.scale.y = strtof(ptr + 1, &ptr);
			bone.scale.z = strtof(ptr + 1, &ptr);

			ptr = fileLines[i + 5];
			bone.translation.x = strtof(ptr, &ptr);
			bone.translation.y = strtof(ptr + 1, &ptr);
			bone.translation.z = strtof(ptr + 1, &ptr);

			bones.push_back(bone);
		}

	}

	free(fileLines);

	return bones;

}

std::vector<Bone> getInterpolatedBones(std::vector<Bone> bones0, std::vector<Bone> bones1, float t){

	std::vector<Bone> interpolatedBones;

	for(int i = 0; i < bones0.size(); i++){

		Bone interpolatedBone = bones0[i];

		interpolatedBone.rotation = lerp(bones0[i].rotation, bones1[i].rotation, t);
		interpolatedBone.scale = lerp(bones0[i].scale, bones1[i].scale, t);
		interpolatedBone.translation = lerp(bones0[i].translation, bones1[i].translation, t);

		interpolatedBones.push_back(interpolatedBone);

	}

	return interpolatedBones;

}

std::vector<Mat4f> getBindMatricesFromBones(std::vector<Bone> bones){


	Mat4f matrices[bones.size()];
	bool matrixCalculationFlags[bones.size()];
	memset(matrixCalculationFlags, 0, bones.size() * sizeof(bool));

	//printf("---\n");

	for(int iteration = 0; iteration < bones.size(); iteration++){
		for(int i = 0; i < bones.size(); i++){

			if(matrixCalculationFlags[i]){
				continue;
			}

			Bone *bone_p = &bones[i];

			if(bone_p->parent == -1){

				//printf("%i\n", i);

				Mat4f transformation = getIdentityMat4f();

				//transformation *= getScalingMat4f(bone_p->scale);
				transformation *= getQuaternionMat4f(bone_p->rotation);
				transformation *= getTranslationMat4f(bone_p->translation);

				//Vec3f_log(bone_p->scale);
				//Vec4f_log(bone_p->rotation);
				//Vec3f_log(bone_p->translation);
				//Mat4f_log(transformation);

				matrices[i] = transformation;
				matrixCalculationFlags[i] = true;
				
			}else if(matrixCalculationFlags[bone_p->parent]){

				//printf("%i\n", i);
				
				Mat4f transformation = getIdentityMat4f();

				transformation *= getQuaternionMat4f(bone_p->rotation);
				transformation *= getTranslationMat4f(bone_p->translation);
				//transformation *= getScalingMat4f(bone_p->scale);

				transformation *= matrices[bone_p->parent];

				//Vec3f_log(bone_p->scale);
				//Vec4f_log(bone_p->rotation);
				//Vec3f_log(bone_p->translation);
				//Mat4f_log(transformation);

				matrices[i] = transformation;
				matrixCalculationFlags[i] = true;

			}


		}
	}

	std::vector<Mat4f> outputMatrices;

	for(int i = 0; i < bones.size(); i++){
		outputMatrices.push_back(matrices[i]);
	}

	return outputMatrices;

}

std::vector<Mat4f> getBoneRigTransformations(BoneRig *boneRig_p, std::vector<Bone> bones){

	std::vector<Mat4f> bindMatrices = getBindMatricesFromBones(bones);

	std::vector<Mat4f> transformations;

	for(int i = 0; i < bindMatrices.size(); i++){
		Mat4f transformation = boneRig_p->inverseBindMatrices[i];
		transformation *= bindMatrices[i];
		//Mat4f transformation = bindMatrices[i];
		//transformation *= boneRig_p->inverseBindMatrices[i];
		transformations.push_back(transformation);
	}

	return transformations;

}
