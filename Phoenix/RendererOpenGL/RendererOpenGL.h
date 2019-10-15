#pragma once

#include <unordered_map>
#include <string>

#include <glad/glad.h>
#include "../../Common/Thirdparty/TINYSTL/vector.h"
#include "../../Common/Thirdparty/TINYSTL/unordered_map.h"
#include "../../Common/Renderer/keyBindings.h"

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>

#include "Quaternion.h"

struct Uniform
{
	uint16_t location;
	GLenum type;
};

class ShaderProgram
{
public:
	ShaderProgram(std::string vertexShaderPath, std::string fragmentShaderPath);
	~ShaderProgram();

	void SetUniform(const char*, void*);

	int mId;
	std::unordered_map<size_t, Uniform> mUniformVarMap;
};

/////////////////////
// MESH

struct Vertex
{
	// position
	glm::vec3 Position;
	// normal
	glm::vec3 Normal;
	// texCoords
	glm::vec2 TexCoords;
	// tangent
	glm::vec3 Tangent;
	// bitangent
	glm::vec3 Bitangent;
};

struct Texture
{
	unsigned int id;
	std::string type;
	std::string path;
};

class Mesh {
public:
	/*  Mesh Data  */
	tinystl::vector<Vertex> vertices;
	tinystl::vector<unsigned int> indices;
	tinystl::vector<Texture> textures;
	unsigned int VAO;

	/*  Functions  */
	// constructor
	Mesh(tinystl::vector<Vertex> vertices, tinystl::vector<unsigned int> indices, tinystl::vector<Texture> textures, uint32_t instanceCount = 0, unsigned int instanceVBO = 0);

	// render the mesh
	void Draw(ShaderProgram shader);

private:
	/*  Render data  */
	unsigned int VBO, EBO;
	uint32_t meshInstanceCount = 0;

	/*  Functions    */
	// initializes all the buffer objects/arrays
	void SetupMesh(uint32_t instanceCount, unsigned int instanceVBO);
};


/////////////////////
// MODEL

class Model
{
public:
	/*  Model Data */
	tinystl::vector<Texture> textures_loaded;	// stores all the textures loaded so far, optimization to make sure textures aren't loaded more than once.
	tinystl::vector<Mesh> meshes;
	std::string directory;
	bool gammaCorrection;

	/*  Functions   */
	// constructor, expects a filepath to a 3D model.
	Model(std::string const &path, bool gamma, uint32_t instanceCount = 0);

	// draws the model, and thus all its meshes
	void Draw(ShaderProgram shader);

	unsigned int instanceVBO;

private:
	uint32_t instanceCount = 0;

	/*  Functions   */
	// loads a model with supported ASSIMP extensions from file and stores the resulting meshes in the meshes vector.
	void loadModel(std::string const &path);

	// processes a node in a recursive fashion. Processes each individual mesh located at the node and repeats this process on its children nodes (if any).
	void processNode(aiNode *node, const aiScene *scene);

	Mesh processMesh(aiMesh *mesh, const aiScene *scene);

	// checks all material textures of a given type and loads the textures if they're not loaded yet.
	// the required info is returned as a Texture struct.
	tinystl::vector<Texture> loadMaterialTextures(aiMaterial *mat, aiTextureType type, std::string typeName);
};


/////////////////////
// SKINNED MESH
class SkinnedMesh
{
public:
	SkinnedMesh();

	~SkinnedMesh();

	bool LoadMesh(const std::string& Filename);

	void Render();

	uint32_t NumBones() const
	{
		return m_NumBones;
	}

	void BoneTransform(float TimeInSeconds, tinystl::vector<aiMatrix4x4>& Transforms);

private:
	std::string directory;
	tinystl::vector<Texture> InitMaterials(const aiMaterial* material, aiTextureType type, std::string typeName);

#define NUM_BONES_PER_VEREX 4

	struct BoneInfo
	{
		aiMatrix4x4 BoneOffset;
		aiMatrix4x4 FinalTransformation;

		BoneInfo()
		{
			//BoneOffset.SetZero();
			//FinalTransformation.SetZero();
		}
	};

	struct VertexBoneData
	{
		uint32_t IDs[NUM_BONES_PER_VEREX]  = { 0 };
		float Weights[NUM_BONES_PER_VEREX] = { 0.0f };

		VertexBoneData()
		{
			Reset();
		};

		void Reset()
		{
			memset(IDs, 0, sizeof(IDs) / sizeof(IDs[0]));
			memset(Weights, 0, sizeof(Weights) / sizeof(Weights[0]));
			//ZERO_MEM(IDs);
			//ZERO_MEM(Weights);
		}

		void AddBoneData(uint32_t BoneID, float Weight);
	};

