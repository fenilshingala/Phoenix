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
// SKINNED MESH
struct Texture
{
	unsigned int id;
	std::string type;
	std::string path;
};

class SkinnedMesh
{
public:
	SkinnedMesh();

	~SkinnedMesh();

	bool LoadMesh(const std::string& Filename, uint32_t instanceCount = 0);
	void AddAnimation(const std::string& Filename);

	void Render(ShaderProgram shader);

	uint32_t NumBones() const
	{
		return m_NumBones;
	}

	void BoneTransform(float TimeInSeconds, tinystl::vector<aiMatrix4x4>& Transforms, tinystl::vector<aiMatrix4x4>& BoneTransforms);

	struct LineSegment
	{
		aiMatrix4x4 mParent;
		aiMatrix4x4 mChild;

		LineSegment(aiMatrix4x4 parent, aiMatrix4x4 child) : mParent(parent), mChild(child)
		{
		}
	};
	std::vector<LineSegment> mLineSegments;

	std::vector<const aiAnimation*> mAnimations;
	void SetCurrentAnimation(int& index);

	bool mIsAnim = false;
	unsigned int instanceVBO;

private:
	int mCurrentAnimationIndex = 0;

	std::string directory;
	tinystl::vector<Texture> textures_loaded;
	tinystl::vector<Texture> InitMaterials(const aiMaterial* material, aiTextureType type, std::string typeName);

#define NUM_BONES_PER_VEREX 4

	struct BoneInfo
	{
		aiMatrix4x4 BoneOffset;
		aiMatrix4x4 FinalTransformation;
		aiMatrix4x4 m_BoneInverseTransform;

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
	uint32_t mInstanceCount = 0;


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
	//tinystl::vector<Texture> m_Textures;
	tinystl::unordered_map<uint32_t, tinystl::vector<Texture>> mMeshTexturesMap;

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

uint32_t LoadTexture(const char*, bool isHDR = false);

struct InstanceData
{
	glm::mat4 model;
	glm::vec3 color;
};

enum PBRTextureType
{
	ALBEDO,
	NORMAL,
	METALLIC,
	ROUGHNESS,
	AO
};

struct PBRMat_Tex
{
#define INVALID_TEXTURE_ID -1

	~PBRMat_Tex();
	void LoadPBRTexture(const char* filepath, PBRTextureType type);
	void BindTextures();

private:
	unsigned int albedo		= INVALID_TEXTURE_ID;
	unsigned int normal		= INVALID_TEXTURE_ID;
	unsigned int metallic	= INVALID_TEXTURE_ID;
	unsigned int roughness	= INVALID_TEXTURE_ID;
	unsigned int ao			= INVALID_TEXTURE_ID;
};

struct PBRMat
{
	inline void setAlbedo(glm::vec3 a_vAlbedo)		{ albedo = a_vAlbedo; }
	inline void setMetallic(float a_fMetallic)		{ metallic = a_fMetallic; }
	inline void setRoughness(float a_fRoughness)	{ roughness = a_fRoughness; }
	inline void setAo(float a_fAo)					{ ao = a_fAo; }

	inline glm::vec3 getAlbedo()	{ return albedo; }
	inline float getMetallic()		{ return metallic; }
	inline float getRoughness()		{ return roughness; }
	inline float getAo()			{ return ao; }

	void UpdateMaterial(ShaderProgram* pShaderProgram);

	PBRMat() {}

	PBRMat(glm::vec3 a_vAlbedo, float a_fMetallic, float a_fRoughness, float a_fAo):
	albedo(a_vAlbedo),
	metallic(a_fMetallic),
	roughness(a_fRoughness),
	ao(a_fAo)
	{}

private:
	glm::vec3	albedo;
	float		metallic;
	float		roughness;
	float		ao;
};

class OpenGLRenderer
{
public:
	OpenGLRenderer();
	~OpenGLRenderer();

	void RenderLine();

	void RenderQuad();
	void RenderQuadInstanced(int numOfInstances);
	void UpdateQuadInstanceBuffer(uint32_t size, void* data);

	void RenderCube();
	void UpdateCubeInstanceBuffer(uint32_t size, void* data);
	void RenderCubeInstanced(int numOfInstances);

	void RenderSphere();
	void UpdateSphereInstanceBuffer(uint32_t size, void* data);
	void RenderSphereInstanced(int numOfInstances);

	glm::mat4 ModelMatForLineBWTwoPoints(glm::vec3 A, glm::vec3 B);

private:
#define INVALID_BUFFER_ID -1

	//////////////////////////////////////////////// INSTANCE VBO
	void attachInstanceVBO(unsigned int& vbo);
	void setupBasicShapeBuffers(unsigned int vao, unsigned int instanceVBO);

	//////////////////////////////////////////////// LINE
	unsigned int mLineVAO = INVALID_BUFFER_ID;
	unsigned int mLineVBO = INVALID_BUFFER_ID;
	void setupLine();

	//////////////////////////////////////////////// QUAD
	unsigned int mQuadVAO = INVALID_BUFFER_ID;
	unsigned int mQuadVBO = INVALID_BUFFER_ID;

	unsigned int quadInstanceVAO	= INVALID_BUFFER_ID;
	unsigned int quadInstanceBuffer = INVALID_BUFFER_ID;

	void setupQuad();

	//////////////////////////////////////////////// CUBE
	unsigned int mCubeVAO = INVALID_BUFFER_ID;
	unsigned int mCubeVBO = INVALID_BUFFER_ID;

	unsigned int cubeInstanceVAO	= INVALID_BUFFER_ID;
	unsigned int cubeInstanceBuffer = INVALID_BUFFER_ID;
	
	void setupCube();

	//////////////////////////////////////////////// SPHERE
	unsigned int sphereIndexCount  = INVALID_BUFFER_ID;
	unsigned int sphereVAO		   = INVALID_BUFFER_ID;

	unsigned int sphereInstanceVAO		= INVALID_BUFFER_ID;
	unsigned int sphereInstanceBuffer	= INVALID_BUFFER_ID;
	
	void setupSphere();
};