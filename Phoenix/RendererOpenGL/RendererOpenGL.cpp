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
{}

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

uint32_t LoadTexture(const char* path)
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

	glTexImage2D(GL_TEXTURE_2D, 0, internal_format, width, height, 0, internal_format, GL_UNSIGNED_BYTE, data);
	
	glGenerateMipmap(GL_TEXTURE_2D);
	
	stbi_image_free(data);

	return texture;
}


///////////////////////////////////////////
//// MESH

Mesh::Mesh(tinystl::vector<Vertex> vertices, tinystl::vector<unsigned int> indices, tinystl::vector<Texture> textures, uint32_t instanceCount, unsigned int instanceVBO)
{
	this->vertices = vertices;
	this->indices = indices;
	this->textures = textures;

	// now that we have all the required data, set the vertex buffers and its attribute pointers.
	SetupMesh(instanceCount, instanceVBO);
}

void Mesh::SetupMesh(uint32_t instanceCount, unsigned int instanceVBO)
{
	// create buffers/arrays
	glGenVertexArrays(1, &VAO);
	glGenBuffers(1, &VBO);
	glGenBuffers(1, &EBO);

	glBindVertexArray(VAO);
	// load data into vertex buffers
	glBindBuffer(GL_ARRAY_BUFFER, VBO);
	// A great thing about structs is that their memory layout is sequential for all its items.
	// The effect is that we can simply pass a pointer to the struct and it translates perfectly to a glm::vec3/2 array which
	// again translates to 3/2 floats which translates to a byte array.
	glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(Vertex), &vertices[0], GL_STATIC_DRAW);

	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(unsigned int), &indices[0], GL_STATIC_DRAW);

	// set the vertex attribute pointers
	// vertex Positions
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)0);
	// vertex normals
	glEnableVertexAttribArray(1);
	glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, Normal));
	// vertex texture coords
	glEnableVertexAttribArray(2);
	glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, TexCoords));
	// vertex tangent
	glEnableVertexAttribArray(3);
	glVertexAttribPointer(3, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, Tangent));
	// vertex bitangent
	glEnableVertexAttribArray(4);
	glVertexAttribPointer(4, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, Bitangent));

	if (instanceCount != 0)
	{
		meshInstanceCount = instanceCount;

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
}

void Mesh::Draw(ShaderProgram shader)
{
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
		std::string fullName = name;
		fullName.append(number.c_str(), number.c_str() + number.size());
		shader.SetUniform((name + number).c_str(), &i);
		// and finally bind the texture
		glBindTexture(GL_TEXTURE_2D, textures[i].id);
	}

	// draw mesh
	glBindVertexArray(VAO);
	if(meshInstanceCount != 0)
		glDrawElementsInstanced(GL_TRIANGLES, (GLsizei)indices.size(), GL_UNSIGNED_INT, 0, meshInstanceCount);
	else
		glDrawElements(GL_TRIANGLES, (GLsizei)indices.size(), GL_UNSIGNED_INT, 0);
	glBindVertexArray(0);

	// always good practice to set everything back to defaults once configured.
	glActiveTexture(GL_TEXTURE0);
}


///////////////////////////////////////////
//// MODEL

Model::Model(std::string const &path, bool gamma, uint32_t _instanceCount) : gammaCorrection(gamma)
{ 
	instanceCount = _instanceCount;
	if (instanceCount != 0)
	{
		glGenBuffers(1, &instanceVBO);
	}
	loadModel(path);
}

void Model::Draw(ShaderProgram shader)
{
	for (unsigned int i = 0; i < meshes.size(); i++)
		meshes[i].Draw(shader);
}

tinystl::unordered_map<size_t, Model*> mModelsMap;
Model* LoadModel(const char* filepath, bool gamma, uint32_t instanceCount)
{
	size_t hashedFilePath = hasher(std::string(filepath));
	tinystl::unordered_map<size_t, Model*>::iterator itr = mModelsMap.find(hashedFilePath);
	if (itr == mModelsMap.end())
	{
		Model* pModel = new Model(filepath, gamma, instanceCount);
		mModelsMap[hashedFilePath] = pModel;
		return pModel;
	}

	return itr->second;
}