	void CalcInterpolatedScaling(aiVector3D& Out, float AnimationTime, const aiNodeAnim* pNodeAnim);
	void CalcInterpolatedRotation(Quaternion& Out, float AnimationTime, const aiNodeAnim* pNodeAnim);
	void CalcInterpolatedPosition(aiVector3D& Out, float AnimationTime, const aiNodeAnim* pNodeAnim);
	uint32_t FindScaling(float AnimationTime, const aiNodeAnim* pNodeAnim);
	uint32_t FindRotation(float AnimationTime, const aiNodeAnim* pNodeAnim);
	uint32_t FindPosition(float AnimationTime, const aiNodeAnim* pNodeAnim);
	const aiNodeAnim* FindNodeAnim(const aiAnimation* pAnimation, const std::string NodeName);
	void ReadNodeHeirarchy(float AnimationTime, const aiNode* pNode, const aiMatrix4x4& ParentTransform);
	bool InitFromScene(const aiScene* pScene, const std::string& Filename);
	void InitMesh(uint32_t MeshIndex,
		const aiMesh* paiMesh,
		tinystl::vector<aiVector3D>& Positions,
		tinystl::vector<aiVector3D>& Normals,
		tinystl::vector<aiVector2D>& TexCoords,
		tinystl::vector<VertexBoneData>& Bones,
		tinystl::vector<unsigned int>& Indices);
	void LoadBones(uint32_t MeshIndex, const aiMesh* paiMesh, tinystl::vector<VertexBoneData>& Bones);

#define INVALID_MATERIAL 0xFFFFFFFF

	enum VB_TYPES {
		INDEX_BUFFER,
		POS_VB,
		NORMAL_VB,
		TEXCOORD_VB,
		BONE_VB,
		NUM_VBs
	};

	uint32_t m_VAO;
	uint32_t m_Buffers[NUM_VBs];

	struct MeshEntry {
		MeshEntry()
		{
			NumIndices = 0;
			BaseVertex = 0;
			BaseIndex = 0;
			MaterialIndex = INVALID_MATERIAL;
		}

		unsigned int NumIndices;
		unsigned int BaseVertex;
		unsigned int BaseIndex;
		unsigned int MaterialIndex;
	};

	tinystl::vector<MeshEntry> m_Entries;
	tinystl::vector<Texture> m_Textures;

	std::unordered_map<std::string, uint32_t> m_BoneMapping; // maps a bone name to its index
	uint32_t m_NumBones;
	tinystl::vector<BoneInfo> m_BoneInfo;
	aiMatrix4x4 m_GlobalInverseTransform;

	const aiScene* m_pScene;
	Assimp::Importer m_Importer;
};


/////////////////////
// CAMERA

enum Camera_Movement
{
	FORWARD,
	BACKWARD,
	LEFT,
	RIGHT
};

// Default camera values
const float YAW = -90.0f;
const float PITCH = 0.0f;
const float SPEED = 2.5f;
const float SENSITIVITY = 0.1f;
const float ZOOM = 45.0f;

class Camera
{
public:
	// Camera Attributes
	glm::vec3 Position;
	glm::vec3 Front;
	glm::vec3 Up;
	glm::vec3 Right;
	glm::vec3 WorldUp;
	// Euler Angles
	float Yaw;
	float Pitch;
	// Camera options
	float MovementSpeed;
	float MouseSensitivity;
	float Zoom;

	// Constructor with vectors
	Camera(glm::vec3 position = glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3 up = glm::vec3(0.0f, 1.0f, 0.0f), float yaw = YAW, float pitch = PITCH);
	// Constructor with scalar values
	Camera(float posX, float posY, float posZ, float upX, float upY, float upZ, float yaw, float pitch);

	// Returns the view matrix calculated using Euler Angles and the LookAt Matrix
	glm::mat4 GetViewMatrix();

	// Processes input received from any keyboard-like input system. Accepts input parameter in the form of camera defined ENUM (to abstract it from windowing systems)
	void ProcessKeyboard(Camera_Movement direction, float deltaTime);

	// Processes input received from a mouse input system. Expects the offset value in both the x and y direction.
	void ProcessMouseMovement(float xoffset, float yoffset, GLboolean constrainPitch = true);

	// Processes input received from a mouse scroll-wheel event. Only requires input on the vertical wheel-axis
	void ProcessMouseScroll(float yoffset);

private:
	// Calculates the front vector from the Camera's (updated) Euler Angles
	void updateCameraVectors();
};

uint32_t LoadTexture(const char*);
Model* LoadModel(const char*, bool, uint32_t instanceCount = 0);

void BindQuadVAO();
void BindCubeVAO();

class OpenGLRenderer
{
public:
	OpenGLRenderer();
	~OpenGLRenderer();

	void RenderQuad();
	void RenderQuadInstanced(int numOfInstances);
	void RenderCube();
	void RenderCubeInstanced(int numOfInstances);
};