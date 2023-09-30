#ifndef GEOMETRY_H_
#define GEOMETRY_H_

#include "stdbool.h"
#include "stdlib.h"
#include "string.h"

struct Vec2f{
	float x;
	float y;

	float& operator[] (int i){
		switch(i){
			case 0: return x;
			case 1: return y;
			default: throw "Coord index outside of vector bound";
		}
	}

	Vec2f operator+(Vec2f vIn){
		Vec2f vOut = { x + vIn.x, y + vIn.y };
		return vOut;
	}
	Vec2f operator-(Vec2f vIn){
		Vec2f vOut = { x - vIn.x, y - vIn.y };
		return vOut;
	}
	Vec2f operator*(float a){
		Vec2f vOut = { x * a, y * a };
		return vOut;
	}
	Vec2f operator/(float a){
		Vec2f vOut = { x / a, y / a };
		return vOut;
	}
	void operator+=(Vec2f vIn){
		x += vIn.x;
		y += vIn.y;
	}
	void operator-=(Vec2f vIn){
		x -= vIn.x;
		y -= vIn.y;
	}
	void operator*=(float a){
		x *= a;
		y *= a;
	}
	void operator/=(float a){
		x /= a;
		y /= a;
	}
};

struct Vec3f{
	float x;
	float y;
	float z;

	float& operator[] (int i){
		switch(i){
			case 0: return x;
			case 1: return y;
			case 2: return z;
			default: throw "Coord index outside of vector bound";
		}
	}

	Vec3f operator+(Vec3f vIn){
		Vec3f vOut = { x + vIn.x, y + vIn.y, z + vIn.z };
		return vOut;
	}
	Vec3f operator-(Vec3f vIn){
		Vec3f vOut = { x - vIn.x, y - vIn.y, z - vIn.z };
		return vOut;
	}
	Vec3f operator*(float a){
		Vec3f vOut = { x * a, y * a, z * a };
		return vOut;
	}
	Vec3f operator/(float a){
		Vec3f vOut = { x / a, y / a, z / a };
		return vOut;
	}
	void operator+=(Vec3f vIn){
		x += vIn.x;
		y += vIn.y;
		z += vIn.z;
	}
	void operator-=(Vec3f vIn){
		x -= vIn.x;
		y -= vIn.y;
		z -= vIn.z;
	}
	void operator*=(float a){
		x *= a;
		y *= a;
		z *= a;
	}
	void operator/=(float a){
		x /= a;
		y /= a;
		z /= a;
	}
};

struct Vec4f{
	float x;
	float y;
	float z;
	float w;

	float& operator[] (int i){
		switch(i){
			case 0: return x;
			case 1: return y;
			case 2: return z;
			case 3: return w;
			default: throw "Coord index outside of vector bound";
		}
	}

	Vec4f operator+(Vec4f vIn){
		Vec4f vOut = { x + vIn.x, y + vIn.y, z + vIn.z, w + vIn.w };
		return vOut;
	}
	Vec4f operator-(Vec4f vIn){
		Vec4f vOut = { x - vIn.x, y - vIn.y, z - vIn.z, w - vIn.w };
		return vOut;
	}
	Vec4f operator*(float a){
		Vec4f vOut = { x * a, y * a, z * a, w * a };
		return vOut;
	}
	Vec4f operator/(float a){
		Vec4f vOut = { x / a, y / a, z / a, w / a };
		return vOut;
	}
	void operator+=(Vec4f vIn){
		x += vIn.x;
		y += vIn.y;
		z += vIn.z;
		w += vIn.w;
	}
	void operator-=(Vec4f vIn){
		x -= vIn.x;
		y -= vIn.y;
		z -= vIn.z;
		w -= vIn.w;
	}
	void operator*=(float a){
		x *= a;
		y *= a;
		z *= a;
		w *= a;
	}
	void operator/=(float a){
		x /= a;
		y /= a;
		z /= a;
		w /= a;
	}
};

