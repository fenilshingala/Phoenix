#pragma once

#include <glm/glm.hpp>
#include <assimp/quaternion.h>
#include <assimp/matrix4x4.h>

class Quaternion
{
public:
	Quaternion();
	Quaternion(float x, float y, float z, float w);
	Quaternion(const aiQuaternion);
	aiQuaternion toAiQuaternion();

	void Normalize();

	glm::mat4   toRotationMatrix();
	aiMatrix4x4 toAiRotationMatrix();

	Quaternion fromMatrix(glm::mat4 matrix);
	
	static Quaternion interpolate(Quaternion a, Quaternion b, float blend);

private:
	float x, y, z, w;
};