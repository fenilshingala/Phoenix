#include "RendererOpenGL.h"

#include <assert.h>
#include <sstream>
#include <fstream>
#include <functional>

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

#include <iostream>
#include "../../Common/Thirdparty/TINYSTL/unordered_map.h"

#include "Quaternion.h"

//////////////////////////////////////////////////////////
// CAMERA

Camera::Camera(glm::vec3 position, glm::vec3 up, float yaw, float pitch) : Front(glm::vec3(0.0f, 0.0f, -1.0f)), MovementSpeed(SPEED), MouseSensitivity(SENSITIVITY), Zoom(ZOOM)
{
	Position = position;
	WorldUp = up;
	Yaw = yaw;
	Pitch = pitch;
	updateCameraVectors();
}

Camera::Camera(float posX, float posY, float posZ, float upX, float upY, float upZ, float yaw, float pitch) : Front(glm::vec3(0.0f, 0.0f, -1.0f)), MovementSpeed(SPEED), MouseSensitivity(SENSITIVITY), Zoom(ZOOM)
{
	Position = glm::vec3(posX, posY, posZ);
	WorldUp = glm::vec3(upX, upY, upZ);
	Yaw = yaw;
	Pitch = pitch;
	updateCameraVectors();
}

glm::mat4 Camera::GetViewMatrix()
{
	return glm::lookAt(Position, Position + Front, Up);
}

void Camera::ProcessKeyboard(Camera_Movement direction, float deltaTime)
{
	float velocity = MovementSpeed * deltaTime;
	if (direction == FORWARD)
		Position += Front * velocity;
	if (direction == BACKWARD)
		Position -= Front * velocity;
	if (direction == LEFT)
		Position -= Right * velocity;
	if (direction == RIGHT)
		Position += Right * velocity;
}

void Camera::ProcessMouseMovement(float xoffset, float yoffset, GLboolean constrainPitch)
{
	xoffset *= MouseSensitivity;
	yoffset *= MouseSensitivity;

	Yaw += xoffset;
	Pitch += yoffset;

	// Make sure that when pitch is out of bounds, screen doesn't get flipped
	if (constrainPitch)
	{
		if (Pitch > 89.0f)
			Pitch = 89.0f;
		if (Pitch < -89.0f)
			Pitch = -89.0f;
	}

	// Update Front, Right and Up Vectors using the updated Euler angles
	updateCameraVectors();
}

void Camera::ProcessMouseScroll(float yoffset)
{
	if (Zoom >= 1.0f && Zoom <= 45.0f)
		Zoom -= yoffset;
	if (Zoom <= 1.0f)
		Zoom = 1.0f;
	if (Zoom >= 45.0f)
		Zoom = 45.0f;
}

void Camera::updateCameraVectors()
{
	// Calculate the new Front vector
	glm::vec3 front;
	front.x = cos(glm::radians(Yaw)) * cos(glm::radians(Pitch));
	front.y = sin(glm::radians(Pitch));
	front.z = sin(glm::radians(Yaw)) * cos(glm::radians(Pitch));
	Front = glm::normalize(front);
	// Also re-calculate the Right and Up vector
	Right = glm::normalize(glm::cross(Front, WorldUp));  // Normalize the vectors, because their length gets closer to 0 the more you look up or down which results in slower movement.
	Up = glm::normalize(glm::cross(Right, Front));
}


//////////////////////////////////////////////////////////
// OPENGL 

std::hash<std::string> hasher;

ShaderProgram::ShaderProgram(std::string vertexShaderPath, std::string fragmentShaderPath)
{
	std::string line;
	std::stringstream ss[2];
	int vertexShader, fragmentShader;

	// vert
	{
		std::ifstream vertStream(vertexShaderPath.c_str());
		while (getline(vertStream, line))
		{
			ss[0] << line << '\n';
		}
		std::string vertexShaderSource = ss[0].str();
		const char* charData = vertexShaderSource.c_str();

		vertexShader = glCreateShader(GL_VERTEX_SHADER);
		glShaderSource(vertexShader, 1, &charData, NULL);
		glCompileShader(vertexShader);
		
		int success;
		glGetShaderiv(vertexShader, GL_COMPILE_STATUS, &success);
		assert(success);
	}

	// frag
	{
		std::ifstream fragStream(fragmentShaderPath.c_str());
		while (getline(fragStream, line))
		{
			ss[1] << line << '\n';
		}
		std::string fragmentShaderSource = ss[1].str().c_str();
		const char* charData = fragmentShaderSource.c_str();

		fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
		glShaderSource(fragmentShader, 1, &charData, NULL);
		glCompileShader(fragmentShader);
		
		int success;
		glGetShaderiv(fragmentShader, GL_COMPILE_STATUS, &success);
		assert(success);
	}

	mId = glCreateProgram();
	glAttachShader(mId, vertexShader);
	glAttachShader(mId, fragmentShader);
	glLinkProgram(mId);
	
	int success;
	glGetProgramiv(mId, GL_LINK_STATUS, &success);
	assert(success);
	glDeleteShader(vertexShader);
	glDeleteShader(fragmentShader);

	int32_t count, length, size;
	GLenum type;
	const uint32_t bufferSize = 256;
	char name[bufferSize] = {};

	glGetProgramiv(mId, GL_ACTIVE_ATTRIBUTES, &count);
	for (int32_t i = 0; i < count; ++i)
	{
		glGetActiveAttrib(mId, i, bufferSize, &length, &size, &type, name);
	}

	glGetProgramiv(mId, GL_ACTIVE_UNIFORMS, &count);
	for (int32_t i = 0; i < count; ++i)
	{
		glGetActiveUniform(mId, i, bufferSize, &length, &size, &type, name);
		Uniform uniformInfo;
		uniformInfo.location = glGetUniformLocation(mId, name);
		uniformInfo.type = type;
		mUniformVarMap.insert( {hasher(std::string(name)), uniformInfo} );
	}
}

ShaderProgram::~ShaderProgram()
{
	glDeleteProgram(mId);
}

