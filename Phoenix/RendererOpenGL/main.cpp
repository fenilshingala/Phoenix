#include "RendererOpenGL.h"

#include <iostream>

bool exitOnESC = false;

Camera camera(glm::vec3(0.0f, 0.0f, 3.0f));
float lastX = 0.0f;
float lastY = 0.0f;
bool firstMouse = true;

// timing
float deltaTime = 0.0f;
float lastFrame = 0.0f;

// lighting
glm::vec3 lightPos(1.2f, 1.0f, 2.0f);

OpenGLRenderer renderer;

void processInputs()
{
	if (renderer.isKeyTriggered(PH_KEY_ESCAPE))
	{
		exitOnESC = true;
	}
	if (renderer.isKeyPressed(PH_KEY_W))
	{
		camera.ProcessKeyboard(FORWARD, deltaTime);
	}
	if (renderer.isKeyPressed(PH_KEY_S))
	{
		camera.ProcessKeyboard(BACKWARD, deltaTime);
	}
	if (renderer.isKeyPressed(PH_KEY_A))
	{
		camera.ProcessKeyboard(LEFT, deltaTime);
	}
	if (renderer.isKeyPressed(PH_KEY_D))
	{
		camera.ProcessKeyboard(RIGHT, deltaTime);
	}

	float xpos = renderer.mouseX();
	float ypos = renderer.mouseY();
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

	if (renderer.isMouseLeftPressed())
	{
		camera.ProcessMouseMovement(xoffset, yoffset);
		camera.ProcessMouseScroll(renderer.scrollY());
	}

}

