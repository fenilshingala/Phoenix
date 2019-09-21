#pragma once

#include <unordered_map>
#include <string>

#include <glad/glad.h>
#include "../../Common/Thirdparty/TINYSTL/vector.h"
#include "../../Common/Renderer/keyBindings.h"

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>

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
	Mesh(tinystl::vector<Vertex> vertices, tinystl::vector<unsigned int> indices, tinystl::vector<Texture> textures);

	// render the mesh
	void Draw(ShaderProgram shader);

private:
	/*  Render data  */
	unsigned int VBO, EBO;

	/*  Functions    */
	// initializes all the buffer objects/arrays
	void SetupMesh();
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
	Model(std::string const &path, bool gamma = false);

	// draws the model, and thus all its meshes
	void Draw(ShaderProgram shader);

private:
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

class OpenGLRenderer
{
public:
	OpenGLRenderer()
	{}

	~OpenGLRenderer()
	{}
};