void ShaderProgram::SetUniform(const char* name, void* data)
{
	std::unordered_map<size_t, Uniform>::iterator itr = mUniformVarMap.find( hasher(std::string(name)) );
	if (itr != mUniformVarMap.end())
	{
		Uniform uniformInfo = itr->second;
		switch (uniformInfo.type)
		{
			// FLOAT
		case GL_FLOAT:
			glUniform1fv(uniformInfo.location, 1, (float*)data);
			break;
		case GL_FLOAT_VEC2:
			glUniform2fv(uniformInfo.location, 1, (float*)data);
			break;
		case GL_FLOAT_VEC3:
			glUniform3fv(uniformInfo.location, 1, (float*)data);
			break;
		case GL_FLOAT_VEC4:
			glUniform4fv(uniformInfo.location, 1, (float*)data);
			break;

			// DOUBLE
		case GL_DOUBLE:
			glUniform1dv(uniformInfo.location, 1, (double*)data);
			break;
		case GL_DOUBLE_VEC2:
			glUniform2dv(uniformInfo.location, 1, (double*)data);
			break;
		case GL_DOUBLE_VEC3:
			glUniform3dv(uniformInfo.location, 1, (double*)data);
			break;
		case GL_DOUBLE_VEC4:
			glUniform4dv(uniformInfo.location, 1, (double*)data);
			break;
		
			// BOOL AND INT
		case GL_BOOL:
		case GL_INT:
			glUniform1iv(uniformInfo.location, 1, (int*)data);
			break;

		case GL_BOOL_VEC2:
		case GL_INT_VEC2:
			glUniform2iv(uniformInfo.location, 1, (int*)data);
			break;

		case GL_BOOL_VEC3:
		case GL_INT_VEC3:
			glUniform3iv(uniformInfo.location, 1, (int*)data);
			break;

		case GL_BOOL_VEC4:
		case GL_INT_VEC4:
			glUniform4iv(uniformInfo.location, 1, (int*)data);
			break;

			// UNSIGNED INT
		case GL_UNSIGNED_INT:
			glUniform1uiv(uniformInfo.location, 1, (unsigned int*)data);
			break;
		case GL_UNSIGNED_INT_VEC2:
			glUniform2uiv(uniformInfo.location, 1, (unsigned int*)data);
			break;
		case GL_UNSIGNED_INT_VEC3:
			glUniform3uiv(uniformInfo.location, 1, (unsigned int*)data);
			break;
		case GL_UNSIGNED_INT_VEC4:
			glUniform4uiv(uniformInfo.location, 1, (unsigned int*)data);
			break;

			// FLOAT MATRIX
		case GL_FLOAT_MAT2:
			glUniformMatrix2fv(uniformInfo.location, 1, GL_FALSE, (float*)data);
			break;
		case GL_FLOAT_MAT3:
			glUniformMatrix3fv(uniformInfo.location, 1, GL_FALSE, (float*)data);
			break;
		case GL_FLOAT_MAT4:
			glUniformMatrix4fv(uniformInfo.location, 1, GL_FALSE, (float*)data);
			break;
		case GL_FLOAT_MAT2x3:
			glUniformMatrix2x3fv(uniformInfo.location, 1, GL_FALSE, (float*)data);
			break;
		case GL_FLOAT_MAT2x4:
			glUniformMatrix2x4fv(uniformInfo.location, 1, GL_FALSE, (float*)data);
			break;
		case GL_FLOAT_MAT3x2:
			glUniformMatrix3x2fv(uniformInfo.location, 1, GL_FALSE, (float*)data);
			break;
		case GL_FLOAT_MAT3x4:
			glUniformMatrix3x4fv(uniformInfo.location, 1, GL_FALSE, (float*)data);
			break;
		case GL_FLOAT_MAT4x2:
			glUniformMatrix4x2fv(uniformInfo.location, 1, GL_FALSE, (float*)data);
			break;
		case GL_FLOAT_MAT4x3:
			glUniformMatrix4x3fv(uniformInfo.location, 1, GL_FALSE, (float*)data);
			break;

			// DOUBLE MATRIX
		case GL_DOUBLE_MAT2:
			glUniformMatrix2dv(uniformInfo.location, 1, GL_FALSE, (double*)data);
			break;
		case GL_DOUBLE_MAT3:
			glUniformMatrix3dv(uniformInfo.location, 1, GL_FALSE, (double*)data);
			break;
		case GL_DOUBLE_MAT4:
			glUniformMatrix4dv(uniformInfo.location, 1, GL_FALSE, (double*)data);
			break;
		case GL_DOUBLE_MAT2x3:
			glUniformMatrix2x3dv(uniformInfo.location, 1, GL_FALSE, (double*)data);
			break;
		case GL_DOUBLE_MAT2x4:
			glUniformMatrix2x4dv(uniformInfo.location, 1, GL_FALSE, (double*)data);
			break;
		case GL_DOUBLE_MAT3x2:
			glUniformMatrix3x2dv(uniformInfo.location, 1, GL_FALSE, (double*)data);
			break;
		case GL_DOUBLE_MAT3x4:
			glUniformMatrix3x4dv(uniformInfo.location, 1, GL_FALSE, (double*)data);
			break;
		case GL_DOUBLE_MAT4x2:
			glUniformMatrix4x2dv(uniformInfo.location, 1, GL_FALSE, (double*)data);
			break;
		case GL_DOUBLE_MAT4x3:
			glUniformMatrix4x3dv(uniformInfo.location, 1, GL_FALSE, (double*)data);
			break;

			// SAMPLERS
		case GL_SAMPLER_1D:
		case GL_SAMPLER_2D:
		case GL_SAMPLER_CUBE:
			glUniform1iv(uniformInfo.location, 1, (int*)data);
			break;
		case GL_SAMPLER_3D: break;
		
			// SAMPLER ARRAYS
		case GL_SAMPLER_1D_ARRAY: break;
		case GL_SAMPLER_2D_ARRAY: break;
		default: break;
		}
	}
}

uint32_t LoadTexture(const char* path, bool isHDR)
{
	unsigned int texture;
	glGenTextures(1, &texture);
	glBindTexture(GL_TEXTURE_2D, texture);
	
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	
	int width, height, nrChannels;
	stbi_set_flip_vertically_on_load(true);
	unsigned char *data = stbi_load(path, &width, &height, &nrChannels, 0);
	
	assert(data);
	
	int internal_format = 0;
	switch (nrChannels)
	{
	case 4: internal_format = GL_RGBA; break;
	case 3: internal_format = GL_RGB;  break;
	case 2: internal_format = GL_RG;   break;
	case 1: internal_format = GL_RED;  break;
	default: assert(0); break;
	}

	if (isHDR)
	{
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB16F, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, data);
	}
	else
	{
		glTexImage2D(GL_TEXTURE_2D, 0, internal_format, width, height, 0, internal_format, GL_UNSIGNED_BYTE, data);
	}
	
	glGenerateMipmap(GL_TEXTURE_2D);
	
	stbi_image_free(data);

	return texture;
}


#pragma region BASIC_SHAPES

//////////////////////////////////////////////// INSTANCE VBO
void OpenGLRenderer::attachInstanceVBO(unsigned int& vbo)
{
	//glGenBuffers(1, &vbo);
	glBindBuffer(GL_ARRAY_BUFFER, vbo);

	glEnableVertexAttribArray(3);
	glVertexAttribPointer(3, 4, GL_FLOAT, GL_FALSE, 19 * sizeof(float), (void*)0);
	glVertexAttribDivisor(3, 1); // tell OpenGL this is an instanced vertex attribute.

	glEnableVertexAttribArray(4);
	glVertexAttribPointer(4, 4, GL_FLOAT, GL_FALSE, 19 * sizeof(float), (void*)(4 * sizeof(float)));
	glVertexAttribDivisor(4, 1); // tell OpenGL this is an instanced vertex attribute.

	glEnableVertexAttribArray(5);
	glVertexAttribPointer(5, 4, GL_FLOAT, GL_FALSE, 19 * sizeof(float), (void*)(8 * sizeof(float)));
	glVertexAttribDivisor(5, 1); // tell OpenGL this is an instanced vertex attribute.

	glEnableVertexAttribArray(6);
	glVertexAttribPointer(6, 4, GL_FLOAT, GL_FALSE, 19 * sizeof(float), (void*)(12 * sizeof(float)));
	glVertexAttribDivisor(6, 1); // tell OpenGL this is an instanced vertex attribute.

	glEnableVertexAttribArray(7);
	glVertexAttribPointer(7, 3, GL_FLOAT, GL_FALSE, 19 * sizeof(float), (void*)(16 * sizeof(float)));
	glVertexAttribDivisor(7, 1); // tell OpenGL this is an instanced vertex attribute.

	glBindBuffer(GL_ARRAY_BUFFER, 0);
}