int main()
{
	renderer.initWindow();
	camera.MovementSpeed *= 16;

	lastX = renderer.windowWidth()  / 2.0f;
	lastY = renderer.windowHeight() / 2.0f;
	
	glEnable(GL_DEPTH_TEST);

	ShaderProgram lightingShader("../../Phoenix/RendererOpenGL/Shaders/basic_lighting.vert", "../../Phoenix/RendererOpenGL/Shaders/basic_lighting.frag");
	ShaderProgram lampShader("../../Phoenix/RendererOpenGL/Shaders/lamp.vert", "../../Phoenix/RendererOpenGL/Shaders/lamp.frag");

	float vertices[] = {
		-0.5f, -0.5f, -0.5f,  0.0f,  0.0f, -1.0f,
		 0.5f, -0.5f, -0.5f,  0.0f,  0.0f, -1.0f,
		 0.5f,  0.5f, -0.5f,  0.0f,  0.0f, -1.0f,
		 0.5f,  0.5f, -0.5f,  0.0f,  0.0f, -1.0f,
		-0.5f,  0.5f, -0.5f,  0.0f,  0.0f, -1.0f,
		-0.5f, -0.5f, -0.5f,  0.0f,  0.0f, -1.0f,

		-0.5f, -0.5f,  0.5f,  0.0f,  0.0f,  1.0f,
		 0.5f, -0.5f,  0.5f,  0.0f,  0.0f,  1.0f,
		 0.5f,  0.5f,  0.5f,  0.0f,  0.0f,  1.0f,
		 0.5f,  0.5f,  0.5f,  0.0f,  0.0f,  1.0f,
		-0.5f,  0.5f,  0.5f,  0.0f,  0.0f,  1.0f,
		-0.5f, -0.5f,  0.5f,  0.0f,  0.0f,  1.0f,

		-0.5f,  0.5f,  0.5f, -1.0f,  0.0f,  0.0f,
		-0.5f,  0.5f, -0.5f, -1.0f,  0.0f,  0.0f,
		-0.5f, -0.5f, -0.5f, -1.0f,  0.0f,  0.0f,
		-0.5f, -0.5f, -0.5f, -1.0f,  0.0f,  0.0f,
		-0.5f, -0.5f,  0.5f, -1.0f,  0.0f,  0.0f,
		-0.5f,  0.5f,  0.5f, -1.0f,  0.0f,  0.0f,

		 0.5f,  0.5f,  0.5f,  1.0f,  0.0f,  0.0f,
		 0.5f,  0.5f, -0.5f,  1.0f,  0.0f,  0.0f,
		 0.5f, -0.5f, -0.5f,  1.0f,  0.0f,  0.0f,
		 0.5f, -0.5f, -0.5f,  1.0f,  0.0f,  0.0f,
		 0.5f, -0.5f,  0.5f,  1.0f,  0.0f,  0.0f,
		 0.5f,  0.5f,  0.5f,  1.0f,  0.0f,  0.0f,

		-0.5f, -0.5f, -0.5f,  0.0f, -1.0f,  0.0f,
		 0.5f, -0.5f, -0.5f,  0.0f, -1.0f,  0.0f,
		 0.5f, -0.5f,  0.5f,  0.0f, -1.0f,  0.0f,
		 0.5f, -0.5f,  0.5f,  0.0f, -1.0f,  0.0f,
		-0.5f, -0.5f,  0.5f,  0.0f, -1.0f,  0.0f,
		-0.5f, -0.5f, -0.5f,  0.0f, -1.0f,  0.0f,

		-0.5f,  0.5f, -0.5f,  0.0f,  1.0f,  0.0f,
		 0.5f,  0.5f, -0.5f,  0.0f,  1.0f,  0.0f,
		 0.5f,  0.5f,  0.5f,  0.0f,  1.0f,  0.0f,
		 0.5f,  0.5f,  0.5f,  0.0f,  1.0f,  0.0f,
		-0.5f,  0.5f,  0.5f,  0.0f,  1.0f,  0.0f,
		-0.5f,  0.5f, -0.5f,  0.0f,  1.0f,  0.0f
	};
	// first, configure the cube's VAO (and VBO)
	unsigned int VBO, cubeVAO;
	glGenVertexArrays(1, &cubeVAO);
	glGenBuffers(1, &VBO);

	glBindBuffer(GL_ARRAY_BUFFER, VBO);
	glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

	glBindVertexArray(cubeVAO);

	// position attribute
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)0);
	glEnableVertexAttribArray(0);
	// normal attribute
	glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)(3 * sizeof(float)));
	glEnableVertexAttribArray(1);


	// second, configure the light's VAO (VBO stays the same; the vertices are the same for the light object which is also a 3D cube)
	unsigned int lightVAO;
	glGenVertexArrays(1, &lightVAO);
	glBindVertexArray(lightVAO);

	glBindBuffer(GL_ARRAY_BUFFER, VBO);
	// note that we update the lamp's position attribute's stride to reflect the updated buffer data
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)0);
	glEnableVertexAttribArray(0);

	/*uint32_t texture0 = renderer.LoadTexture("../../Phoenix/App/Test/textures/container.jpg");
	uint32_t texture1 = renderer.LoadTexture("../../Phoenix/App/Test/textures/awesomeface.png");
	
	int zero = 0, one = 1;
	glUseProgram(shaderProgram.mId);
	shaderProgram.SetUniform("texture1", &zero);
	shaderProgram.SetUniform("texture2", &one);

	glm::mat4 projection = glm::perspective(glm::radians(45.0f), (float)renderer.windowWidth() / (float)renderer.windowHeight(), 0.1f, 100.0f);
	shaderProgram.SetUniform("projection", &projection);*/

	while (!renderer.windowShouldClose() && !exitOnESC)
	{
		float currentFrame = (float)glfwGetTime();
		deltaTime = currentFrame - lastFrame;
		lastFrame = currentFrame;

		glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		// draw our first triangle
		glUseProgram(lightingShader.mId);
		glm::vec3 objectColor = glm::vec3(1.0f, 0.5f, 0.31f);
		lightingShader.SetUniform("objectColor", &objectColor);
		glm::vec3 lightColor = glm::vec3(1.0f, 1.0f, 1.0f);
		lightingShader.SetUniform("lightColor", &lightColor);
		lightingShader.SetUniform("lightPos", &lightPos);
		lightingShader.SetUniform("viewPos", &camera.Position);

		glm::mat4 projection = glm::perspective(glm::radians(camera.Zoom), (float)renderer.windowWidth() / (float)renderer.windowHeight(), 0.1f, 100.0f);
		glm::mat4 view = camera.GetViewMatrix();
		lightingShader.SetUniform("projection", &projection);
		lightingShader.SetUniform("view", &view);

		glm::mat4 model = glm::mat4(1.0f);
		lightingShader.SetUniform("model", &model);

		// render the cube
		glBindVertexArray(cubeVAO);
		glDrawArrays(GL_TRIANGLES, 0, 36);


		// also draw the lamp object
		glUseProgram(lampShader.mId);
		
		lampShader.SetUniform("projection", &projection);
		lampShader.SetUniform("view", &view);
		model = glm::mat4(1.0f);
		model = glm::translate(model, lightPos);
		model = glm::scale(model, glm::vec3(0.2f)); // a smaller cube
		lampShader.SetUniform("model", &model);

		glBindVertexArray(lightVAO);
		glDrawArrays(GL_TRIANGLES, 0, 36);

		renderer.swapwindow();

		renderer.pollEvents();
		renderer.updateInputs();

		processInputs();
	}

	glDeleteVertexArrays(1, &cubeVAO);
	glDeleteVertexArrays(1, &lightVAO);
	glDeleteBuffers(1, &VBO);

	renderer.destroyWindow();

	return 0;
}