struct Mat2f{
	float values[2][2];

	float *operator[] (int i){
		return values[i];
	}
};

struct Mat3f{
	float values[3][3];

	float *operator[] (int i){
		return values[i];
	}
};

struct Mat4f{
	float values[4][4];

	float *operator[] (int i){
		return values[i];
	}

	Mat4f operator+ (Mat4f mIn){
		Mat4f newMatrix;
		memcpy(newMatrix.values, values, 16 * sizeof(float));

		for(int i = 0; i < 4; i++){
			for(int j = 0; j < 4; j++){
				newMatrix.values[i][j] += mIn.values[i][j];
			}
		}

		return newMatrix;
	}
	Mat4f operator* (Mat4f mIn){
		Mat4f newMatrix;
		memset(newMatrix.values, 0, 16 * sizeof(float));

		for(int newRow = 0; newRow < 4; newRow++){
			for(int newCol = 0; newCol < 4; newCol++){
				for(int i = 0; i < 4; i++){
					newMatrix.values[newRow][newCol] += values[newRow][i] * mIn.values[i][newCol];
				}
			}
		}

		return newMatrix;
	}

	Mat4f operator* (float a){
		Mat4f newMatrix;
		memcpy(newMatrix.values, values, 16 * sizeof(float));

		for(int i = 0; i < 4; i++){
			for(int j = 0; j < 4; j++){
				newMatrix[i][j] *= a;
			}
		}

		return newMatrix;
	}

	Vec4f operator* (Vec4f vIn){
		Vec4f newVec;

		for(int i = 0; i < 4; i++){
			newVec[i] = vIn.x * values[i][0] + vIn.y * values[i][1] + vIn.z * values[i][2] + vIn.w * values[i][3];
		}

		return newVec;
	}

	void operator*= (Mat4f vIn){
		Mat4f valueMatrix;
		memcpy(valueMatrix.values, values, 16 * sizeof(float));

		valueMatrix = vIn * valueMatrix;

		memcpy(values, valueMatrix.values, 16 * sizeof(float));
	}
};

//CONSTANTS

const Vec4f IDENTITY_QUATERNION = { 0.0, 0.0, 0.0, 1.0 };

//GENERAL MATH FUNCTIONS
float normalize(float);

float square(float);

float getRandom();

void setRandomSeed(int);

bool checkEqualsFloat(float, float, float);

int getSign(float);

int sign(float);

int min(int, int);

int max(int, int);

//VEC2F FUNCTIONS

void Vec2f_set(Vec2f *, float, float);

void Vec2f_log(Vec2f);

void Vec2f_add(Vec2f *, Vec2f);

void Vec2f_sub(Vec2f *, Vec2f);

void Vec2f_mul(Vec2f *, Vec2f);

void Vec2f_div(Vec2f *, Vec2f);

void Vec2f_mulByFloat(Vec2f *, float);

void Vec2f_divByFloat(Vec2f *, float);

void Vec2f_normalize(Vec2f *);

float getMagVec2f(Vec2f);

Vec2f getAddVec2f(Vec2f, Vec2f);

Vec2f getSubVec2f(Vec2f, Vec2f);

Vec2f getMulVec2fFloat(Vec2f, float);

Vec2f getDivVec2fFloat(Vec2f, float);

float getDistanceVec2f(Vec2f, Vec2f);

Vec2f getNormalizedVec2f(Vec2f);

Vec2f getInverseVec2f(Vec2f);

float getDotVec2f(Vec2f, Vec2f);

//VEC3F FUNCTIONS

void Vec3f_log(Vec3f);

void Vec3f_set(Vec3f *, float, float, float);

void Vec3f_add(Vec3f *, Vec3f);

void Vec3f_sub(Vec3f *, Vec3f);

void Vec3f_mulByFloat(Vec3f *, float);

void Vec3f_mulByVec3f(Vec3f *, Vec3f);

void Vec3f_divByFloat(Vec3f *, float);