void OpenGLRenderer::setupBasicShapeBuffers(unsigned int vao, unsigned int instanceVBO)
{
	glBindVertexArray(vao);
	{
		glEnableVertexAttribArray(0);
		glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)0);
		glEnableVertexAttribArray(1);
		glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(3 * sizeof(float)));
		glEnableVertexAttribArray(2);
		glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(6 * sizeof(float)));

		if (instanceVBO != INVALID_BUFFER_ID)
		{
			attachInstanceVBO(instanceVBO);
		}
	}
	glBindVertexArray(0);
}

//////////////////////////////////////////////// LINE
float lineVertices[] = {
	// positions       
	0.0f, 0.0f, 0.0f,
	1.0f, 0.0f, 0.0f
};

void OpenGLRenderer::setupLine()
{
	// setup plane VAO
	glGenVertexArrays(1, &mLineVAO);
	glGenBuffers(1, &mLineVBO);
	glBindVertexArray(mLineVAO);
	glBindBuffer(GL_ARRAY_BUFFER, mLineVBO);
	glBufferData(GL_ARRAY_BUFFER, sizeof(lineVertices), &lineVertices, GL_STATIC_DRAW);
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
}


//////////////////////////////////////////////// QUAD
float quadVertices[] = {
	// positions		// normals		  // texture Coords
	-1.0f,  1.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 1.0f,
	-1.0f, -1.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f,
	 1.0f,  1.0f, 0.0f, 0.0f, 0.0f, 1.0f, 1.0f, 1.0f,
	 1.0f, -1.0f, 0.0f, 0.0f, 0.0f, 1.0f, 1.0f, 0.0f,
};

void OpenGLRenderer::setupQuad()
{
	// setup plane VAO
	glGenVertexArrays(1, &mQuadVAO);
	glGenBuffers(1, &mQuadVBO);
	glBindBuffer(GL_ARRAY_BUFFER, mQuadVBO);
	glBufferData(GL_ARRAY_BUFFER, sizeof(quadVertices), &quadVertices, GL_STATIC_DRAW);
	setupBasicShapeBuffers(mQuadVAO, INVALID_BUFFER_ID);

	glGenVertexArrays(1, &quadInstanceVAO);
	glBindBuffer(GL_ARRAY_BUFFER, mQuadVBO);
	setupBasicShapeBuffers(quadInstanceVAO, quadInstanceBuffer);
}


//////////////////////////////////////////////// CUBE
float cubeVertices[] = {
	// Position			  Normals			  TexCoords
	// back face
	-1.0f, -1.0f, -1.0f,  0.0f,  0.0f, -1.0f, 0.0f, 0.0f, // bottom-left
	 1.0f,  1.0f, -1.0f,  0.0f,  0.0f, -1.0f, 1.0f, 1.0f, // top-right
	 1.0f, -1.0f, -1.0f,  0.0f,  0.0f, -1.0f, 1.0f, 0.0f, // bottom-right         
	 1.0f,  1.0f, -1.0f,  0.0f,  0.0f, -1.0f, 1.0f, 1.0f, // top-right
	-1.0f, -1.0f, -1.0f,  0.0f,  0.0f, -1.0f, 0.0f, 0.0f, // bottom-left
	-1.0f,  1.0f, -1.0f,  0.0f,  0.0f, -1.0f, 0.0f, 1.0f, // top-left
	// front face
	-1.0f, -1.0f,  1.0f,  0.0f,  0.0f,  1.0f, 0.0f, 0.0f, // bottom-left
	 1.0f, -1.0f,  1.0f,  0.0f,  0.0f,  1.0f, 1.0f, 0.0f, // bottom-right
	 1.0f,  1.0f,  1.0f,  0.0f,  0.0f,  1.0f, 1.0f, 1.0f, // top-right
	 1.0f,  1.0f,  1.0f,  0.0f,  0.0f,  1.0f, 1.0f, 1.0f, // top-right
	-1.0f,  1.0f,  1.0f,  0.0f,  0.0f,  1.0f, 0.0f, 1.0f, // top-left
	-1.0f, -1.0f,  1.0f,  0.0f,  0.0f,  1.0f, 0.0f, 0.0f, // bottom-left
	// left face
	-1.0f,  1.0f,  1.0f, -1.0f,  0.0f,  0.0f, 1.0f, 0.0f, // top-right
	-1.0f,  1.0f, -1.0f, -1.0f,  0.0f,  0.0f, 1.0f, 1.0f, // top-left
	-1.0f, -1.0f, -1.0f, -1.0f,  0.0f,  0.0f, 0.0f, 1.0f, // bottom-left
	-1.0f, -1.0f, -1.0f, -1.0f,  0.0f,  0.0f, 0.0f, 1.0f, // bottom-left
	-1.0f, -1.0f,  1.0f, -1.0f,  0.0f,  0.0f, 0.0f, 0.0f, // bottom-right
	-1.0f,  1.0f,  1.0f, -1.0f,  0.0f,  0.0f, 1.0f, 0.0f, // top-right
	// right face
	 1.0f,  1.0f,  1.0f,  1.0f,  0.0f,  0.0f, 1.0f, 0.0f, // top-left
	 1.0f, -1.0f, -1.0f,  1.0f,  0.0f,  0.0f, 0.0f, 1.0f, // bottom-right
	 1.0f,  1.0f, -1.0f,  1.0f,  0.0f,  0.0f, 1.0f, 1.0f, // top-right         
	 1.0f, -1.0f, -1.0f,  1.0f,  0.0f,  0.0f, 0.0f, 1.0f, // bottom-right
	 1.0f,  1.0f,  1.0f,  1.0f,  0.0f,  0.0f, 1.0f, 0.0f, // top-left
	 1.0f, -1.0f,  1.0f,  1.0f,  0.0f,  0.0f, 0.0f, 0.0f, // bottom-left     
	// bottom face
	-1.0f, -1.0f, -1.0f,  0.0f, -1.0f,  0.0f, 0.0f, 1.0f, // top-right
	 1.0f, -1.0f, -1.0f,  0.0f, -1.0f,  0.0f, 1.0f, 1.0f, // top-left
	 1.0f, -1.0f,  1.0f,  0.0f, -1.0f,  0.0f, 1.0f, 0.0f, // bottom-left
	 1.0f, -1.0f,  1.0f,  0.0f, -1.0f,  0.0f, 1.0f, 0.0f, // bottom-left
	-1.0f, -1.0f,  1.0f,  0.0f, -1.0f,  0.0f, 0.0f, 0.0f, // bottom-right
	-1.0f, -1.0f, -1.0f,  0.0f, -1.0f,  0.0f, 0.0f, 1.0f, // top-right
	// top face
	-1.0f,  1.0f, -1.0f,  0.0f,  1.0f,  0.0f, 0.0f, 1.0f, // top-left
	 1.0f,  1.0f , 1.0f,  0.0f,  1.0f,  0.0f, 1.0f, 0.0f, // bottom-right
	 1.0f,  1.0f, -1.0f,  0.0f,  1.0f,  0.0f, 1.0f, 1.0f, // top-right     
	 1.0f,  1.0f,  1.0f,  0.0f,  1.0f,  0.0f, 1.0f, 0.0f, // bottom-right
	-1.0f,  1.0f, -1.0f,  0.0f,  1.0f,  0.0f, 0.0f, 1.0f, // top-left
	-1.0f,  1.0f,  1.0f,  0.0f,  1.0f,  0.0f, 0.0f, 0.0f  // bottom-left        
};

