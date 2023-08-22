#include "engine/geometry.h"
#include "engine/files.h"
#include "engine/3d.h"
#include "engine/shaders.h"

#define STB_IMAGE_STATIC
#define STB_IMAGE_IMPLEMENTATION
#include "stb/stb_image.h"

#ifdef _WIN32
#include "glad/wgl.h"
#endif
#include "glad/gl.h"

#include "stdio.h"
#include "math.h"
#include "string.h"
#include "stdlib.h"
#include <vector>

typedef struct Face{
	unsigned int indices[9];
}Face;

void Model_initFromMeshData(Model *model_p, const unsigned char *mesh, int numberOfTriangles){

	//printf("%i\n", numberOfTriangles);

	int componentSize = 2 * sizeof(Vec3f) + sizeof(Vec2f);

	glGenBuffers(1, &model_p->VBO);
	glBindBuffer(GL_ARRAY_BUFFER, model_p->VBO);
	glBufferData(GL_ARRAY_BUFFER, componentSize * 3 * numberOfTriangles, mesh, GL_STATIC_DRAW);

	glGenVertexArrays(1, &model_p->VAO);
	glBindVertexArray(model_p->VAO);

	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void *)0);
	glEnableVertexAttribArray(0);

	glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void *)(3 * sizeof(float)));
	glEnableVertexAttribArray(1);

	glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void *)(5 * sizeof(float)));
	glEnableVertexAttribArray(2);

	model_p->numberOfTriangles = numberOfTriangles;

}

void Model_initFromFile_mesh(Model *model_p, const char *path){

	long int fileSize;
	char *data = getFileData_mustFree(path, &fileSize);

	int numberOfTriangles = fileSize / (sizeof(float) * 8 * 3);

	Model_initFromMeshData(model_p, (unsigned char *)data, numberOfTriangles);

	free(data);

}

unsigned char *generateMeshDataFromTriangleMesh(TriangleMesh triangleMesh, Vec2f *textureData){

	float *data = (float *)malloc(8 * sizeof(float) * 3 * triangleMesh.n_triangles);

	for(int i = 0; i < triangleMesh.n_triangles; i++){
		
		int trianglesIndex = i * 3;
		int dataIndex = i * 8 * 3;

		Vec3f point1 = triangleMesh.triangles[trianglesIndex + 0];
		Vec3f point2 = triangleMesh.triangles[trianglesIndex + 1];
		Vec3f point3 = triangleMesh.triangles[trianglesIndex + 2];

		Vec3f normal = getCrossVec3f(getSubVec3f(point2, point1), getSubVec3f(point3, point1));
		Vec3f_normalize(&normal);

		//normal = getVec3f(0.0, 1.0, 0.0);

		memcpy(data + dataIndex + 8 * 0, &point1, sizeof(Vec3f));
		memcpy(data + dataIndex + 8 * 0 + 5, &normal, sizeof(Vec3f));

		memcpy(data + dataIndex + 8 * 1, &point2, sizeof(Vec3f));
		memcpy(data + dataIndex + 8 * 1 + 5, &normal, sizeof(Vec3f));

		memcpy(data + dataIndex + 8 * 2, &point3, sizeof(Vec3f));
		memcpy(data + dataIndex + 8 * 2 + 5, &normal, sizeof(Vec3f));

		if(textureData != NULL){
			memcpy(data + dataIndex + 8 * 0 + 3, textureData + trianglesIndex + 0, sizeof(Vec2f));
			memcpy(data + dataIndex + 8 * 1 + 3, textureData + trianglesIndex + 1, sizeof(Vec2f));
			memcpy(data + dataIndex + 8 * 2 + 3, textureData + trianglesIndex + 2, sizeof(Vec2f));
		}

	}

	return (unsigned char *)data;

}

