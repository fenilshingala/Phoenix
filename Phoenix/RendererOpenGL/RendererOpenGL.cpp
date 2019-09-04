#include "RendererOpenGL.h"

#include <assert.h>
#include <sstream>
#include <fstream>
#include <functional>

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>

#include <iostream>
	
////////////////////////////////////////////////
// WINDOW

void framebuffer_size_callback(GLFWwindow* window, int width, int height)
{	glViewport(0, 0, width, height);	}

#define NUM_OF_KEY_BINDINGS PH_KEY_MAX
uint32_t keyPressBindings[NUM_OF_KEY_BINDINGS] = { 0 };
uint32_t prevKeyPressBindings[NUM_OF_KEY_BINDINGS] = { 0 };
uint32_t currKeyPressBindings[NUM_OF_KEY_BINDINGS] = { 0 };

uint32_t keyReleaseBindings[NUM_OF_KEY_BINDINGS] = { 0 };
uint32_t prevKeyReleaseBindings[NUM_OF_KEY_BINDINGS] = { 0 };
uint32_t currKeyReleaseBindings[NUM_OF_KEY_BINDINGS] = { 0 };
bool keysUpdated = false;
void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods)
{
	if (key >= NUM_OF_KEY_BINDINGS)
		return;
	if (action == GLFW_PRESS || GLFW_REPEAT)
		keyPressBindings[key] = 1;
	if (action == GLFW_RELEASE)
		keyReleaseBindings[key] = 1;
}

float mousePosX = 0.0f, mousePosY = 0.0f;
void mouse_callback(GLFWwindow* window, double xpos, double ypos)
{
	mousePosX = (float)xpos; mousePosY = (float)ypos;
}

float scrollXOffset = 0.0f, scrollYOffset = 0.0f;
void scroll_callback(GLFWwindow* window, double xoffset, double yoffset)
{
	scrollXOffset = (float)xoffset;
	scrollYOffset = (float)yoffset;
}

bool mouseLeft = false, mouseRight = false;
void mouse_button_callback(GLFWwindow* window, int button, int action, int mods)
{
	if (button == GLFW_MOUSE_BUTTON_RIGHT && action == GLFW_PRESS)
		mouseRight = true;
	if (button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_PRESS)
		mouseLeft = true;

	if (button == GLFW_MOUSE_BUTTON_RIGHT && action == GLFW_RELEASE && mouseRight)
		mouseRight = false;
	if (button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_RELEASE && mouseLeft)
		mouseLeft = false;
}

void OpenGLRenderer::initWindow(int _WIDTH, int _HEIGHT)
{
	WIDTH = _WIDTH;		HEIGHT = _HEIGHT;
	glfwInit();

	glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

	window = glfwCreateWindow(WIDTH, HEIGHT, "Renderer", nullptr, nullptr);

	glfwMakeContextCurrent(window);
	glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
	glfwSetKeyCallback(window, key_callback);
	glfwSetCursorPosCallback(window, mouse_callback);
	glfwSetScrollCallback(window, scroll_callback);
	glfwSetMouseButtonCallback(window, mouse_button_callback);

	glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);

	assert(gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)); // Failed to initialize GLAD
}

void OpenGLRenderer::pollEvents() {	glfwPollEvents(); }
void OpenGLRenderer::waitEvents() { glfwWaitEvents(); }
int OpenGLRenderer::windowShouldClose() {	return glfwWindowShouldClose(window);	}
void OpenGLRenderer::destroyWindow()	{	glfwDestroyWindow(window);  glfwTerminate();	}

void OpenGLRenderer::updateInputs()
{
	memcpy(prevKeyPressBindings, currKeyPressBindings, sizeof(uint32_t) * NUM_OF_KEY_BINDINGS);
	memcpy(currKeyPressBindings, keyPressBindings, sizeof(uint32_t) * NUM_OF_KEY_BINDINGS);
	memset(keyPressBindings, 0, sizeof(uint32_t) * NUM_OF_KEY_BINDINGS);

	memcpy(prevKeyReleaseBindings, currKeyReleaseBindings, sizeof(uint32_t) *  NUM_OF_KEY_BINDINGS);
	memcpy(currKeyReleaseBindings, keyReleaseBindings, sizeof(uint32_t) * NUM_OF_KEY_BINDINGS);
	memset(keyReleaseBindings, 0, sizeof(uint32_t) * NUM_OF_KEY_BINDINGS);
	
}

bool OpenGLRenderer::isKeyPressed(uint32_t _key)
{
	assert(_key <= NUM_OF_KEY_BINDINGS);
	return currKeyPressBindings[_key];
}

bool OpenGLRenderer::isKeyTriggered(uint32_t _key)
{
	assert(_key <= NUM_OF_KEY_BINDINGS);
	return !prevKeyPressBindings[_key] && currKeyPressBindings[_key];
}

bool OpenGLRenderer::isKeyReleased(uint32_t _key)
{
	assert(_key <= NUM_OF_KEY_BINDINGS);
	return keyReleaseBindings[_key];
}

float OpenGLRenderer::mouseX()
{
	return mousePosX;
}

float OpenGLRenderer::mouseY()
{
	return mousePosY;
}

float OpenGLRenderer::scrollX()
{
	return scrollXOffset;
}

float OpenGLRenderer::scrollY()
{
	return scrollYOffset;
}

bool OpenGLRenderer::isMouseLeftPressed()
{
	return mouseLeft;
}

bool OpenGLRenderer::isMouseRightPressed()
{
	return mouseRight;
}


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

ShaderProgram::ShaderProgram(tinystl::string vertexShaderPath, tinystl::string fragmentShaderPath)
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
	const uint32_t bufferSize = 16;
	char name[bufferSize] = {};

	glGetProgramiv(mId, GL_ACTIVE_ATTRIBUTES, &count);
	for (int32_t i = 0; i < count; ++i)
	{
		glGetActiveAttrib(mId, i, bufferSize, &length, &size, &type, name);
		int x = 0;
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

void OpenGLRenderer::swapwindow() { glfwSwapBuffers(window); }

uint32_t OpenGLRenderer::LoadTexture(const char* path)
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
	case 1: internal_format = GL_R;    break;
	default: assert(0); break;
	}

	glTexImage2D(GL_TEXTURE_2D, 0, internal_format, width, height, 0, internal_format, GL_UNSIGNED_BYTE, data);
	
	glGenerateMipmap(GL_TEXTURE_2D);
	
	stbi_image_free(data);

	return texture;
}

void OpenGLRenderer::initGui()
{
	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	ImGuiIO& io = ImGui::GetIO(); (void)io;

	ImGui::StyleColorsDark();

	ImGui_ImplGlfw_InitForOpenGL(window, true);
	ImGui_ImplOpenGL3_Init("#version 130");

	Assimp::Importer importer;
	const aiScene *scene = importer.ReadFile("", aiProcess_Triangulate | aiProcess_FlipUVs);
}

void OpenGLRenderer::beginGuiFrame()
{
	ImGui_ImplOpenGL3_NewFrame();
	ImGui_ImplGlfw_NewFrame();
	ImGui::NewFrame();
}

void OpenGLRenderer::endGuiFrame()
{
	ImGui::Render();
	ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
	ImGui::EndFrame();
}