void OpenGLRenderer::setupCube()
{
	glGenVertexArrays(1, &mCubeVAO);
	glGenBuffers(1, &mCubeVBO);
	glBindBuffer(GL_ARRAY_BUFFER, mCubeVBO);
	glBufferData(GL_ARRAY_BUFFER, sizeof(cubeVertices), cubeVertices, GL_STATIC_DRAW);
	setupBasicShapeBuffers(mCubeVAO, INVALID_BUFFER_ID);
	
	// INSTANCING
	glGenVertexArrays(1, &cubeInstanceVAO);
	glBindBuffer(GL_ARRAY_BUFFER, mCubeVBO);
	setupBasicShapeBuffers(cubeInstanceVAO, cubeInstanceBuffer);
}


//////////////////////////////////////////////// SPHERE
void OpenGLRenderer::setupSphere()
{
	glGenVertexArrays(1, &sphereVAO);

	unsigned int vbo, ebo;
	glGenBuffers(1, &vbo);
	glGenBuffers(1, &ebo);

	std::vector<glm::vec3> positions;
	std::vector<glm::vec2> uv;
	std::vector<glm::vec3> normals;
	std::vector<unsigned int> indices;

	const unsigned int X_SEGMENTS = 64;
	const unsigned int Y_SEGMENTS = 64;
	const float PI = 3.14159265359f;
	for (unsigned int y = 0; y <= Y_SEGMENTS; ++y)
	{
		for (unsigned int x = 0; x <= X_SEGMENTS; ++x)
		{
			float xSegment = (float)x / (float)X_SEGMENTS;
			float ySegment = (float)y / (float)Y_SEGMENTS;
			float xPos = std::cos(xSegment * 2.0f * PI) * std::sin(ySegment * PI);
			float yPos = std::cos(ySegment * PI);
			float zPos = std::sin(xSegment * 2.0f * PI) * std::sin(ySegment * PI);

			positions.push_back(glm::vec3(xPos, yPos, zPos));
			uv.push_back(glm::vec2(xSegment, ySegment));
			normals.push_back(glm::vec3(xPos, yPos, zPos));
		}
	}

	bool oddRow = false;
	for (int y = 0; y < Y_SEGMENTS; ++y)
	{
		if (!oddRow) // even rows: y == 0, y == 2; and so on
		{
			for (int x = 0; x <= X_SEGMENTS; ++x)
			{
				indices.push_back(y       * (X_SEGMENTS + 1) + x);
				indices.push_back((y + 1) * (X_SEGMENTS + 1) + x);
			}
		}
		else
		{
			for (int x = X_SEGMENTS; x >= 0; --x)
			{
				indices.push_back((y + 1) * (X_SEGMENTS + 1) + x);
				indices.push_back(y       * (X_SEGMENTS + 1) + x);
			}
		}
		oddRow = !oddRow;
	}
	sphereIndexCount = (unsigned int)indices.size();

	std::vector<float> data;
	for (int i = 0; i < positions.size(); ++i)
	{
		data.push_back(positions[i].x);
		data.push_back(positions[i].y);
		data.push_back(positions[i].z);
		if (normals.size() > 0)
		{
			data.push_back(normals[i].x);
			data.push_back(normals[i].y);
			data.push_back(normals[i].z);
		}
		if (uv.size() > 0)
		{
			data.push_back(uv[i].x);
			data.push_back(uv[i].y);
		}
	}
	glBindVertexArray(sphereVAO);
	glBindBuffer(GL_ARRAY_BUFFER, vbo);
	glBufferData(GL_ARRAY_BUFFER, data.size() * sizeof(float), &data[0], GL_STATIC_DRAW);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(unsigned int), &indices[0], GL_STATIC_DRAW);
	setupBasicShapeBuffers(sphereVAO, INVALID_BUFFER_ID);

	glGenVertexArrays(1, &sphereInstanceVAO);
	glBindVertexArray(sphereInstanceVAO);
	glBindBuffer(GL_ARRAY_BUFFER, vbo);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);
	setupBasicShapeBuffers(sphereInstanceVAO, sphereInstanceBuffer);
}


//////////////////////////////////////////////// INSTANCE BUFFER UPDATE
void OpenGLRenderer::UpdateQuadInstanceBuffer(uint32_t size, void* data)
{
	glBindBuffer(GL_ARRAY_BUFFER, quadInstanceBuffer);
	glBufferData(GL_ARRAY_BUFFER, size, data, GL_STATIC_DRAW);
	glBindBuffer(GL_ARRAY_BUFFER, 0);
}

void OpenGLRenderer::UpdateCubeInstanceBuffer(uint32_t size, void* data)
{
	glBindBuffer(GL_ARRAY_BUFFER, cubeInstanceBuffer);
	glBufferData(GL_ARRAY_BUFFER, size, data, GL_STATIC_DRAW);
	glBindBuffer(GL_ARRAY_BUFFER, 0);
}

void OpenGLRenderer::UpdateSphereInstanceBuffer(uint32_t size, void* data)
{
	glBindBuffer(GL_ARRAY_BUFFER, sphereInstanceBuffer);
	glBufferData(GL_ARRAY_BUFFER, size, data, GL_STATIC_DRAW);
	glBindBuffer(GL_ARRAY_BUFFER, 0);
}


OpenGLRenderer::OpenGLRenderer()
{
	glGenBuffers(1, &quadInstanceBuffer);
	glGenBuffers(1, &cubeInstanceBuffer);
	glGenBuffers(1, &sphereInstanceBuffer);

	setupLine();
	setupQuad();
	setupCube();
	setupSphere();
}

OpenGLRenderer::~OpenGLRenderer()
{
	glDeleteVertexArrays(1, &sphereInstanceVAO);
	glDeleteVertexArrays(1, &sphereVAO);
	glDeleteVertexArrays(1, &cubeInstanceVAO);
	glDeleteVertexArrays(1, &mCubeVAO);
	glDeleteVertexArrays(1, &quadInstanceVAO);
	glDeleteVertexArrays(1, &mQuadVAO);
}

void OpenGLRenderer::RenderLine()
{
	glBindVertexArray(mLineVAO);
	glDrawArrays(GL_LINES, 0, 2);
	glBindVertexArray(0);
}

void OpenGLRenderer::RenderQuad()
{
	glBindVertexArray(mQuadVAO);
	glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
	glBindVertexArray(0);
}

void OpenGLRenderer::RenderQuadInstanced(int numOfInstances)
{
	glBindVertexArray(quadInstanceVAO);
	glDrawArraysInstanced(GL_TRIANGLE_STRIP, 0, 4, numOfInstances);
	glBindVertexArray(0);
}

void OpenGLRenderer::RenderCube()
{
	glBindVertexArray(mCubeVAO);
	glDrawArrays(GL_TRIANGLES, 0, 36);
	glBindVertexArray(0);
}

void OpenGLRenderer::RenderCubeInstanced(int numOfInstances)
{
	glBindVertexArray(cubeInstanceVAO);
	glDrawArraysInstanced(GL_TRIANGLES, 0, 36, numOfInstances);
	glBindVertexArray(0);
}