void Vec3f_normalize(Vec3f *);

void Vec3f_inverse(Vec3f *);

void Vec3f_rotate(Vec3f *, float, float, float);

float getMagVec3f(Vec3f);

bool checkEqualsVec3f(Vec3f, Vec3f, float);

Vec3f getAddVec3f(Vec3f, Vec3f);

Vec3f getSubVec3f(Vec3f, Vec3f);

Vec3f getMulVec3fFloat(Vec3f, float);

Vec3f getDivVec3fFloat(Vec3f, float);

float getDistanceVec3f(Vec3f, Vec3f);

float getDotVec3f(Vec3f, Vec3f);

Vec3f getCrossVec3f(Vec3f, Vec3f);

float getAngleBetweenVec3f(Vec3f, Vec3f);

float getAreaFromTriangleVec3f(Vec3f, Vec3f, Vec3f);

Vec3f getNormalFromTriangleVec3f(Vec3f, Vec3f, Vec3f);

//void Vec3f_mulByMat4f(Vec3f *, Mat4f, float);
Vec3f mulVec3fMat4f(Vec3f, Mat4f, float);

//VEC4F FUNCTIONS

float getMagVec4f(Vec4f);

void Vec4f_log(Vec4f);

//void Vec4f_mulByMat4f(Vec4f *, Mat4f);

//MAT4F FUNCTIONS

//void Mat4f_mulByMat4f(Mat4f *, Mat4f);

//VECTOR FUNCTIONS

void plog(Vec2f);
void plog(Vec3f);
void plog(Vec4f);

Vec2f getVec2f(float, float);
Vec3f getVec3f(float, float, float);
Vec4f getVec4f(float, float, float, float);

Vec3f getVec3f(float);

float length(Vec2f);
float length(Vec3f);
float length(Vec4f);

Vec2f normalize(Vec2f);
Vec3f normalize(Vec3f);
Vec4f normalize(Vec4f);
Vec4f normalizeQuaternion(Vec4f);

float dot(Vec2f, Vec2f);
float dot(Vec3f, Vec3f);
float dot(Vec4f, Vec4f);

float lerp(float, float, float);
Vec2f lerp(Vec2f, Vec2f, float);
Vec3f lerp(Vec3f, Vec3f, float);
Vec4f lerp(Vec4f, Vec4f, float);

Vec3f cross(Vec3f, Vec3f);

Vec4f getQuaternion(Vec3f, float);

Vec4f mulQuaternions(Vec4f, Vec4f);

Vec3f getRotatedVec3f(Vec3f, Vec3f, float);

//MATRIX FUNCTIONS

void plog(Mat4f);

float det(Mat2f);
float det(Mat3f);
float det(Vec3f, Vec3f, Vec3f);
float det(Mat4f);

Mat2f getRotationMat2f(float);

Mat4f inverse(Mat4f);

Mat4f getIdentityMat4f();

//Mat4f getRotationMat4f(float, float, float);

Mat4f getTranslationMat4f(float, float, float);
Mat4f getTranslationMat4f(Vec3f);

Mat4f getScalingMat4f(float, float, float);
Mat4f getScalingMat4f(Vec3f);
Mat4f getScalingMat4f(float);

Mat4f getPerspectiveMat4f(float, float, float, float);

Mat4f getOrthographicMat4f(float, float, float, float);

Mat4f getLookAtMat4f(Vec3f, Vec3f);

Mat4f getQuaternionMat4f(Vec4f);

//GEOMETRIC FUNCTIONS

bool checkLineToTriangleIntersectionVec3f(Vec3f, Vec3f, Vec3f, Vec3f, Vec3f, Vec3f *);

bool checkSphereToTriangleCollision(Vec3f, float, Vec3f, Vec3f, Vec3f);

bool checkTriangleTriangleCollisionVec3f(Vec3f, Vec3f, Vec3f, Vec3f, Vec3f, Vec3f, Vec3f *);

#endif
