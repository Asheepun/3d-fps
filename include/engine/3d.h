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

typedef struct Texture{
	char name[STRING_SIZE];
	unsigned int ID;
}Texture;

struct TextureAtlas{
	std::vector<SmallString> names;
	std::vector<Vec4f> textureCoordinates;
	Texture texture;
};

struct TextureBuffer{
	unsigned int VBO;
	unsigned int TB;
};

typedef struct VertexMesh{
	Vec3f *vertices;
	int length;
}VertexMesh;

struct TriangleMesh{
	Vec3f *triangles;
	int n_triangles;
	char name[STRING_SIZE];
};

void Model_initFromMeshData(Model *, const unsigned char *, int);

void Model_initFromFile_mesh(Model *, const char *);

void VertexMesh_initFromFile_mesh(VertexMesh *, const char *);

void TriangleMesh_initFromFile_mesh(TriangleMesh *, const char *);

void Texture_init(Texture *, const char *, unsigned char *, int, int);

void Texture_initFromFile(Texture *, const char *, const char *);

void Texture_initAsDepthMap(Texture *, int, int);

void Texture_initAsColorMap(Texture *, int, int);

void TextureAtlas_init(TextureAtlas *, const char **, int);

void TextureBuffer_init(TextureBuffer *, void *, int, enum Type3D);

void TextureBuffer_free(TextureBuffer *);

void GL3D_uniformMat4f(unsigned int, const char *, Mat4f);

void GL3D_uniformVec3f(unsigned int, const char *, Vec3f);

void GL3D_uniformVec4f(unsigned int, const char *, Vec4f);

void GL3D_uniformInt(unsigned int, const char *, int);

void GL3D_uniformFloat(unsigned int, const char *, float);

void GL3D_uniformTexture(unsigned int, const char *, unsigned int, unsigned int);

void GL3D_uniformTexture3D(unsigned int, const char *, unsigned int, unsigned int);

void GL3D_uniformTextureBuffer(unsigned int, const char *, unsigned int, unsigned int);

#endif
