#ifndef H3D_H_
#define H3D_H_

#include "engine/geometry.h"
#include "engine/strings.h"

#ifdef _WIN32
#include "glad/wgl.h"
#endif
#include "glad/gl.h"

#include <vector>

enum Type3D{
	TYPE_I8,
	TYPE_I16,
	TYPE_F32,
	TYPE_F64,
};

typedef struct Model{
	char name[STRING_SIZE];
	unsigned int VBO;
	unsigned int VAO;
	unsigned int numberOfTriangles;
}Model;

struct Bone{
	char name[STRING_SIZE];
	int parent;
	Vec4f rotation;
	Vec3f scale;
	Vec3f translation;
};

typedef struct BoneModel{
	char name[STRING_SIZE];
	unsigned int VBO;
	unsigned int VAO;
	unsigned int n_triangles;
	std::vector<Bone> bones;
	std::vector<Mat4f> inverseBindMatrices;
}BoneModel;

typedef struct Texture{
	char name[STRING_SIZE];
	unsigned int ID;
}Texture;

struct Shader{
	char name[STRING_SIZE];
	unsigned int ID;
};

struct TextureAtlas{
	std::vector<SmallString> names;
	std::vector<Vec4f> textureCoordinates;
	Texture texture;
};

struct TextureBuffer{
	unsigned int VBO;
	unsigned int TB;
	unsigned int n_elements;
	unsigned int elementSize;
};

/*
typedef struct VertexMesh{
	Vec3f *vertices;
	int length;
}VertexMesh;
*/

struct TriangleMesh{
	Vec3f *triangles;
	int n_triangles;
	char name[STRING_SIZE];
};

struct PointMesh{
	Vec3f *points;
	int n_points;
	char name[STRING_SIZE];
};

static const int MODEL_COMPONENT_SIZE = sizeof(float) * 8;

static const int BONE_MODEL_COMPONENT_SIZE = sizeof(float) * 12 + sizeof(unsigned char) * 4;

void Model_initFromMeshData(Model *, const unsigned char *, int);

void Model_initFromFile_mesh(Model *, const char *);

void BoneModel_initFromFile(BoneModel *, const char *, const char *);

int BoneModel_getBoneIndexByName(BoneModel *, const char *);

std::vector<Bone> getInterpolatedBones(std::vector<Bone>, std::vector<Bone>, float);
std::vector<Mat4f> getBindMatricesFromBones(std::vector<Bone>);

unsigned char *generateMeshDataFromTriangleMesh(TriangleMesh, Vec2f *);

//void VertexMesh_initFromFile_mesh(VertexMesh *, const char *);

void TriangleMesh_initFromFile_mesh(TriangleMesh *, const char *);

void PointMesh_initFromTriangleMesh(PointMesh *, TriangleMesh);

void Texture_init(Texture *, const char *, unsigned char *, int, int);

void Texture_initFromFile(Texture *, const char *, const char *);

void Texture_initFromFileAsAlphaMap(Texture *, const char *, const char *);

void Texture_initAsDepthMap(Texture *, int, int);

void Texture_initAsColorMap(Texture *, int, int);

void TextureAtlas_init(TextureAtlas *, const char **, int);

void TextureBuffer_init(TextureBuffer *, void *, int);

void TextureBuffer_update(TextureBuffer *, int, int, void *);

void TextureBuffer_initAsVec4fArray(TextureBuffer *, Vec4f *, int);
void TextureBuffer_initAsMat4fArray(TextureBuffer *, Mat4f *, int, bool);

void TextureBuffer_free(TextureBuffer *);

void Texture_free(Texture *);

void Shader_init(Shader *, const char *, const char *, const char *);

void GL3D_uniformMat2f(unsigned int, const char *, Mat2f);

void GL3D_uniformMat4f(unsigned int, const char *, Mat4f);

void GL3D_uniformMat4fArray(unsigned int, const char *, Mat4f *, int);

void GL3D_uniformVec2f(unsigned int, const char *, Vec2f);

void GL3D_uniformVec3f(unsigned int, const char *, Vec3f);

void GL3D_uniformVec4f(unsigned int, const char *, Vec4f);

void GL3D_uniformInt(unsigned int, const char *, int);

void GL3D_uniformFloat(unsigned int, const char *, float);

void GL3D_uniformTexture(unsigned int, const char *, unsigned int, unsigned int);

void GL3D_uniformTexture3D(unsigned int, const char *, unsigned int, unsigned int);

void GL3D_uniformTextureBuffer(unsigned int, const char *, unsigned int, unsigned int);

bool checkClosestLineTriangleMeshIntersection(Vec3f, Vec3f, TriangleMesh, Vec3f, float, Vec3f *, int *);

#endif
