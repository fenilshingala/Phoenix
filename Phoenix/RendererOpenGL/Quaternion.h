#pragma once

#include <glm/glm.hpp>
#include <assimp/quaternion.h>

class Quaternion
{
public:
	Quaternion();
	Quaternion(float x, float y, float z, float w);
	Quaternion(const aiQuaternion);
	aiQuaternion toAiQuaternion();

	void Normalize();

	glm::mat4 toRotationMatrix();
	Quaternion fromMatrix(glm::mat4 matrix);
	static Quaternion interpolate(Quaternion a, Quaternion b, float blend);

private:
	float x, y, z, w;
};