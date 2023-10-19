#ifndef H3D_NO_DRIVER_H_
#define H3D_NO_DRIVER_H_

#include "engine/geometry.h"
#include "engine/strings.h"

#include <vector>

struct TriangleMesh{
	Vec3f *triangles;
	float *triangleRadii;
	int n_triangles;
	char name[STRING_SIZE];
};

struct Bone{
	char name[STRING_SIZE];
	int parent;
	Vec4f rotation;
	Vec3f scale;
	Vec3f translation;
};

struct BoneRig{
	char name[STRING_SIZE];
	std::vector<Bone> originBones;
	std::vector<Mat4f> inverseBindMatrices;
};

struct BoneTriangleMesh{
	Vec3f *triangles;
	unsigned char *indices;
	Vec4f *weights;
	int n_triangles;
	char name[STRING_SIZE];
};

static const int MODEL_COMPONENT_SIZE = sizeof(float) * 8;

static const int BONE_MODEL_COMPONENT_SIZE = sizeof(float) * 12 + sizeof(unsigned char) * 4;

void TriangleMesh_initFromFile_mesh(TriangleMesh *, const char *);
void BoneTriangleMesh_initFromFile(BoneTriangleMesh *, const char *);
void BoneRig_initFromFile(BoneRig *, const char *);

void TriangleMesh_free(TriangleMesh *);

std::vector<Bone> getBonesFromFile(const char *);

std::vector<Mat4f> getBindMatricesFromBones(std::vector<Bone>);

std::vector<Bone> getInterpolatedBones(std::vector<Bone>, std::vector<Bone>, float);

std::vector<Mat4f> getBoneRigTransformations(BoneRig *, std::vector<Bone>);

#endif
