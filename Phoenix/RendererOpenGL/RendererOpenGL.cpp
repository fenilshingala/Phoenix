#include <glad/glad.h>
#include "RendererOpenGL.h"

#include <assert.h>
#include <string>
#include <sstream>
#include <fstream>

void framebuffer_size_callback(GLFWwindow* window, int width, int height)
{	glViewport(0, 0, width, height);	}

// WINDOW
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

	assert(!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)); // Failed to initialize GLAD
}

void OpenGLRenderer::pollEvents()		{	glfwPollEvents();	}
int OpenGLRenderer::windowShouldClose() {	return glfwWindowShouldClose(window);	}
void OpenGLRenderer::destroyWindow()	{	glfwDestroyWindow(window);  glfwTerminate();	}


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
		const char* vertexShaderSource = ss[0].str().c_str();

		vertexShader = glCreateShader(GL_VERTEX_SHADER);
		glShaderSource(vertexShader, 1, &vertexShaderSource, NULL);
		glCompileShader(vertexShader);
		
		int success;
		char infoLog[512];
		glGetShaderiv(vertexShader, GL_COMPILE_STATUS, &success);
		if (!success)
		{
			glGetShaderInfoLog(vertexShader, 512, NULL, infoLog);
			//std::cout << "ERROR::SHADER::VERTEX::COMPILATION_FAILED\n" << infoLog << std::endl;
		}
	}

	// frag
	{
		std::ifstream fragStream(vertexShaderPath.c_str());
		while (getline(fragStream, line))
		{
			ss[1] << line << '\n';
		}
		const char* fragmentShaderSource = ss[1].str().c_str();

		fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
		glShaderSource(fragmentShader, 1, &fragmentShaderSource, NULL);
		glCompileShader(fragmentShader);
		
		int success;
		char infoLog[512];
		glGetShaderiv(fragmentShader, GL_COMPILE_STATUS, &success);
		if (!success)
		{
			glGetShaderInfoLog(fragmentShader, 512, NULL, infoLog);
			//std::cout << "ERROR::SHADER::FRAGMENT::COMPILATION_FAILED\n" << infoLog << std::endl;
		}
	}

	shaderProgram = glCreateProgram();
	glAttachShader(shaderProgram, vertexShader);
	glAttachShader(shaderProgram, fragmentShader);
	glLinkProgram(shaderProgram);
	
	int success;
	char infoLog[512];
	glGetProgramiv(shaderProgram, GL_LINK_STATUS, &success);
	if (!success) {
		glGetProgramInfoLog(shaderProgram, 512, NULL, infoLog);
		//std::cout << "ERROR::SHADER::PROGRAM::LINKING_FAILED\n" << infoLog << std::endl;
	}
	glDeleteShader(vertexShader);
	glDeleteShader(fragmentShader);
}

ShaderProgram::~ShaderProgram()
{}