void OpenGLRenderer::RenderSphere()
{
	glBindVertexArray(sphereVAO);
	glDrawElements(GL_TRIANGLE_STRIP, sphereIndexCount, GL_UNSIGNED_INT, 0);
	glBindVertexArray(0);
}

void OpenGLRenderer::RenderSphereInstanced(int numOfInstances)
{
	glBindVertexArray(sphereInstanceVAO);
	glDrawElementsInstanced(GL_TRIANGLE_STRIP, sphereIndexCount, GL_UNSIGNED_INT, 0, numOfInstances);
	glBindVertexArray(0);
}

#pragma endregion BASIC_SHAPES


glm::mat4 OpenGLRenderer::ModelMatForLineBWTwoPoints(glm::vec3 A, glm::vec3 B)
{
	float scale = glm::distance(A, B);
	glm::mat4 rot = glm::mat4(1.0f);
	// Calculate angles using the direction vector
	glm::vec3 dir = B - A;
	dir.z *= -1.0f;
	double y = atan2(dir.z, sqrt(dir.x*dir.x + dir.y*dir.y));
	double z = atan2(dir.y, dir.x);
	rot = glm::mat4_cast(glm::quat(glm::vec3(0, y, z)));

	// world space
	if (scale == 0)
		return glm::mat4(0.0f);

	return glm::translate(glm::mat4(1.0f), glm::vec3(A)) * rot * glm::scale(glm::mat4(1.0f), glm::vec3(scale, scale, scale));
}


#define POSITION_LOCATION    0
#define NORMAL_LOCATION      1
#define TEX_COORD_LOCATION   2
#define BONE_ID_LOCATION     3
#define BONE_WEIGHT_LOCATION 4

void SkinnedMesh::VertexBoneData::AddBoneData(uint32_t BoneID, float Weight)
{
 	uint32_t size = sizeof(IDs) / sizeof(IDs[0]);
	for (uint32_t i = 0; i < size; i++)
	{
		if (Weights[i] == 0.0)
		{
			IDs[i] = BoneID;
			Weights[i] = Weight;
			return;
		}
	}

	// should never get here - more bones than we have space for
	assert(0);
}

SkinnedMesh::SkinnedMesh()
{
	m_VAO = 0;
	memset(m_Buffers, 0, sizeof(m_Buffers) / sizeof(m_Buffers[0]));
	m_NumBones = 0;
	m_pScene = NULL;
}


SkinnedMesh::~SkinnedMesh()
{
	//for (uint32_t i = 0; i < m_Textures.size(); i++)
	//{
	//	//SAFE_DELETE(m_Textures[i]);
	//}

	if (m_Buffers[0] != 0)
	{
		glDeleteBuffers(sizeof(m_Buffers) / sizeof(m_Buffers[0]), m_Buffers);
	}

	if (m_VAO != 0)
	{
		glDeleteVertexArrays(1, &m_VAO);
		m_VAO = 0;
	}

	m_pScene->~aiScene();
}

bool SkinnedMesh::LoadMesh(const std::string& Filename, uint32_t instanceCount)
{
	mInstanceCount = instanceCount;
	directory = Filename.substr(0, Filename.find_last_of('/'));

	// Create the VAO
	glGenVertexArrays(1, &m_VAO);
	glBindVertexArray(m_VAO);

	// Create the buffers for the vertices attributes
	glGenBuffers(sizeof(m_Buffers) / sizeof(m_Buffers[0]), m_Buffers);

	bool Ret = false;

	m_pScene = m_Importer.ReadFile(Filename.c_str(), aiProcess_Triangulate | aiProcess_GenSmoothNormals | aiProcess_JoinIdenticalVertices);
	m_pScene = m_Importer.GetOrphanedScene();

	if (m_pScene)
	{
		m_GlobalInverseTransform = m_pScene->mRootNode->mTransformation;
		m_GlobalInverseTransform.Inverse();
		Ret = InitFromScene(m_pScene, Filename);

		if (m_pScene->mNumAnimations > 0)
		{
			mIsAnim = true;
			mAnimations.push_back(m_pScene->mAnimations[0]);
		}
	}
	else
	{
		printf("Error parsing '%s': '%s'\n", Filename.c_str(), m_Importer.GetErrorString());
	}

	// Make sure the VAO is not changed from the outside
	glBindVertexArray(0);

	return Ret;
}

void SkinnedMesh::AddAnimation(const std::string& Filename)
{
	const aiScene* anim = m_Importer.ReadFile(Filename.c_str(), aiProcess_Triangulate | aiProcess_GenSmoothNormals | aiProcess_JoinIdenticalVertices);
	anim = m_Importer.GetOrphanedScene();
	if (anim->mNumAnimations > 0)
	{
		mIsAnim = true;
		mAnimations.push_back(anim->mAnimations[0]);
	}
}

tinystl::vector<Texture> SkinnedMesh::InitMaterials(const aiMaterial* material, aiTextureType type, std::string typeName)
{
	tinystl::vector<Texture> textures;
	for (unsigned int i = 0; i < material->GetTextureCount(type); i++)
	{
		aiString str;
		material->GetTexture(type, i, &str);
		// check if texture was loaded before and if so, continue to next iteration: skip loading a new texture
		bool skip = false;
		for (unsigned int j = 0; j < textures_loaded.size(); j++)
		{
			if (std::strcmp(textures_loaded[j].path.data(), str.C_Str()) == 0)
			{
				textures.push_back(textures_loaded[j]);
				skip = true; // a texture with the same filepath has already been loaded, continue to next one. (optimization)
				break;
			}
		}
		if (!skip)
		{   // if texture hasn't been loaded already, load it
			Texture texture;
			std::string fullPath = this->directory + "/" + str.C_Str();
			texture.id = LoadTexture(fullPath.c_str());
			texture.type = typeName;
			texture.path = str.C_Str();
			textures.push_back(texture);
			textures_loaded.push_back(texture);  // store it as texture loaded for entire model, to ensure we won't unnecesery load duplicate textures.
		}
	}
	return textures;
}