void BoneModel_initFromFile(BoneModel *model_p, const char *meshPath, const char *bonesPath){

	//load bone mesh
	{
		long int fileSize;
		char *data = getFileData_mustFree(meshPath, &fileSize);

		//Vec3f testNormal;
		//memcpy(&testNormal, data + 3 * sizeof(float) + BONE_MODEL_COMPONENT_SIZE * 90, 3 * sizeof(float));

		//Vec3f_log(testNormal);
		//printf("%f\n", getMagVec3f(testNormal));

		//printf("done testing bone mesh\n");

		int n_triangles = fileSize / (BONE_MODEL_COMPONENT_SIZE * 3);

		/*
		for(int i = 0; i < n_triangles; i++){
			
			Vec3f p1 = *(Vec3f *)(data + (i * 3 + 0) * BONE_MODEL_COMPONENT_SIZE);
			Vec3f p2 = *(Vec3f *)(data + (i * 3 + 1) * BONE_MODEL_COMPONENT_SIZE);
			Vec3f p3 = *(Vec3f *)(data + (i * 3 + 2) * BONE_MODEL_COMPONENT_SIZE);

			Vec3f n = normalize(cross(p1 - p2, p1 - p3));

			Vec3f *n1_p = (Vec3f *)(data + (i * 3 + 0) * BONE_MODEL_COMPONENT_SIZE + sizeof(Vec3f));
			Vec3f *n2_p = (Vec3f *)(data + (i * 3 + 1) * BONE_MODEL_COMPONENT_SIZE + sizeof(Vec3f));
			Vec3f *n3_p = (Vec3f *)(data + (i * 3 + 2) * BONE_MODEL_COMPONENT_SIZE + sizeof(Vec3f));

			printf("---\n");
			Vec3f_log(*n1_p);
			Vec3f_log(*n2_p);
			Vec3f_log(*n3_p);
			Vec3f_log(n);

			*n1_p = n;
			*n2_p = n;
			*n3_p = n;

		}
		*/

		//printf("%i\n", fileSize);
		//printf("%i\n", n_triangles);

		glGenBuffers(1, &model_p->VBO);
		glBindBuffer(GL_ARRAY_BUFFER, model_p->VBO);
		glBufferData(GL_ARRAY_BUFFER, BONE_MODEL_COMPONENT_SIZE * 3 * n_triangles, data, GL_STATIC_DRAW);

		glGenVertexArrays(1, &model_p->VAO);
		glBindVertexArray(model_p->VAO);

		glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, BONE_MODEL_COMPONENT_SIZE, (void *)0);
		glEnableVertexAttribArray(0);

		glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, BONE_MODEL_COMPONENT_SIZE, (void *)(3 * sizeof(float)));
		glEnableVertexAttribArray(1);

		glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, BONE_MODEL_COMPONENT_SIZE, (void *)(6 * sizeof(float)));
		glEnableVertexAttribArray(2);

		glVertexAttribIPointer(3, 4, GL_UNSIGNED_BYTE, BONE_MODEL_COMPONENT_SIZE, (void *)(8 * sizeof(float)));
		glEnableVertexAttribArray(3);

		glVertexAttribPointer(4, 4, GL_FLOAT, GL_FALSE, BONE_MODEL_COMPONENT_SIZE, (void *)(8 * sizeof(float) + 4 * sizeof(unsigned char)));
		glEnableVertexAttribArray(4);

		model_p->n_triangles = n_triangles;

		free(data);
	
	}

	//load bones
	{
		int n_lines;
		FileLine *fileLines = getFileLines_mustFree(bonesPath, &n_lines);

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

				model_p->bones.push_back(bone);
			}

		}

		std::vector<Mat4f> bindMatrices = getBindMatricesFromBones(model_p->bones);

		model_p->inverseBindMatrices.clear();

		for(int i = 0; i < bindMatrices.size(); i++){
			model_p->inverseBindMatrices.push_back(inverse(bindMatrices[i]));
		}

	}

}

int BoneModel_getBoneIndexByName(BoneModel *model_p, const char *name){
	for(int i = 0; i < model_p->bones.size(); i++){
		if(strcmp(model_p->bones[i].name, name) == 0){
			return i;
		}
	}
	return -1;
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

	for(int iteration = 0; iteration < bones.size(); iteration++){
		for(int i = 0; i < bones.size(); i++){

			if(matrixCalculationFlags[i]){
				continue;
			}

			Bone *bone_p = &bones[i];

			if(bone_p->parent == -1){

				//printf("%i\n", i);

				Mat4f transformation = getIdentityMat4f();

				transformation *= getScalingMat4f(bone_p->scale);
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

				transformation *= getScalingMat4f(bone_p->scale);
				transformation *= getQuaternionMat4f(bone_p->rotation);
				transformation *= getTranslationMat4f(bone_p->translation);

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

void VertexMesh_initFromFile_mesh(VertexMesh *vertexMesh_p, const char *path){

	long int fileSize;
	char *data = getFileData_mustFree(path, &fileSize);

	int numberOfTriangles = fileSize / (sizeof(float) * 8 * 3);

	vertexMesh_p->length = numberOfTriangles * 3;
	vertexMesh_p->vertices = (Vec3f *)malloc(vertexMesh_p->length * sizeof(Vec3f));
	
	for(int i = 0; i < numberOfTriangles * 3; i++){
		
		memcpy(vertexMesh_p->vertices + i, data + i * 8 * sizeof(float), sizeof(Vec3f));

	}

	free(data);

}

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

void Texture_init(Texture *texture_p, const char *name, unsigned char *data, int width, int height){

	String_set(texture_p->name, name, SMALL_STRING_SIZE);

	glGenTextures(1, &texture_p->ID);

	glBindTexture(GL_TEXTURE_2D, texture_p->ID);

	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);

	glGenerateMipmap(GL_TEXTURE_2D);

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

}

void Texture_initAsDepthMap(Texture *texture_p, int width, int height){

	glGenTextures(1, &texture_p->ID);
	glBindTexture(GL_TEXTURE_2D, texture_p->ID);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT, width, height, 0, GL_DEPTH_COMPONENT, GL_FLOAT, NULL);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);

	Vec4f borderColor = getVec4f(1.0, 1.0, 1.0, 1.0);
	glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, (float *)&borderColor);

}

