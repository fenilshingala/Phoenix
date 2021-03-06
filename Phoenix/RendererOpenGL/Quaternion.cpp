#include "Quaternion.h"

#include <math.h> 

Quaternion::Quaternion()
{
	this->x = 0.0f;
	this->y = 0.0f;
	this->z = 0.0f;
	this->w = 0.0f;
}

Quaternion::Quaternion(float x, float y, float z, float w)
{
	this->x = x;
	this->y = y;
	this->z = z;
	this->w = w;

	Normalize();
}

Quaternion::Quaternion(const aiQuaternion aiQuat)
{
	this->x = aiQuat.x;
	this->y = aiQuat.y;
	this->z = aiQuat.z;
	this->w = aiQuat.w;
}

aiQuaternion Quaternion::toAiQuaternion()
{
	aiQuaternion aiQuat;
	aiQuat.x = this->x;
	aiQuat.y = this->y;
	aiQuat.z = this->z;
	aiQuat.w = this->w;

	return aiQuat;
}

void Quaternion::Normalize()
{
	float mag = (float)sqrt(w * w + x * x + y * y + z * z);
	w /= mag;
	x /= mag;
	y /= mag;
	z /= mag;
}

Quaternion Quaternion::operator+(const Quaternion& rhs)
{
	return Quaternion(w + rhs.w, x + rhs.x, y + rhs.y, z + rhs.z);
}

Quaternion Quaternion::operator*(const Quaternion& rhs)
{
	Quaternion q;

	q.w = w * rhs.w - x * rhs.x - y * rhs.y - z * rhs.z;
	q.x = w * rhs.x + x * rhs.w + y * rhs.z - z * rhs.y;
	q.y = w * rhs.y - x * rhs.z + y * rhs.w + z * rhs.x;
	q.z = w * rhs.z + x * rhs.y - y * rhs.x + z * rhs.w;

	return q;
}

float Quaternion::Dot(const Quaternion& b)
{
	return (w * b.w + x * b.x * y * b.y + z * b.z);
}

Quaternion Quaternion::Inverse(Quaternion& rotation)
{
	return Quaternion(rotation.w, -1.0f * rotation.x, -1.0f * rotation.y, -1.0f * rotation.z);
}

Quaternion Quaternion::Conjugate()
{
	return Quaternion(w, -x, -y, -z);
}

Quaternion Quaternion::Rotate(float angle, glm::vec3& axis)
{
	glm::vec3 vn = axis / (float)axis.length();
	
	angle *= 0.0174532925f; // To radians!
	angle *= 0.5f;
	float sinAngle = sin(angle);

	return Quaternion(cos(angle), vn.x * sinAngle, vn.y * sinAngle, vn.z * sinAngle);
}

glm::mat4 Quaternion::toRotationMatrix()
{
	glm::mat4 matrix;
	float xy = x * y;
	float xz = x * z;
	float xw = x * w;
	float yz = y * z;
	float yw = y * w;
	float zw = z * w;
	float xSquared = x * x;
	float ySquared = y * y;
	float zSquared = z * z;
	matrix[0][0] = 1 - 2 * (ySquared + zSquared);
	matrix[0][1] = 2 * (xy - zw);
	matrix[0][2] = 2 * (xz + yw);
	matrix[0][3] = 0;
	matrix[1][0] = 2 * (xy + zw);
	matrix[1][1] = 1 - 2 * (xSquared + zSquared);
	matrix[1][2] = 2 * (yz - xw);
	matrix[1][3] = 0;
	matrix[2][0] = 2 * (xz - yw);
	matrix[2][1] = 2 * (yz + xw);
	matrix[2][2] = 1 - 2 * (xSquared + ySquared);
	matrix[2][3] = 0;
	matrix[3][0] = 0;
	matrix[3][1] = 0;
	matrix[3][2] = 0;
	matrix[3][3] = 1;

	return matrix;
}