bool SkinnedMesh::InitFromScene(const aiScene* pScene, const std::string& Filename)
{
	m_Entries.resize(pScene->mNumMeshes);
	//m_Textures.resize(pScene->mNumMaterials);

	tinystl::vector<aiVector3D> Positions;
	tinystl::vector<aiVector3D> Normals;
	tinystl::vector<aiVector2D> TexCoords;
	tinystl::vector<VertexBoneData> Bones;
	tinystl::vector<uint32_t> Indices;

	uint32_t NumVertices = 0;
	uint32_t NumIndices = 0;

	// Count the number of vertices and indices
	for (uint32_t i = 0; i < m_Entries.size(); i++)
	{
		m_Entries[i].MaterialIndex = pScene->mMeshes[i]->mMaterialIndex;
		m_Entries[i].NumIndices = pScene->mMeshes[i]->mNumFaces * 3;
		m_Entries[i].BaseVertex = NumVertices;
		m_Entries[i].BaseIndex = NumIndices;

		NumVertices += pScene->mMeshes[i]->mNumVertices;
		NumIndices += m_Entries[i].NumIndices;
	}

	// Reserve space in the vectors for the vertex attributes and indices
	Positions.reserve(NumVertices);
	Normals.reserve(NumVertices);
	TexCoords.reserve(NumVertices);
	Bones.resize(NumVertices);
	Indices.reserve(NumIndices);

	// Initialize the meshes in the scene one by one
	for (uint32_t i = 0; i < m_Entries.size(); i++)
	{
		const aiMesh* paiMesh = pScene->mMeshes[i];
		InitMesh(i, paiMesh, Positions, Normals, TexCoords, Bones, Indices);
	}

	for (uint32_t i = 0; i < pScene->mNumMeshes; ++i)
	{
		aiMesh* mesh = pScene->mMeshes[i];
		uint32_t materialIndex = mesh->mMaterialIndex;
		aiMaterial* material = pScene->mMaterials[materialIndex];
		tinystl::vector<Texture> textures;

		tinystl::vector<Texture> diffuseMaps = InitMaterials(material, aiTextureType_DIFFUSE, "texture_diffuse");
		textures.insert(textures.end(), diffuseMaps.begin(), diffuseMaps.end());

		tinystl::vector<Texture> specularMaps = InitMaterials(material, aiTextureType_SPECULAR, "texture_specular");
		textures.insert(textures.end(), specularMaps.begin(), specularMaps.end());

		tinystl::vector<Texture> normalMaps = InitMaterials(material, aiTextureType_HEIGHT, "texture_normal");
		textures.insert(textures.end(), normalMaps.begin(), normalMaps.end());

		tinystl::vector<Texture> heightMaps = InitMaterials(material, aiTextureType_AMBIENT, "texture_height");
		textures.insert(textures.end(), heightMaps.begin(), heightMaps.end());

		mMeshTexturesMap[materialIndex] = textures;
	}

	// Generate and populate the buffers with vertex attributes and the indices
	glBindBuffer(GL_ARRAY_BUFFER, m_Buffers[POS_VB]);
	glBufferData(GL_ARRAY_BUFFER, sizeof(Positions[0]) * Positions.size(), &Positions[0], GL_STATIC_DRAW);
	glEnableVertexAttribArray(POSITION_LOCATION);
	glVertexAttribPointer(POSITION_LOCATION, 3, GL_FLOAT, GL_FALSE, 0, 0);

	glBindBuffer(GL_ARRAY_BUFFER, m_Buffers[TEXCOORD_VB]);
	glBufferData(GL_ARRAY_BUFFER, sizeof(TexCoords[0]) * TexCoords.size(), &TexCoords[0], GL_STATIC_DRAW);
	glEnableVertexAttribArray(TEX_COORD_LOCATION);
	glVertexAttribPointer(TEX_COORD_LOCATION, 2, GL_FLOAT, GL_FALSE, 0, 0);

	glBindBuffer(GL_ARRAY_BUFFER, m_Buffers[NORMAL_VB]);
	glBufferData(GL_ARRAY_BUFFER, sizeof(Normals[0]) * Normals.size(), &Normals[0], GL_STATIC_DRAW);
	glEnableVertexAttribArray(NORMAL_LOCATION);
	glVertexAttribPointer(NORMAL_LOCATION, 3, GL_FLOAT, GL_FALSE, 0, 0);

	glBindBuffer(GL_ARRAY_BUFFER, m_Buffers[BONE_VB]);
	glBufferData(GL_ARRAY_BUFFER, sizeof(Bones[0]) * Bones.size(), &Bones[0], GL_STATIC_DRAW);
	glEnableVertexAttribArray(BONE_ID_LOCATION);
	glVertexAttribIPointer(BONE_ID_LOCATION, 4, GL_INT, sizeof(VertexBoneData), (const GLvoid*)0);
	glEnableVertexAttribArray(BONE_WEIGHT_LOCATION);
	glVertexAttribPointer(BONE_WEIGHT_LOCATION, 4, GL_FLOAT, GL_FALSE, sizeof(VertexBoneData), (const GLvoid*)16);

	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_Buffers[INDEX_BUFFER]);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(Indices[0]) * Indices.size(), &Indices[0], GL_STATIC_DRAW);

	if (mInstanceCount != 0)
	{
		glGenBuffers(1, &instanceVBO);
		glBindBuffer(GL_ARRAY_BUFFER, instanceVBO); // this attribute comes from a different vertex buffer

		glEnableVertexAttribArray(5);
		glVertexAttribPointer(5, 4, GL_FLOAT, GL_FALSE, 16 * sizeof(float), (void*)0);
		glVertexAttribDivisor(5, 1); // tell OpenGL this is an instanced vertex attribute.

		glEnableVertexAttribArray(6);
		glVertexAttribPointer(6, 4, GL_FLOAT, GL_FALSE, 16 * sizeof(float), (void*)(4 * sizeof(float)));
		glVertexAttribDivisor(6, 1); // tell OpenGL this is an instanced vertex attribute.

		glEnableVertexAttribArray(7);
		glVertexAttribPointer(7, 4, GL_FLOAT, GL_FALSE, 16 * sizeof(float), (void*)(8 * sizeof(float)));
		glVertexAttribDivisor(7, 1); // tell OpenGL this is an instanced vertex attribute.

		glEnableVertexAttribArray(8);
		glVertexAttribPointer(8, 4, GL_FLOAT, GL_FALSE, 16 * sizeof(float), (void*)(12 * sizeof(float)));
		glVertexAttribDivisor(8, 1); // tell OpenGL this is an instanced vertex attribute.

		glBindBuffer(GL_ARRAY_BUFFER, 0);
	}
	glBindVertexArray(0);

	return glGetError();
}

void SkinnedMesh::InitMesh(uint32_t MeshIndex,
	const aiMesh* paiMesh,
	tinystl::vector<aiVector3D>& Positions,
	tinystl::vector<aiVector3D>& Normals,
	tinystl::vector<aiVector2D>& TexCoords,
	tinystl::vector<VertexBoneData>& Bones,
	tinystl::vector<uint32_t>& Indices)
{
	const aiVector3D Zero3D(0.0f, 0.0f, 0.0f);

	// Populate the vertex attribute vectors
	for (uint32_t i = 0; i < paiMesh->mNumVertices; i++)
	{
		const aiVector3D* pPos = &(paiMesh->mVertices[i]);
		const aiVector3D* pNormal = &(paiMesh->mNormals[i]);
		const aiVector3D* pTexCoord = paiMesh->HasTextureCoords(0) ? &(paiMesh->mTextureCoords[0][i]) : &Zero3D;

		Positions.push_back(aiVector3D(pPos->x, pPos->y, pPos->z));
		Normals.push_back(aiVector3D(pNormal->x, pNormal->y, pNormal->z));
		TexCoords.push_back(aiVector2D(pTexCoord->x, pTexCoord->y));
	}

	LoadBones(MeshIndex, paiMesh, Bones);

	// Populate the index buffer
	for (uint32_t i = 0; i < paiMesh->mNumFaces; i++)
	{
		const aiFace& Face = paiMesh->mFaces[i];
		assert(Face.mNumIndices == 3);
		Indices.push_back(Face.mIndices[0]);
		Indices.push_back(Face.mIndices[1]);
		Indices.push_back(Face.mIndices[2]);
	}
}


