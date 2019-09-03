#include "RendererOpenGL.h"

#include <assert.h>
#include <sstream>
#include <fstream>
#include <glm/glm.hpp>

////////////////////////////////////////////////
// WINDOW

void framebuffer_size_callback(GLFWwindow* window, int width, int height)
{	glViewport(0, 0, width, height);	}

#define NUM_OF_KEY_BINDINGS PH_KEY_MAX
uint32_t keyPressBindings[NUM_OF_KEY_BINDINGS] = { 0 };
uint32_t prevKeyPressBindings[NUM_OF_KEY_BINDINGS] = { 0 };
uint32_t keyReleaseBindings[NUM_OF_KEY_BINDINGS] = { 0 };
uint32_t prevKeyReleaseBindings[NUM_OF_KEY_BINDINGS] = { 0 };
void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods)
{
	if (key >= NUM_OF_KEY_BINDINGS)
		return;
	if (action == GLFW_PRESS)
		keyPressBindings[key] = 1;
	if (action == GLFW_RELEASE)
		keyReleaseBindings[key] = 1;
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

	assert(gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)); // Failed to initialize GLAD
}

void OpenGLRenderer::pollEvents() {	glfwPollEvents(); }
void OpenGLRenderer::waitEvents() { glfwWaitEvents(); }
int OpenGLRenderer::windowShouldClose() {	return glfwWindowShouldClose(window);	}
void OpenGLRenderer::destroyWindow()	{	glfwDestroyWindow(window);  glfwTerminate();	}

void OpenGLRenderer::updateInputs()
{
	memcpy(prevKeyPressBindings, keyPressBindings, NUM_OF_KEY_BINDINGS);
	memset(keyPressBindings, 0, NUM_OF_KEY_BINDINGS);

	memcpy(prevKeyReleaseBindings, keyReleaseBindings, NUM_OF_KEY_BINDINGS);
	memset(keyReleaseBindings, 0, NUM_OF_KEY_BINDINGS);
}

bool OpenGLRenderer::isKeyPressed(uint32_t _key)
{
	if (keyPressBindings[_key])
		return true;
	return false;
	GLFW_KEY_0;
}

bool OpenGLRenderer::isKeyTriggered(uint32_t _key)
{
	if (!prevKeyPressBindings[_key] && keyPressBindings[_key])
		return true;
	return false;
}


//////////////////////////////////////////////////////////
// OPENGL 

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
	for (uint32_t i = 0; i < count; ++i)
	{
		glGetActiveAttrib(mId, i, bufferSize, &length, &size, &type, name);
		int x = 0;
	}


	glGetProgramiv(mId, GL_ACTIVE_UNIFORMS, &count);
	for (uint32_t i = 0; i < count; ++i)
	{
		glGetActiveUniform(mId, i, bufferSize, &length, &size, &type, name);
		Uniform uniformInfo;
		uniformInfo.location = glGetUniformLocation(mId, name);
		uniformInfo.type = type;
		mUniformVarMap.insert( {hasher(name), uniformInfo} );
	}
}

ShaderProgram::~ShaderProgram()
{}

void ShaderProgram::SetUniform(const char* name, void* data)
{
	std::unordered_map<size_t, Uniform>::iterator itr = mUniformVarMap.find(hasher(name));
	if (itr != mUniformVarMap.end())
	{
		Uniform uniformInfo = itr->second;
		switch (uniformInfo.type)
		{
		case GL_FLOAT:
			glUniform1fv(uniformInfo.location, 1, (float*)data); break;
		case GL_FLOAT_VEC2:
			glUniform2fv(uniformInfo.location, 1, (float*)data); break;
		case GL_FLOAT_VEC3:
			glUniform3fv(uniformInfo.location, 1, (float*)data); break;
		case GL_FLOAT_VEC4:
			glUniform4fv(uniformInfo.location, 1, (float*)data); break;
		default:
			break;
		}
	}
}

void OpenGLRenderer::swapwindow() { glfwSwapBuffers(window); }