void Model::loadModel(std::string const &path)
{
	// read file via ASSIMP
	Assimp::Importer importer;
	const aiScene* scene = importer.ReadFile(path, aiProcess_Triangulate | aiProcess_FlipUVs | aiProcess_CalcTangentSpace);
	// check for errors
	if (!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode) // if is Not Zero
	{
		std::cout << "ERROR::ASSIMP:: " << importer.GetErrorString() << std::endl;
		return;
	}
	// retrieve the directory path of the filepath
	directory = path.substr(0, path.find_last_of('/'));

	// process ASSIMP's root node recursively
	processNode(scene->mRootNode, scene);
}

void Model::processNode(aiNode *node, const aiScene *scene)
{
	// process each mesh located at the current node
	for (unsigned int i = 0; i < node->mNumMeshes; i++)
	{
		// the node object only contains indices to index the actual objects in the scene. 
		// the scene contains all the data, node is just to keep stuff organized (like relations between nodes).
		aiMesh* mesh = scene->mMeshes[node->mMeshes[i]];
		meshes.push_back(processMesh(mesh, scene));
	}
	// after we've processed all of the meshes (if any) we then recursively process each of the children nodes
	for (unsigned int i = 0; i < node->mNumChildren; i++)
	{
		processNode(node->mChildren[i], scene);
	}

}

Mesh Model::processMesh(aiMesh *mesh, const aiScene *scene)
{
	// data to fill
	tinystl::vector<Vertex> vertices;
	tinystl::vector<unsigned int> indices;
	tinystl::vector<Texture> textures;

	// Walk through each of the mesh's vertices
	for (unsigned int i = 0; i < mesh->mNumVertices; i++)
	{
		Vertex vertex;
		glm::vec3 vector; // we declare a placeholder vector since assimp uses its own vector class that doesn't directly convert to glm's vec3 class so we transfer the data to this placeholder glm::vec3 first.
		// positions
		vector.x = mesh->mVertices[i].x;
		vector.y = mesh->mVertices[i].y;
		vector.z = mesh->mVertices[i].z;
		vertex.Position = vector;
		// normals
		vector.x = mesh->mNormals[i].x;
		vector.y = mesh->mNormals[i].y;
		vector.z = mesh->mNormals[i].z;
		vertex.Normal = vector;
		// texture coordinates
		if (mesh->mTextureCoords[0]) // does the mesh contain texture coordinates?
		{
			glm::vec2 vec;
			// a vertex can contain up to 8 different texture coordinates. We thus make the assumption that we won't 
			// use models where a vertex can have multiple texture coordinates so we always take the first set (0).
			vec.x = mesh->mTextureCoords[0][i].x;
			vec.y = mesh->mTextureCoords[0][i].y;
			vertex.TexCoords = vec;
		}
		else
			vertex.TexCoords = glm::vec2(0.0f, 0.0f);
		// tangent
		vector.x = mesh->mTangents[i].x;
		vector.y = mesh->mTangents[i].y;
		vector.z = mesh->mTangents[i].z;
		vertex.Tangent = vector;
		// bitangent
		vector.x = mesh->mBitangents[i].x;
		vector.y = mesh->mBitangents[i].y;
		vector.z = mesh->mBitangents[i].z;
		vertex.Bitangent = vector;
		vertices.push_back(vertex);
	}
	// now wak through each of the mesh's faces (a face is a mesh its triangle) and retrieve the corresponding vertex indices.
	for (unsigned int i = 0; i < mesh->mNumFaces; i++)
	{
		aiFace face = mesh->mFaces[i];
		// retrieve all indices of the face and store them in the indices vector
		for (unsigned int j = 0; j < face.mNumIndices; j++)
			indices.push_back(face.mIndices[j]);
	}
	// process materials
	aiMaterial* material = scene->mMaterials[mesh->mMaterialIndex];
	// we assume a convention for sampler names in the shaders. Each diffuse texture should be named
	// as 'texture_diffuseN' where N is a sequential number ranging from 1 to MAX_SAMPLER_NUMBER. 
	// Same applies to other texture as the following list summarizes:
	// diffuse: texture_diffuseN
	// specular: texture_specularN
	// normal: texture_normalN

	// 1. diffuse maps
	tinystl::vector<Texture> diffuseMaps = loadMaterialTextures(material, aiTextureType_DIFFUSE, "texture_diffuse");
	textures.insert(textures.end(), diffuseMaps.begin(), diffuseMaps.end());
	// 2. specular maps
	tinystl::vector<Texture> specularMaps = loadMaterialTextures(material, aiTextureType_SPECULAR, "texture_specular");
	textures.insert(textures.end(), specularMaps.begin(), specularMaps.end());
	// 3. normal maps
	tinystl::vector<Texture> normalMaps = loadMaterialTextures(material, aiTextureType_HEIGHT, "texture_normal");
	textures.insert(textures.end(), normalMaps.begin(), normalMaps.end());
	// 4. height maps
	tinystl::vector<Texture> heightMaps = loadMaterialTextures(material, aiTextureType_AMBIENT, "texture_height");
	textures.insert(textures.end(), heightMaps.begin(), heightMaps.end());

	// return a mesh object created from the extracted mesh data
	return Mesh(vertices, indices, textures, instanceCount, instanceVBO);
}