void Texture_initAsColorMap(Texture *texture_p, int width, int height){

	glGenTextures(1, &texture_p->ID);
	glBindTexture(GL_TEXTURE_2D, texture_p->ID);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, width, height, 0, GL_RGBA, GL_FLOAT, NULL);
	//glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_FLOAT, NULL);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);

	Vec4f borderColor = getVec4f(1.0, 1.0, 1.0, 1.0);
	glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, (float *)&borderColor);

}

void Texture_initFromFile(Texture *texture_p, const char *path, const char *name){

	int width, height, channels;
	unsigned char *data = stbi_load(path, &width, &height, &channels, 4);

	Texture_init(texture_p, name, data, width, height);

	free(data);

}

void TextureAtlas_init(TextureAtlas *textureAtlas_p, const char **pathsAndNames, int numberOfPaths){

	int atlasWidth = 0;
	int atlasHeight = 0;

	unsigned char *atlasData;
	unsigned char *textureData[numberOfPaths];
	Vec2f textureDataDimensions[numberOfPaths];

	for(int i = 0; i < numberOfPaths; i++){

		int width, height, channels;

		textureData[i] = stbi_load(pathsAndNames[i * 2], &width, &height, &channels, 4);

		textureDataDimensions[i].x = width;
		textureDataDimensions[i].y = height;

		atlasWidth += width;

		if(height > atlasHeight){
			atlasHeight = height;
		}

		SmallString name;
		String_set(name, pathsAndNames[i * 2 + 1], SMALL_STRING_SIZE);
		textureAtlas_p->names.push_back(name);

	}

	//create texture atlas data
	atlasData = (unsigned char *)malloc(4 * sizeof(unsigned char) * atlasWidth * atlasHeight);

	for(int y = 0; y < atlasHeight; y++){

		int x = 0;

		for(int i = 0; i < numberOfPaths; i++){

			if(y < (int)textureDataDimensions[i].y){

				unsigned char *dest = atlasData + 4 * (y * atlasWidth + x);
				unsigned char *src = textureData[i] + 4 * (y * (int)textureDataDimensions[i].y);
				int size = (int)textureDataDimensions[i].x * 4 * sizeof(unsigned char);

				memcpy(dest, src, size);

			}
				
			x += textureDataDimensions[i].x;

		}
	
	}

	//set texture coordinates
	{
		float x = 0.0;

		for(int i = 0; i < numberOfPaths; i++){

			Vec4f coordinates = getVec4f(x / atlasWidth, 1.0 - textureDataDimensions[i].y / atlasHeight, textureDataDimensions[i].x / atlasWidth, textureDataDimensions[i].y / atlasHeight);

			textureAtlas_p->textureCoordinates.push_back(coordinates);
			
			x += textureDataDimensions[i].x;
		
		}
	}

	Texture_init(&textureAtlas_p->texture, "atlas", atlasData, atlasWidth, atlasHeight);

	//free data
	free(atlasData);
	for(int i = 0; i < numberOfPaths; i++){
		free(textureData[i]);
	}

}

void TextureBuffer_init(TextureBuffer *textureBuffer_p, void *data, int size, enum Type3D type){

	GLenum glType;

	if(type == TYPE_I8){
		glType = GL_R8;
	}
	if(type == TYPE_I16){
		glType = GL_R16;
	}
	if(type == TYPE_F32){
		glType = GL_R32F;
	}

	glGenBuffers(1, &textureBuffer_p->VBO);
	glBindBuffer(GL_ARRAY_BUFFER, textureBuffer_p->VBO);
	glBufferData(GL_ARRAY_BUFFER, size, data, GL_STATIC_DRAW);

	glGenTextures(1, &textureBuffer_p->TB);
	glBindTexture(GL_TEXTURE_BUFFER, textureBuffer_p->TB);
	glTexBuffer(GL_TEXTURE_BUFFER, GL_RGBA32F, textureBuffer_p->VBO);

}

void TextureBuffer_initAsVec4fArray(TextureBuffer *textureBuffer_p, Vec4f *vectors, int n_vectors){

	textureBuffer_p->n_elements = n_vectors;
	textureBuffer_p->elementSize = sizeof(Vec4f);

	glGenBuffers(1, &textureBuffer_p->VBO);
	glBindBuffer(GL_ARRAY_BUFFER, textureBuffer_p->VBO);
	glBufferData(GL_ARRAY_BUFFER, textureBuffer_p->n_elements * textureBuffer_p->elementSize, vectors, GL_STATIC_DRAW);

	glGenTextures(1, &textureBuffer_p->TB);
	glBindTexture(GL_TEXTURE_BUFFER, textureBuffer_p->TB);
	glTexBuffer(GL_TEXTURE_BUFFER, GL_RGBA32F, textureBuffer_p->VBO);

}

