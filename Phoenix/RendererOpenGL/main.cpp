#include "RendererOpenGL.h"
#include <glm/glm.hpp>

bool exitOnESC = false;
int main()
{
	OpenGLRenderer renderer;
	
	renderer.initWindow();
	
	ShaderProgram shaderProgram("../../Phoenix/App/Test/Shaders/basic.vert", "../../Phoenix/App/Test/Shaders/basic.frag");

	float vertices[] = {
		 0.5f,  0.5f, 0.0f,  // top right
		 0.5f, -0.5f, 0.0f,  // bottom right
		-0.5f, -0.5f, 0.0f,  // bottom left
		-0.5f,  0.5f, 0.0f   // top left 
	};
	unsigned int indices[] = {  // note that we start from 0!
		0, 1, 3,  // first Triangle
		1, 2, 3   // second Triangle
	};
	unsigned int VBO, VAO, EBO;
	glGenVertexArrays(1, &VAO);
	glGenBuffers(1, &VBO);
	glGenBuffers(1, &EBO);
	glBindVertexArray(VAO);

	glBindBuffer(GL_ARRAY_BUFFER, VBO);
	glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);

	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
	glEnableVertexAttribArray(0);

	glBindBuffer(GL_ARRAY_BUFFER, 0);

	glBindVertexArray(0);

	while (!renderer.windowShouldClose() && !exitOnESC)
	{
		renderer.updateInputs();

		//mainLoop();
		// render
		// ------
		glClearColor(0.2f, 0.3f, 0.3f, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT);

		glm::mat4 model(1.0f);
		glm::mat4 view(1.0f);

		shaderProgram.SetUniform("model", &model);
		shaderProgram.SetUniform("view", &view);

		// draw our first triangle
		glUseProgram(shaderProgram.mId);
		glBindVertexArray(VAO);
		glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);

		renderer.swapwindow();

		if (renderer.isKeyTriggered(PH_KEY_ESCAPE))
		{
			exitOnESC = true;
		}

		renderer.pollEvents();
	}

	renderer.destroyWindow();

	return 0;
}