tinystl::vector<Texture> Model::loadMaterialTextures(aiMaterial *mat, aiTextureType type, std::string typeName)
{
	tinystl::vector<Texture> textures;
	for (unsigned int i = 0; i < mat->GetTextureCount(type); i++)
	{
		aiString str;
		mat->GetTexture(type, i, &str);
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


//////////////////////////////////////////////// QUAD
unsigned int mQuadVAO = 0;
unsigned int mQuadVBO = 0;
float quadVertices[] = {
	// positions        // texture Coords
	-1.0f,  1.0f, 0.0f, 0.0f, 1.0f,
	-1.0f, -1.0f, 0.0f, 0.0f, 0.0f,
	 1.0f,  1.0f, 0.0f, 1.0f, 1.0f,
	 1.0f, -1.0f, 0.0f, 1.0f, 0.0f,
};

void setupQuad()
{
	// setup plane VAO
	glGenVertexArrays(1, &mQuadVAO);
	glGenBuffers(1, &mQuadVBO);
	glBindVertexArray(mQuadVAO);
	glBindBuffer(GL_ARRAY_BUFFER, mQuadVBO);
	glBufferData(GL_ARRAY_BUFFER, sizeof(quadVertices), &quadVertices, GL_STATIC_DRAW);
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)0);
	glEnableVertexAttribArray(1);
	glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)(3 * sizeof(float)));
}


//////////////////////////////////////////////// CUBE
unsigned int mCubeVAO = 0;
unsigned int mCubeVBO = 0;
float vertices[] = {
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

void setupCube()
{
	glGenVertexArrays(1, &mCubeVAO);
	glGenBuffers(1, &mCubeVBO);
	// fill buffer
	glBindBuffer(GL_ARRAY_BUFFER, mCubeVBO);
	glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
	// link vertex attributes
	glBindVertexArray(mCubeVAO);
	{
		glEnableVertexAttribArray(0);
		glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)0);
		glEnableVertexAttribArray(1);
		glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(3 * sizeof(float)));
		glEnableVertexAttribArray(2);
		glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(6 * sizeof(float)));
	}
	glBindVertexArray(0);
}

OpenGLRenderer::OpenGLRenderer()
{
	setupQuad();
	setupCube();
}