void TextureBuffer_initAsMat4fArray(TextureBuffer *textureBuffer_p, Mat4f *matrices, int n_matrices, bool compress){

	textureBuffer_p->n_elements = n_matrices;
	textureBuffer_p->elementSize = sizeof(Mat4f);

	glGenBuffers(1, &textureBuffer_p->VBO);
	glBindBuffer(GL_ARRAY_BUFFER, textureBuffer_p->VBO);
	glBufferData(GL_ARRAY_BUFFER, textureBuffer_p->n_elements * textureBuffer_p->elementSize, matrices, GL_STATIC_DRAW);

	glGenTextures(1, &textureBuffer_p->TB);
	glBindTexture(GL_TEXTURE_BUFFER, textureBuffer_p->TB);
	glTexBuffer(GL_TEXTURE_BUFFER, GL_RGBA32F, textureBuffer_p->VBO);

}


void TextureBuffer_free(TextureBuffer *textureBuffer_p){
	
	glDeleteBuffers(1, &textureBuffer_p->VBO);
	glDeleteTextures(1, &textureBuffer_p->TB);

}

void Shader_init(Shader *shader_p, const char *name, const char *vertexShaderPath, const char *fragmentShaderPath){

	unsigned int vertexShader = getCompiledShader(vertexShaderPath, GL_VERTEX_SHADER);
	unsigned int fragmentShader = getCompiledShader(fragmentShaderPath, GL_FRAGMENT_SHADER);

	shader_p->ID = glCreateProgram();
	glAttachShader(shader_p->ID, vertexShader);
	glAttachShader(shader_p->ID, fragmentShader);
	glLinkProgram(shader_p->ID);

	String_set(shader_p->name, name, STRING_SIZE);

}

void GL3D_uniformMat4f(unsigned int shaderProgram, const char *locationName, Mat4f m){

	unsigned int location = glGetUniformLocation(shaderProgram, locationName);

	glUniformMatrix4fv(location, 1, GL_FALSE, (float *)m.values);

}

void GL3D_uniformMat4fArray(unsigned int shaderProgram, const char *locationName, Mat4f *matrices, int n_matrices){

	unsigned int location = glGetUniformLocation(shaderProgram, locationName);

	glUniformMatrix4fv(location, n_matrices, GL_FALSE, (float *)matrices);

}

void GL3D_uniformVec3f(unsigned int shaderProgram, const char *locationName, Vec3f v){

	unsigned int location = glGetUniformLocation(shaderProgram, locationName);

	glUniform3f(location, v.x, v.y, v.z);

}

void GL3D_uniformVec4f(unsigned int shaderProgram, const char *locationName, Vec4f v){

	unsigned int location = glGetUniformLocation(shaderProgram, locationName);

	glUniform4f(location, v.x, v.y, v.z, v.w);

}

void GL3D_uniformInt(unsigned int shaderProgram, const char *locationName, int x){

	unsigned int location = glGetUniformLocation(shaderProgram, locationName);

	glUniform1i(location, x);

}

void GL3D_uniformFloat(unsigned int shaderProgram, const char *locationName, float x){

	unsigned int location = glGetUniformLocation(shaderProgram, locationName);

	glUniform1f(location, x);

}

void GL3D_uniformTexture(unsigned int shaderProgram, const char *locationName, unsigned int locationIndex, unsigned int textureID){

	unsigned int location = glGetUniformLocation(shaderProgram, locationName);
	glUniform1i(location, locationIndex);
	glActiveTexture(GL_TEXTURE0 + locationIndex);
	glBindTexture(GL_TEXTURE_2D, textureID);

}

void GL3D_uniformTexture3D(unsigned int shaderProgram, const char *locationName, unsigned int locationIndex, unsigned int textureID){

	unsigned int location = glGetUniformLocation(shaderProgram, locationName);
	glUniform1i(location, locationIndex);
	glActiveTexture(GL_TEXTURE0 + locationIndex);
	glBindTexture(GL_TEXTURE_3D, textureID);

}

void GL3D_uniformTextureBuffer(unsigned int shaderProgram, const char *locationName, unsigned int locationIndex, unsigned int textureID){

	unsigned int location = glGetUniformLocation(shaderProgram, locationName);
	glUniform1i(location, locationIndex);
	glActiveTexture(GL_TEXTURE0 + locationIndex);
	glBindTexture(GL_TEXTURE_BUFFER, textureID);

}
