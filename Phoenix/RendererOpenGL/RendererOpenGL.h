#pragma once

#include <GLFW/glfw3.h>
#include "../../Common/Thirdparty/stl/TINYSTL/string.h"

class ShaderProgram
{
public:
	ShaderProgram(tinystl::string vertexShaderPath, tinystl::string fragmentShaderPath);
	~ShaderProgram();

private:
	int shaderProgram;
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
	int windowShouldClose();
	void destroyWindow();

private:
	GLFWwindow* window;

	int WIDTH;
	int HEIGHT;
};