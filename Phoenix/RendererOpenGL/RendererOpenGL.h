#pragma once

#include <unordered_map>
#include <string>

#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include "../../Common/Thirdparty/TINYSTL/string.h"
#include "../../Common/Renderer/keyBindings.h"

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <imgui/imgui.h>
#include <imgui/imgui_impl_glfw.h>
#include <imgui/imgui_impl_opengl3.h>

struct Uniform
{
	uint16_t location;
	GLenum type;
};

class ShaderProgram
{
public:
	ShaderProgram(tinystl::string vertexShaderPath, tinystl::string fragmentShaderPath);
	~ShaderProgram();

	void SetUniform(const char*, void*);

	int mId;
	std::unordered_map<size_t, Uniform> mUniformVarMap;
};

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

class OpenGLRenderer
{
public:
	OpenGLRenderer() : window(nullptr), WIDTH(800), HEIGHT(600)
	{}

	~OpenGLRenderer()
	{}

	// WINDOW
	void initWindow(int _WIDTH = 800, int _HEIGHT = 600);
	void pollEvents();
	void waitEvents();
	int windowShouldClose();
	void destroyWindow();
	void swapwindow();
	inline int windowWidth()  { return WIDTH; }
	inline int windowHeight() { return HEIGHT; }

	void updateInputs();
	bool isKeyPressed(uint32_t _key);
	bool isKeyTriggered(uint32_t _key);
	bool isKeyReleased(uint32_t _key);
	float mouseX();
	float mouseY();
	float scrollX();
	float scrollY();
	bool isMouseLeftPressed();
	bool isMouseRightPressed();

	// OPENGL
	uint32_t LoadTexture(const char*);

	// GUI
	void initGui();
	void beginGuiFrame();
	void endGuiFrame();

private:
	GLFWwindow* window;

	int WIDTH;
	int HEIGHT;
};