aiMatrix4x4 Quaternion::toAiRotationMatrix()
{
	aiMatrix4x4 matrix;
	float xy = x * y;
	float xz = x * z;
	float xw = x * w;
	float yz = y * z;
	float yw = y * w;
	float zw = z * w;
	float xSquared = x * x;
	float ySquared = y * y;
	float zSquared = z * z;
	matrix.a1 = 1 - 2 * (ySquared + zSquared);
	matrix.a2 = 2 * (xy - zw);
	matrix.a3 = 2 * (xz + yw);
	matrix.a4 = 0;
	matrix.b1 = 2 * (xy + zw);
	matrix.b2 = 1 - 2 * (xSquared + zSquared);
	matrix.b3 = 2 * (yz - xw);
	matrix.b4 = 0;
	matrix.c1 = 2 * (xz - yw);
	matrix.c2 = 2 * (yz + xw);
	matrix.c3 = 1 - 2 * (xSquared + ySquared);
	matrix.c4 = 0;
	matrix.d1 = 0;
	matrix.d2 = 0;
	matrix.d3 = 0;
	matrix.d4 = 1;

	return matrix;
}

Quaternion Quaternion::fromMatrix(glm::mat4 matrix)
{
	float w, x, y, z;
	float diagonal = matrix[0][0] + matrix[1][1] + matrix[2][2];
	if (diagonal > 0)
	{
		float w4 = (float)(sqrt(diagonal + 1.0f) * 2.0f);
		w = w4 / 4.0f;
		x = (matrix[2][1] - matrix[1][2]) / w4;
		y = (matrix[0][2] - matrix[2][0]) / w4;
		z = (matrix[1][0] - matrix[0][1]) / w4;
	}
	else if ((matrix[0][0] > matrix[1][1]) && (matrix[0][0] > matrix[2][2]))
	{
		float x4 = (float)(sqrt(1.0f + matrix[0][0] - matrix[1][1] - matrix[2][2]) * 2.0f);
		w = (matrix[2][1] - matrix[1][2]) / x4;
		x = x4 / 4.0f;
		y = (matrix[0][1] + matrix[1][0]) / x4;
		z = (matrix[0][2] + matrix[2][0]) / x4;
	}
	else if (matrix[1][1] > matrix[2][2])
	{
		float y4 = (float)(sqrt(1.0f + matrix[1][1] - matrix[0][0] - matrix[2][2]) * 2.0f);
		w = (matrix[0][2] - matrix[2][0]) / y4;
		x = (matrix[0][1] + matrix[1][0]) / y4;
		y = y4 / 4.0f;
		z = (matrix[1][2] + matrix[2][1]) / y4;
	}
	else
	{
		float z4 = (float)(sqrt(1.0f + matrix[2][2] - matrix[0][0] - matrix[1][1]) * 2.0f);
		w = (matrix[1][0] - matrix[0][1]) / z4;
		x = (matrix[0][2] + matrix[2][0]) / z4;
		y = (matrix[1][2] + matrix[2][1]) / z4;
		z = z4 / 4.0f;
	}
	return Quaternion(x, y, z, w);
}

Quaternion Quaternion::interpolate(Quaternion a, Quaternion b, float blend)
{
	Quaternion result(0, 0, 0, 1);
	float dot = a.w * b.w + a.x * b.x + a.y * b.y + a.z * b.z;
	float blendI = 1.0f - blend;
	if (dot < 0)
	{
		result.w = blendI * a.w + blend * -b.w;
		result.x = blendI * a.x + blend * -b.x;
		result.y = blendI * a.y + blend * -b.y;
		result.z = blendI * a.z + blend * -b.z;
	}
	else
	{
		result.w = blendI * a.w + blend * b.w;
		result.x = blendI * a.x + blend * b.x;
		result.y = blendI * a.y + blend * b.y;
		result.z = blendI * a.z + blend * b.z;
	}
	result.Normalize();
	return result;
}