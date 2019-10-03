#include "../Picker.h"

#if FORWARD_LIGHTING

#include "../Common.h"

// lighting
glm::vec3 lightPos(1.2f, 1.0f, 2.0f);

void Run()
{
	window.initWindow();
	lastX = window.windowWidth()  / 2.0f;
	lastY = window.windowHeight() / 2.0f;

	glEnable(GL_DEPTH_TEST);
	
	ShaderProgram lightingShader("../../Phoenix/RendererOpenGL/App/Resources/Shaders/basic_lighting.vert",
								 "../../Phoenix/RendererOpenGL/App/Resources/Shaders/basic_lighting.frag");
	ShaderProgram lampShader("../../Phoenix/RendererOpenGL/App/Resources/Shaders/mesh.vert",
							 "../../Phoenix/RendererOpenGL/App/Resources/Shaders/mesh.frag");

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

	//uint32_t texture0 = LoadTexture("../../Phoenix/App/Test/textures/container.jpg");
	//uint32_t texture1 = LoadTexture("../../Phoenix/App/Test/textures/awesomeface.png");
	
	//int zero = 0, one = 1;
	//glUseProgram(shaderProgram.mId);
	//shaderProgram.SetUniform("texture1", &zero);
	//shaderProgram.SetUniform("texture2", &one);

	//glm::mat4 projection = glm::perspective(glm::radians(45.0f), (float)window.windowWidth() / (float)window.windowHeight(), 0.1f, 100.0f);
	//shaderProgram.SetUniform("projection", &projection);

	window.initGui();

	while (!window.windowShouldClose() && !exitOnESC)
	{
		window.startFrame();

		{
			glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
			glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

			glm::mat4 projection = glm::perspective(glm::radians(camera.Zoom), (float)window.windowWidth() / (float)window.windowHeight(), 0.1f, 100.0f);
			glm::mat4 view = camera.GetViewMatrix();

			// draw our first triangle
			glUseProgram(lightingShader.mId);
			glm::vec3 objectColor = glm::vec3(1.0f, 0.5f, 0.31f);
			lightingShader.SetUniform("objectColor", &objectColor);
			glm::vec3 lightColor = glm::vec3(1.0f, 1.0f, 1.0f);
			lightingShader.SetUniform("lightColor", &lightColor);
			lightingShader.SetUniform("lightPos", &lightPos);
			lightingShader.SetUniform("viewPos", &camera.Position);

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

			// GUI
			{
				window.beginGuiFrame();

				bool truebool = true;
				
				str = "controlled fps: " + std::to_string(window.frameRate());
				ImGui::Begin("BLEH!", &truebool);
				ImGui::Text(str.c_str());
				
				str = "actual fps: " + std::to_string(window.actualFrameRate());
				ImGui::Text(str.c_str());
				ImGui::End();

				window.endGuiFrame();
			}

			window.swapWindow();

			window.update();
			processInputs();
		}

		window.endFrame();
	}

	window.exitGui();

	glDeleteVertexArrays(1, &cubeVAO);
	glDeleteVertexArrays(1, &lightVAO);
	glDeleteBuffers(1, &VBO);

	window.exitWindow();
}

#endif