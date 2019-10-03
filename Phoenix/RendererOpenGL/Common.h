#pragma once

#include "Picker.h"

#include "Window.h"
#include "RendererOpenGL.h"

bool exitOnESC = false;

Camera camera(glm::vec3(0.0f, 0.0f, 10.0f));
float lastX = 0.0f;
float lastY = 0.0f;
bool firstMouse = true;

// timing
float deltaTime = 0.0f;
float lastFrame = 0.0f;

Window window;
//OpenGLRenderer renderer;

void processInputs()
{
	deltaTime = 0.032f;

	if (window.isKeyPressed(PH_KEY_LSHIFT))
	{
		deltaTime *= 2;
	}

	if (window.isKeyTriggered(PH_KEY_ESCAPE))
	{
		exitOnESC = true;
	}
	if (window.isKeyPressed(PH_KEY_W))
	{
		camera.ProcessKeyboard(FORWARD, deltaTime);
	}
	if (window.isKeyPressed(PH_KEY_S))
	{
		camera.ProcessKeyboard(BACKWARD, deltaTime);
	}
	if (window.isKeyPressed(PH_KEY_A))
	{
		camera.ProcessKeyboard(LEFT, deltaTime);
	}
	if (window.isKeyPressed(PH_KEY_D))
	{
		camera.ProcessKeyboard(RIGHT, deltaTime);
	}

	float xpos = (float)window.mouseX();
	float ypos = (float)window.mouseY();
	if (firstMouse)
	{
		lastX = xpos;
		lastY = ypos;
		firstMouse = false;
	}

	float xoffset = xpos - lastX;
	float yoffset = lastY - ypos; // reversed since y-coordinates go from bottom to top

	lastX = xpos;
	lastY = ypos;

	if (window.isRightClicked())
	{
		camera.ProcessMouseMovement(xoffset, yoffset);
		camera.ProcessMouseScroll((float)window.scrollY());
	}

}

std::string str;