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
	Quaternion operator+(const Quaternion& rhs);
	Quaternion operator*(const Quaternion& rhs);
	float Dot(const Quaternion& b);
	Quaternion Inverse(Quaternion& rotation);
	Quaternion Conjugate();
	Quaternion Rotate(float angle, glm::vec3& axis);
		
	glm::mat4   toRotationMatrix();
	aiMatrix4x4 toAiRotationMatrix();

	Quaternion fromMatrix(glm::mat4 matrix);
	
	static Quaternion interpolate(Quaternion a, Quaternion b, float blend);

private:
	float x, y, z, w;
};