OpenGLRenderer::~OpenGLRenderer()
{
	if (mModelsMap.size())
	{
		for (tinystl::unordered_hash_node<size_t, Model*> itr : mModelsMap)
		{
			delete itr.second;
		}
	}

	glDeleteVertexArrays(1, &mQuadVAO);
	glDeleteVertexArrays(1, &mCubeVAO);
}

void BindQuadVAO()
{
	glBindVertexArray(mQuadVAO);
}

void OpenGLRenderer::RenderQuad()
{
	glBindVertexArray(mQuadVAO);
	glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
	glBindVertexArray(0);
}

void OpenGLRenderer::RenderQuadInstanced(int numOfInstances)
{
	glBindVertexArray(mQuadVAO);
	glDrawArraysInstanced(GL_TRIANGLE_STRIP, 0, 4, numOfInstances);
	glBindVertexArray(0);
}

void BindCubeVAO()
{
	glBindVertexArray(mCubeVAO);
}

void OpenGLRenderer::RenderCube()
{
	glBindVertexArray(mCubeVAO);
	glDrawArrays(GL_TRIANGLES, 0, 36);
	glBindVertexArray(0);
}

void OpenGLRenderer::RenderCubeInstanced(int numOfInstances)
{
	glBindVertexArray(mCubeVAO);
	glDrawArraysInstanced(GL_TRIANGLES, 0, 36, numOfInstances);
	glBindVertexArray(0);
}


#define POSITION_LOCATION    0
#define TEX_COORD_LOCATION   1
#define NORMAL_LOCATION      2
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
	for (uint32_t i = 0; i < m_Textures.size(); i++)
	{
		//SAFE_DELETE(m_Textures[i]);
	}

	if (m_Buffers[0] != 0)
	{
		glDeleteBuffers(sizeof(m_Buffers) / sizeof(m_Buffers[0]), m_Buffers);
	}

	if (m_VAO != 0)
	{
		glDeleteVertexArrays(1, &m_VAO);
		m_VAO = 0;
	}
}

bool SkinnedMesh::LoadMesh(const std::string& Filename)
{
	directory = Filename.substr(0, Filename.find_last_of('/'));

	// Create the VAO
	glGenVertexArrays(1, &m_VAO);
	glBindVertexArray(m_VAO);

	// Create the buffers for the vertices attributes
	glGenBuffers(sizeof(m_Buffers) / sizeof(m_Buffers[0]), m_Buffers);

	bool Ret = false;

	m_pScene = m_Importer.ReadFile(Filename.c_str(), aiProcess_Triangulate | aiProcess_GenSmoothNormals | aiProcess_JoinIdenticalVertices);

	if (m_pScene)
	{
		m_GlobalInverseTransform = m_pScene->mRootNode->mTransformation;
		m_GlobalInverseTransform.Inverse();
		Ret = InitFromScene(m_pScene, Filename);
	}
	else
	{
		printf("Error parsing '%s': '%s'\n", Filename.c_str(), m_Importer.GetErrorString());
	}

	// Make sure the VAO is not changed from the outside
	glBindVertexArray(0);

	return Ret;
}