void SkinnedMesh::LoadBones(uint32_t MeshIndex, const aiMesh* pMesh, tinystl::vector<VertexBoneData>& Bones)
{
	for (uint32_t i = 0; i < pMesh->mNumBones; i++)
	{
		uint32_t BoneIndex = 0;
		std::string BoneName(pMesh->mBones[i]->mName.data);
		
		if (m_BoneMapping.find(BoneName) == m_BoneMapping.end())
		{
			// Allocate an index for a new bone
			BoneIndex = m_NumBones;
			m_NumBones++;
			BoneInfo bi;
			m_BoneInfo.push_back(bi);
			m_BoneInfo[BoneIndex].BoneOffset = pMesh->mBones[i]->mOffsetMatrix;
			m_BoneMapping[BoneName] = BoneIndex;
		}
		else
		{
			BoneIndex = m_BoneMapping[BoneName];
		}

		for (uint32_t j = 0; j < pMesh->mBones[i]->mNumWeights; j++)
		{
			uint32_t VertexID = m_Entries[MeshIndex].BaseVertex + pMesh->mBones[i]->mWeights[j].mVertexId;
			float Weight = pMesh->mBones[i]->mWeights[j].mWeight;
			Bones[VertexID].AddBoneData(BoneIndex, Weight);
		}
	}
}

void SkinnedMesh::Render(ShaderProgram shader)
{
	glBindVertexArray(m_VAO);

	for (uint32_t i = 0; i < m_Entries.size(); i++)
	{
		const uint32_t MaterialIndex = m_Entries[i].MaterialIndex;
		tinystl::unordered_map<uint32_t, tinystl::vector<Texture>>::iterator itr = mMeshTexturesMap.find(MaterialIndex);

		if (itr != mMeshTexturesMap.end())
		{
			tinystl::vector<Texture>& textures = itr->second;

			// bind appropriate textures
			unsigned int diffuseNr = 1;
			unsigned int specularNr = 1;
			unsigned int normalNr = 1;
			unsigned int heightNr = 1;
			for (unsigned int i = 0; i < textures.size(); i++)
			{
				glActiveTexture(GL_TEXTURE0 + i); // active proper texture unit before binding
				// retrieve texture number (the N in diffuse_textureN)
				std::string number;
				std::string name = textures[i].type;
				if (name == "texture_diffuse")
					number = std::to_string(diffuseNr++).c_str();
				else if (name == "texture_specular")
					number = std::to_string(specularNr++).c_str(); // transfer unsigned int to stream
				else if (name == "texture_normal")
					number = std::to_string(normalNr++).c_str(); // transfer unsigned int to stream
				else if (name == "texture_height")
					number = std::to_string(heightNr++).c_str(); // transfer unsigned int to stream

														 // now set the sampler to the correct texture unit
				//glUniform1i(glGetUniformLocation(shader.mID, (name + number).c_str()), i);
				
				shader.SetUniform((name + number).c_str(), &i);
				// and finally bind the texture
				glBindTexture(GL_TEXTURE_2D, textures[i].id);
			}
		}

		if (mInstanceCount != 0)
		{
			glDrawElementsInstancedBaseVertex(GL_TRIANGLES,
				m_Entries[i].NumIndices,
				GL_UNSIGNED_INT,
				(void*)(sizeof(uint32_t) * m_Entries[i].BaseIndex),
				mInstanceCount,
				m_Entries[i].BaseVertex);
		}
		else
		{
			glDrawElementsBaseVertex(GL_TRIANGLES,
				m_Entries[i].NumIndices,
				GL_UNSIGNED_INT,
				(void*)(sizeof(uint32_t) * m_Entries[i].BaseIndex),
				m_Entries[i].BaseVertex);
		}
	}

	// Make sure the VAO is not changed from the outside    
	glBindVertexArray(0);
}


uint32_t SkinnedMesh::FindPosition(float AnimationTime, const aiNodeAnim* pNodeAnim)
{
	for (uint32_t i = 0; i < pNodeAnim->mNumPositionKeys - 1; i++)
	{
		if (AnimationTime < (float)pNodeAnim->mPositionKeys[i + 1].mTime)
		{
			return i;
		}
	}

	assert(0);

	return 0;
}


uint32_t SkinnedMesh::FindRotation(float AnimationTime, const aiNodeAnim* pNodeAnim)
{
	assert(pNodeAnim->mNumRotationKeys > 0);

	for (uint32_t i = 0; i < pNodeAnim->mNumRotationKeys - 1; i++)
	{
		if (AnimationTime < (float)pNodeAnim->mRotationKeys[i + 1].mTime)
		{
			return i;
		}
	}

	assert(0);

	return 0;
}


uint32_t SkinnedMesh::FindScaling(float AnimationTime, const aiNodeAnim* pNodeAnim)
{
	assert(pNodeAnim->mNumScalingKeys > 0);

	for (uint32_t i = 0; i < pNodeAnim->mNumScalingKeys - 1; i++)
	{
		if (AnimationTime < (float)pNodeAnim->mScalingKeys[i + 1].mTime) {
			return i;
		}
	}

	assert(0);

	return 0;
}


void SkinnedMesh::CalcInterpolatedPosition(aiVector3D& Out, float AnimationTime, const aiNodeAnim* pNodeAnim)
{
	if (pNodeAnim->mNumPositionKeys == 1) {
		Out = pNodeAnim->mPositionKeys[0].mValue;
		return;
	}

	uint32_t PositionIndex = FindPosition(AnimationTime, pNodeAnim);
	uint32_t NextPositionIndex = (PositionIndex + 1);
	assert(NextPositionIndex < pNodeAnim->mNumPositionKeys);
	float DeltaTime = (float)(pNodeAnim->mPositionKeys[NextPositionIndex].mTime - pNodeAnim->mPositionKeys[PositionIndex].mTime);
	float Factor = (AnimationTime - (float)pNodeAnim->mPositionKeys[PositionIndex].mTime) / DeltaTime;
	assert(Factor >= 0.0f && Factor <= 1.0f);
	const aiVector3D& Start = pNodeAnim->mPositionKeys[PositionIndex].mValue;
	const aiVector3D& End = pNodeAnim->mPositionKeys[NextPositionIndex].mValue;
	aiVector3D Delta = End - Start;
	Out = Start + Factor * Delta;
}


void SkinnedMesh::CalcInterpolatedRotation(Quaternion& Out, float AnimationTime, const aiNodeAnim* pNodeAnim)
{
	// we need at least two values to interpolate...
	if (pNodeAnim->mNumRotationKeys == 1) {
		Out = pNodeAnim->mRotationKeys[0].mValue;
		return;
	}

	uint32_t RotationIndex = FindRotation(AnimationTime, pNodeAnim);
	uint32_t NextRotationIndex = (RotationIndex + 1);
	assert(NextRotationIndex < pNodeAnim->mNumRotationKeys);
	float DeltaTime = (float)(pNodeAnim->mRotationKeys[NextRotationIndex].mTime - pNodeAnim->mRotationKeys[RotationIndex].mTime);
	float Factor = (AnimationTime - (float)pNodeAnim->mRotationKeys[RotationIndex].mTime) / DeltaTime;
	assert(Factor >= 0.0f && Factor <= 1.0f);
	const Quaternion& StartRotationQ = pNodeAnim->mRotationKeys[RotationIndex].mValue;
	const Quaternion& EndRotationQ = pNodeAnim->mRotationKeys[NextRotationIndex].mValue;
	Out = Quaternion::interpolate(StartRotationQ, EndRotationQ, Factor);
	Out.Normalize();
}


