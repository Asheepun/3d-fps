#include "game.h"

#include "stdio.h"

TriangleMesh generateTerrainTriangleMesh(int width, float heightScale){

	Vec3f *points = (Vec3f *)malloc(sizeof(Vec3f) * width * width);

	for(int x = 0; x < width; x++){
		for(int y = 0; y < width; y++){

			int index = y * width + x;

			points[index].x = x;
			points[index].z = y;
			//terrainPoints[index].y = getRandom() / TERRAIN_SCALE * 2.0;
			points[index].y = getRandom() * heightScale;

			points[index].x /= (float)width;
			points[index].z /= (float)width;
		
		}
	}

	int n_triangles = 2 * (width - 1) * (width - 1);

	Vec3f *triangles = (Vec3f *)malloc(sizeof(Vec3f) * 3 * n_triangles);

	int triangleIndex = 0;

	for(int x = 0; x < width - 1; x++){
		for(int y = 0; y < width - 1; y++){

			int pointsIndex1 = y * width + x;
			int pointsIndex2 = (y + 1) * width + x;
			int pointsIndex3 = y * width + (x + 1);
			int pointsIndex4 = (y + 1) * width + (x + 1);

			Vec3f point1 = points[pointsIndex1];
			Vec3f point2 = points[pointsIndex2];
			Vec3f point3 = points[pointsIndex3];
			Vec3f point4 = points[pointsIndex4];

			triangles[triangleIndex * 3 * 2 + 0] = point1;
			triangles[triangleIndex * 3 * 2 + 1] = point2;
			triangles[triangleIndex * 3 * 2 + 2] = point4;
			triangles[triangleIndex * 3 * 2 + 3] = point1;
			triangles[triangleIndex * 3 * 2 + 4] = point4;
			triangles[triangleIndex * 3 * 2 + 5] = point3;

			triangleIndex++;

		}
	}

	free(points);
	
	TriangleMesh triangleMesh;
	triangleMesh.triangles = triangles;
	triangleMesh.n_triangles = n_triangles;
	String_set(triangleMesh.name, "terrain", STRING_SIZE);

	return triangleMesh;
	
}
