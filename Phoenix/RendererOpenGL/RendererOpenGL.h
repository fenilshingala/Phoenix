#pragma once

#include <unordered_map>
#include <string>

#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include "../../Common/Thirdparty/TINYSTL/string.h"
#include "../../Common/Renderer/keyBindings.h"

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

private:
	std::hash<const char*> hasher;
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

	void updateInputs();
	bool isKeyPressed(uint32_t _key);
	bool isKeyTriggered(uint32_t _key);

	// OPENGL

private:
	GLFWwindow* window;

	int WIDTH;
	int HEIGHT;
};