tinystl::vector<Texture> SkinnedMesh::InitMaterials(const aiMaterial* material, aiTextureType type, std::string typeName)
{
	tinystl::vector<Texture> textures;
	for (unsigned int i = 0; i < material->GetTextureCount(type); i++)
	{
		aiString str;
		material->GetTexture(type, i, &str);
		// check if texture was loaded before and if so, continue to next iteration: skip loading a new texture
		//bool skip = false;
		//for (unsigned int j = 0; j < textures_loaded.size(); j++)
		//{
		//	if (std::strcmp(textures_loaded[j].path.data(), str.C_Str()) == 0)
		//	{
		//		textures.push_back(textures_loaded[j]);
		//		skip = true; // a texture with the same filepath has already been loaded, continue to next one. (optimization)
		//		break;
		//	}
		//}
		//if (!skip)
		{   // if texture hasn't been loaded already, load it
			Texture texture;
			std::string fullPath = this->directory + "/" + str.C_Str();
			texture.id = LoadTexture(fullPath.c_str());
			texture.type = typeName;
			texture.path = str.C_Str();
			textures.push_back(texture);
			//textures_loaded.push_back(texture);  // store it as texture loaded for entire model, to ensure we won't unnecesery load duplicate textures.
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

	// TO DO
	/*if (!InitMaterials(pScene, Filename))
	{
		return false;
	}*/
	for (uint32_t i = 0; i < pScene->mNumMaterials; ++i)
	{
		const aiMaterial* material = pScene->mMaterials[i];

		// 1. diffuse maps
		tinystl::vector<Texture> diffuseMaps = InitMaterials(material, aiTextureType_DIFFUSE, "texture_diffuse");
		m_Textures.insert(m_Textures.end(), diffuseMaps.begin(), diffuseMaps.end());
		//m_Textures[i] = diffuseMaps[0];
	/*
		// 2. specular maps
		tinystl::vector<Texture> specularMaps = InitMaterials(material, aiTextureType_SPECULAR, "texture_specular");
		m_Textures.insert(m_Textures.end(), specularMaps.begin(), specularMaps.end());
		
		// 3. normal maps
		tinystl::vector<Texture> normalMaps = InitMaterials(material, aiTextureType_HEIGHT, "texture_normal");
		m_Textures.insert(m_Textures.end(), normalMaps.begin(), normalMaps.end());
		
		// 4. height maps
		tinystl::vector<Texture> heightMaps = InitMaterials(material, aiTextureType_AMBIENT, "texture_height");
		m_Textures.insert(m_Textures.end(), heightMaps.begin(), heightMaps.end());
	*/
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

void SkinnedMesh::Render()
{
	glBindVertexArray(m_VAO);

	for (uint32_t i = 0; i < m_Entries.size(); i++)
	{
		const uint32_t MaterialIndex = m_Entries[i].MaterialIndex;

		if(m_Textures.size() > 0)
		{
			assert(MaterialIndex < m_Textures.size());

			//if (m_Textures[MaterialIndex]) {
				//m_Textures[MaterialIndex]->Bind(GL_TEXTURE0); TO DO
				glActiveTexture(GL_TEXTURE0);
				glBindTexture(GL_TEXTURE_2D, m_Textures[MaterialIndex].id);
			//}
		}

		glDrawElementsBaseVertex(GL_TRIANGLES,
			m_Entries[i].NumIndices,
			GL_UNSIGNED_INT,
			(void*)(sizeof(uint32_t) * m_Entries[i].BaseIndex),
			m_Entries[i].BaseVertex);
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

	const aiAnimation* pAnimation = m_pScene->mAnimations[0];

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
		aiQuaternion aiRotationQ = RotationQ.toAiQuaternion();
		aiMatrix4x4 RotationM = aiMatrix4x4(aiRotationQ.GetMatrix());

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
	}

	for (uint32_t i = 0; i < pNode->mNumChildren; i++)
	{
		ReadNodeHeirarchy(AnimationTime, pNode->mChildren[i], GlobalTransformation);
	}
}


void SkinnedMesh::BoneTransform(float TimeInSeconds, tinystl::vector<aiMatrix4x4>& Transforms)
{
	aiMatrix4x4 Identity;

	float TicksPerSecond = (float)(m_pScene->mAnimations[0]->mTicksPerSecond != 0 ? m_pScene->mAnimations[0]->mTicksPerSecond : 25.0f);
	float TimeInTicks = TimeInSeconds * TicksPerSecond;
	float AnimationTime = fmod(TimeInTicks, (float)m_pScene->mAnimations[0]->mDuration);

	ReadNodeHeirarchy(AnimationTime, m_pScene->mRootNode, Identity);

	Transforms.resize(m_NumBones);

	for (uint32_t i = 0; i < m_NumBones; i++)
	{
		Transforms[i] = m_BoneInfo[i].FinalTransformation;
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