void SkinnedMesh::CalcInterpolatedScaling(aiVector3D& Out, float AnimationTime, const aiNodeAnim* pNodeAnim)
{
	if (pNodeAnim->mNumScalingKeys == 1) {
		Out = pNodeAnim->mScalingKeys[0].mValue;
		return;
	}

	uint32_t ScalingIndex = FindScaling(AnimationTime, pNodeAnim);
	uint32_t NextScalingIndex = (ScalingIndex + 1);
	assert(NextScalingIndex < pNodeAnim->mNumScalingKeys);
	float DeltaTime = (float)(pNodeAnim->mScalingKeys[NextScalingIndex].mTime - pNodeAnim->mScalingKeys[ScalingIndex].mTime);
	float Factor = (AnimationTime - (float)pNodeAnim->mScalingKeys[ScalingIndex].mTime) / DeltaTime;
	assert(Factor >= 0.0f && Factor <= 1.0f);
	const aiVector3D& Start = pNodeAnim->mScalingKeys[ScalingIndex].mValue;
	const aiVector3D& End = pNodeAnim->mScalingKeys[NextScalingIndex].mValue;
	aiVector3D Delta = End - Start;
	Out = Start + Factor * Delta;
}


void SkinnedMesh::ReadNodeHeirarchy(float AnimationTime, const aiNode* pNode, const aiMatrix4x4& ParentTransform)
{
	std::string NodeName(pNode->mName.data);

	const aiAnimation* pAnimation = mAnimations[mCurrentAnimationIndex];
	
	aiMatrix4x4 NodeTransformation(pNode->mTransformation);

	const aiNodeAnim* pNodeAnim = FindNodeAnim(pAnimation, NodeName);

	if (pNodeAnim)
	{
		// Interpolate scaling and generate scaling transformation matrix
		aiVector3D Scaling;
		CalcInterpolatedScaling(Scaling, AnimationTime, pNodeAnim);
		aiMatrix4x4 ScalingM;
		aiMatrix4x4::Scaling(Scaling, ScalingM);

		// Interpolate rotation and generate rotation transformation matrix
		Quaternion RotationQ;
		CalcInterpolatedRotation(RotationQ, AnimationTime, pNodeAnim);
		aiMatrix4x4 RotationM = RotationQ.toAiRotationMatrix();

		// Interpolate translation and generate translation transformation matrix
		aiVector3D Translation;
		CalcInterpolatedPosition(Translation, AnimationTime, pNodeAnim);
		aiMatrix4x4 TranslationM;
		aiMatrix4x4::Translation(Translation, TranslationM);
		
		// Combine the above transformations
		NodeTransformation = TranslationM * RotationM * ScalingM;
	}

	aiMatrix4x4 GlobalTransformation = ParentTransform * NodeTransformation;

	if (m_BoneMapping.find(NodeName) != m_BoneMapping.end())
	{
		uint32_t BoneIndex = m_BoneMapping[NodeName];
		m_BoneInfo[BoneIndex].FinalTransformation = m_GlobalInverseTransform * GlobalTransformation * m_BoneInfo[BoneIndex].BoneOffset;	
		m_BoneInfo[BoneIndex].m_BoneInverseTransform = m_GlobalInverseTransform * GlobalTransformation;
		mLineSegments.emplace_back(LineSegment( m_GlobalInverseTransform*ParentTransform, m_BoneInfo[BoneIndex].m_BoneInverseTransform ));
	}
	
	for (uint32_t i = 0; i < pNode->mNumChildren; i++)
	{
		ReadNodeHeirarchy(AnimationTime, pNode->mChildren[i], GlobalTransformation);
	}
}


void SkinnedMesh::BoneTransform(float TimeInSeconds, tinystl::vector<aiMatrix4x4>& Transforms, tinystl::vector<aiMatrix4x4>& BoneTransforms)
{
	aiMatrix4x4 Identity;

	float TicksPerSecond = (float)(mAnimations[mCurrentAnimationIndex]->mTicksPerSecond != 0 ? mAnimations[mCurrentAnimationIndex]->mTicksPerSecond : 25.0f);
	float TimeInTicks = TimeInSeconds * TicksPerSecond;
	float AnimationTime = fmod(TimeInTicks, (float)mAnimations[mCurrentAnimationIndex]->mDuration);

	mLineSegments.clear();
	ReadNodeHeirarchy(AnimationTime, m_pScene->mRootNode, Identity);

	Transforms.resize(m_NumBones);
	BoneTransforms.resize(m_NumBones);

	for (uint32_t i = 0; i < m_NumBones; i++)
	{
		Transforms[i] = m_BoneInfo[i].FinalTransformation;
		BoneTransforms[i] = m_BoneInfo[i].m_BoneInverseTransform;
	}
}

const aiNodeAnim* SkinnedMesh::FindNodeAnim(const aiAnimation* pAnimation, const std::string NodeName)
{
	for (uint32_t i = 0; i < pAnimation->mNumChannels; i++)
	{
		const aiNodeAnim* pNodeAnim = pAnimation->mChannels[i];

		if (std::string(pNodeAnim->mNodeName.data) == NodeName)
		{
			return pNodeAnim;
		}
	}

	return NULL;
}

void SkinnedMesh::SetCurrentAnimation(int& index)
{
	if (index >= (int)mAnimations.size())
	{
		mCurrentAnimationIndex = 0;
		index = mCurrentAnimationIndex;
	}
	else if (index < 0)
	{
		mCurrentAnimationIndex = (int)mAnimations.size()-1;
		index = mCurrentAnimationIndex;
	}
	else
		mCurrentAnimationIndex = index;
}

PBRMat_Tex::~PBRMat_Tex()
{
	if (albedo != (unsigned int)INVALID_TEXTURE_ID)
	{
		glDeleteTextures(1, &albedo);
	}
	if (metallic != (unsigned int)INVALID_TEXTURE_ID)
	{
		glDeleteTextures(1, &metallic);
	}
	if (roughness != (unsigned int)INVALID_TEXTURE_ID)
	{
		glDeleteTextures(1, &roughness);
	}
	if (ao != (unsigned int)INVALID_TEXTURE_ID)
	{
		glDeleteTextures(1, &ao);
	}
}

void PBRMat_Tex::LoadPBRTexture(const char* filepath, PBRTextureType type)
{
	unsigned int texture_id = (unsigned int)LoadTexture(filepath);
	switch (type)
	{
	case ALBEDO:
		albedo = texture_id;
		break;
	case NORMAL:
		normal = texture_id;
		break;
	case METALLIC:
		metallic = texture_id;
		break;
	case ROUGHNESS:
		roughness = texture_id;
		break;
	case AO:
		ao = texture_id;
		break;
	}
}

void PBRMat_Tex::BindTextures()
{
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, albedo);
	glActiveTexture(GL_TEXTURE1);
	glBindTexture(GL_TEXTURE_2D, normal);
	glActiveTexture(GL_TEXTURE2);
	glBindTexture(GL_TEXTURE_2D, metallic);
	glActiveTexture(GL_TEXTURE3);
	glBindTexture(GL_TEXTURE_2D, roughness);
	glActiveTexture(GL_TEXTURE4);
	glBindTexture(GL_TEXTURE_2D, ao);
}

void PBRMat::UpdateMaterial(ShaderProgram* pShaderProgram)
{
	pShaderProgram->SetUniform("u_vAlbedo", &albedo);
	pShaderProgram->SetUniform("u_fMetallic", &metallic);
	pShaderProgram->SetUniform("u_fRoughness", &roughness);
	pShaderProgram->SetUniform("u_fAo", &ao);
}