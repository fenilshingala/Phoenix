#include "Window.h"
#include "RendererOpenGL.h"

#include <iostream>

bool exitOnESC = false;

Camera camera(glm::vec3(0.0f, 0.0f, 10.0f));
float lastX = 0.0f;
float lastY = 0.0f;
bool firstMouse = true;

// timing
float deltaTime = 0.0f;
float lastFrame = 0.0f;

// lighting
glm::vec3 lightPos(1.2f, 1.0f, 2.0f);

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


// lighting
/*
int main()
{
	window.initWindow();
	lastX = window.windowWidth()  / 2.0f;
	lastY = window.windowHeight() / 2.0f;

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

	//uint32_t texture0 = LoadTexture("../../Phoenix/App/Test/textures/container.jpg");
	//uint32_t texture1 = LoadTexture("../../Phoenix/App/Test/textures/awesomeface.png");
	
	//int zero = 0, one = 1;
	//glUseProgram(shaderProgram.mId);
	//shaderProgram.SetUniform("texture1", &zero);
	//shaderProgram.SetUniform("texture2", &one);

	//glm::mat4 projection = glm::perspective(glm::radians(45.0f), (float)window.windowWidth() / (float)window.windowHeight(), 0.1f, 100.0f);
	//shaderProgram.SetUniform("projection", &projection);

	window.initGui();

	Model ourModel("../../Phoenix/RendererOpenGL/Objects/Player.obj");

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

	return 0;
}
*/

//////////// DEFFERED SHADING ////////////

// renderCube() renders a 1x1 3D cube in NDC.
// -------------------------------------------------
unsigned int cubeVAO = 0;
unsigned int cubeVBO = 0;
void renderCube()
{
	glBindVertexArray(cubeVAO);
	glDrawArrays(GL_TRIANGLES, 0, 36);
	glBindVertexArray(0);
}

// renderQuad() renders a 1x1 XY quad in NDC
// -----------------------------------------
unsigned int quadVAO = 0;
unsigned int quadVBO;
void renderQuad()
{
	glBindVertexArray(quadVAO);
	glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
	glBindVertexArray(0);
}

int main()
{
	window.initWindow();
	lastX = window.windowWidth() / 2.0f;
	lastY = window.windowHeight() / 2.0f;

	glEnable(GL_DEPTH_TEST);

	//////////////////////////////////////////////// QUAD
	float quadVertices[] = {
		// positions        // texture Coords
		-1.0f,  1.0f, 0.0f, 0.0f, 1.0f,
		-1.0f, -1.0f, 0.0f, 0.0f, 0.0f,
		 1.0f,  1.0f, 0.0f, 1.0f, 1.0f,
		 1.0f, -1.0f, 0.0f, 1.0f, 0.0f,
	};
	// setup plane VAO
	glGenVertexArrays(1, &quadVAO);
	glGenBuffers(1, &quadVBO);
	glBindVertexArray(quadVAO);
	glBindBuffer(GL_ARRAY_BUFFER, quadVBO);
	glBufferData(GL_ARRAY_BUFFER, sizeof(quadVertices), &quadVertices, GL_STATIC_DRAW);
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)0);
	glEnableVertexAttribArray(1);
	glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)(3 * sizeof(float)));

	//////////////////////////////////////////////// CUBE
	float vertices[] = {
		// back face
		-1.0f, -1.0f, -1.0f,  0.0f,  0.0f, -1.0f, 0.0f, 0.0f, // bottom-left
		 1.0f,  1.0f, -1.0f,  0.0f,  0.0f, -1.0f, 1.0f, 1.0f, // top-right
		 1.0f, -1.0f, -1.0f,  0.0f,  0.0f, -1.0f, 1.0f, 0.0f, // bottom-right         
		 1.0f,  1.0f, -1.0f,  0.0f,  0.0f, -1.0f, 1.0f, 1.0f, // top-right
		-1.0f, -1.0f, -1.0f,  0.0f,  0.0f, -1.0f, 0.0f, 0.0f, // bottom-left
		-1.0f,  1.0f, -1.0f,  0.0f,  0.0f, -1.0f, 0.0f, 1.0f, // top-left
		// front face
		-1.0f, -1.0f,  1.0f,  0.0f,  0.0f,  1.0f, 0.0f, 0.0f, // bottom-left
		 1.0f, -1.0f,  1.0f,  0.0f,  0.0f,  1.0f, 1.0f, 0.0f, // bottom-right
		 1.0f,  1.0f,  1.0f,  0.0f,  0.0f,  1.0f, 1.0f, 1.0f, // top-right
		 1.0f,  1.0f,  1.0f,  0.0f,  0.0f,  1.0f, 1.0f, 1.0f, // top-right
		-1.0f,  1.0f,  1.0f,  0.0f,  0.0f,  1.0f, 0.0f, 1.0f, // top-left
		-1.0f, -1.0f,  1.0f,  0.0f,  0.0f,  1.0f, 0.0f, 0.0f, // bottom-left
		// left face
		-1.0f,  1.0f,  1.0f, -1.0f,  0.0f,  0.0f, 1.0f, 0.0f, // top-right
		-1.0f,  1.0f, -1.0f, -1.0f,  0.0f,  0.0f, 1.0f, 1.0f, // top-left
		-1.0f, -1.0f, -1.0f, -1.0f,  0.0f,  0.0f, 0.0f, 1.0f, // bottom-left
		-1.0f, -1.0f, -1.0f, -1.0f,  0.0f,  0.0f, 0.0f, 1.0f, // bottom-left
		-1.0f, -1.0f,  1.0f, -1.0f,  0.0f,  0.0f, 0.0f, 0.0f, // bottom-right
		-1.0f,  1.0f,  1.0f, -1.0f,  0.0f,  0.0f, 1.0f, 0.0f, // top-right
		// right face
		 1.0f,  1.0f,  1.0f,  1.0f,  0.0f,  0.0f, 1.0f, 0.0f, // top-left
		 1.0f, -1.0f, -1.0f,  1.0f,  0.0f,  0.0f, 0.0f, 1.0f, // bottom-right
		 1.0f,  1.0f, -1.0f,  1.0f,  0.0f,  0.0f, 1.0f, 1.0f, // top-right         
		 1.0f, -1.0f, -1.0f,  1.0f,  0.0f,  0.0f, 0.0f, 1.0f, // bottom-right
		 1.0f,  1.0f,  1.0f,  1.0f,  0.0f,  0.0f, 1.0f, 0.0f, // top-left
		 1.0f, -1.0f,  1.0f,  1.0f,  0.0f,  0.0f, 0.0f, 0.0f, // bottom-left     
		// bottom face
		-1.0f, -1.0f, -1.0f,  0.0f, -1.0f,  0.0f, 0.0f, 1.0f, // top-right
		 1.0f, -1.0f, -1.0f,  0.0f, -1.0f,  0.0f, 1.0f, 1.0f, // top-left
		 1.0f, -1.0f,  1.0f,  0.0f, -1.0f,  0.0f, 1.0f, 0.0f, // bottom-left
		 1.0f, -1.0f,  1.0f,  0.0f, -1.0f,  0.0f, 1.0f, 0.0f, // bottom-left
		-1.0f, -1.0f,  1.0f,  0.0f, -1.0f,  0.0f, 0.0f, 0.0f, // bottom-right
		-1.0f, -1.0f, -1.0f,  0.0f, -1.0f,  0.0f, 0.0f, 1.0f, // top-right
		// top face
		-1.0f,  1.0f, -1.0f,  0.0f,  1.0f,  0.0f, 0.0f, 1.0f, // top-left
		 1.0f,  1.0f , 1.0f,  0.0f,  1.0f,  0.0f, 1.0f, 0.0f, // bottom-right
		 1.0f,  1.0f, -1.0f,  0.0f,  1.0f,  0.0f, 1.0f, 1.0f, // top-right     
		 1.0f,  1.0f,  1.0f,  0.0f,  1.0f,  0.0f, 1.0f, 0.0f, // bottom-right
		-1.0f,  1.0f, -1.0f,  0.0f,  1.0f,  0.0f, 0.0f, 1.0f, // top-left
		-1.0f,  1.0f,  1.0f,  0.0f,  1.0f,  0.0f, 0.0f, 0.0f  // bottom-left        
	};
	glGenVertexArrays(1, &cubeVAO);
	glGenBuffers(1, &cubeVBO);
	// fill buffer
	glBindBuffer(GL_ARRAY_BUFFER, cubeVBO);
	glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
	// link vertex attributes
	glBindVertexArray(cubeVAO);
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)0);
	glEnableVertexAttribArray(1);
	glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(3 * sizeof(float)));
	glEnableVertexAttribArray(2);
	glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(6 * sizeof(float)));
	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindVertexArray(0);

	// SHADERS
	ShaderProgram shaderGeometryPass("../../Phoenix/RendererOpenGL/Shaders/g_buffer.vert", "../../Phoenix/RendererOpenGL/Shaders/g_buffer.frag");
	ShaderProgram shaderLightingPass("../../Phoenix/RendererOpenGL/Shaders/deferred_shading.vert", "../../Phoenix/RendererOpenGL/Shaders/deferred_shading.frag");
	ShaderProgram shaderLightBox("../../Phoenix/RendererOpenGL/Shaders/deferred_light_box.vert", "../../Phoenix/RendererOpenGL/Shaders/deferred_light_box.frag");

	// load models
	// -----------
	Model nanosuit("../../Phoenix/RendererOpenGL/Objects/nanosuit/nanosuit.obj");
	tinystl::vector<glm::vec3> objectPositions;
	objectPositions.push_back(glm::vec3(-3.0, -3.0, -3.0));
	objectPositions.push_back(glm::vec3(0.0, -3.0, -3.0));
	objectPositions.push_back(glm::vec3(3.0, -3.0, -3.0));
	objectPositions.push_back(glm::vec3(-3.0, -3.0, 0.0));
	objectPositions.push_back(glm::vec3(0.0, -3.0, 0.0));
	objectPositions.push_back(glm::vec3(3.0, -3.0, 0.0));
	objectPositions.push_back(glm::vec3(-3.0, -3.0, 3.0));
	objectPositions.push_back(glm::vec3(0.0, -3.0, 3.0));
	objectPositions.push_back(glm::vec3(3.0, -3.0, 3.0));

	// configure g-buffer framebuffer
	// ------------------------------
	unsigned int gBuffer;
	glGenFramebuffers(1, &gBuffer);
	glBindFramebuffer(GL_FRAMEBUFFER, gBuffer);
	unsigned int gPosition, gNormal, gAlbedoSpec;
	// position color buffer
	glGenTextures(1, &gPosition);
	glBindTexture(GL_TEXTURE_2D, gPosition);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB16F, window.windowWidth(), window.windowHeight(), 0, GL_RGB, GL_FLOAT, NULL);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, gPosition, 0);
	// normal color buffer
	glGenTextures(1, &gNormal);
	glBindTexture(GL_TEXTURE_2D, gNormal);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB16F, window.windowWidth(), window.windowHeight(), 0, GL_RGB, GL_FLOAT, NULL);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT1, GL_TEXTURE_2D, gNormal, 0);
	// color + specular color buffer
	glGenTextures(1, &gAlbedoSpec);
	glBindTexture(GL_TEXTURE_2D, gAlbedoSpec);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, window.windowWidth(), window.windowHeight(), 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT2, GL_TEXTURE_2D, gAlbedoSpec, 0);
	// tell OpenGL which color attachments we'll use (of this framebuffer) for rendering 
	unsigned int attachments[3] = { GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1, GL_COLOR_ATTACHMENT2 };
	glDrawBuffers(3, attachments);
	// create and attach depth buffer (renderbuffer)
	unsigned int rboDepth;
	glGenRenderbuffers(1, &rboDepth);
	glBindRenderbuffer(GL_RENDERBUFFER, rboDepth);
	glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT, window.windowWidth(), window.windowHeight());
	glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, rboDepth);
	// finally check if framebuffer is complete
	if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
		std::cout << "Framebuffer not complete!" << std::endl;
	glBindFramebuffer(GL_FRAMEBUFFER, 0);

	// lighting info
	// -------------
	const unsigned int NR_LIGHTS = 32;
	tinystl::vector<glm::vec3> lightPositions;
	tinystl::vector<glm::vec3> lightColors;
	srand(13);
	for (unsigned int i = 0; i < NR_LIGHTS; i++)
	{
		// calculate slightly random offsets
		float xPos = (float)( ((rand() % 100) / 100.0f) * 6.0f - 3.0f );
		float yPos = (float)( ((rand() % 100) / 100.0f) * 6.0f - 4.0f );
		float zPos = (float)( ((rand() % 100) / 100.0f) * 6.0f - 3.0f );
		lightPositions.push_back(glm::vec3(xPos, yPos, zPos));
		// also calculate random color
		float rColor = (float)( ((rand() % 100) / 200.0f) + 0.5f ); // between 0.5 and 1.0
		float gColor = (float)( ((rand() % 100) / 200.0f) + 0.5f ); // between 0.5 and 1.0
		float bColor = (float)( ((rand() % 100) / 200.0f) + 0.5f ); // between 0.5 and 1.0
		lightColors.push_back(glm::vec3(rColor, gColor, bColor));
	}

	// shader configuration
	// --------------------
	int zero = 0, one = 1, two = 2;
	glUseProgram(shaderLightingPass.mId);
	shaderLightingPass.SetUniform("gPosition", &zero);
	shaderLightingPass.SetUniform("gNormal", &one);
	shaderLightingPass.SetUniform("gAlbedoSpec", &two);

	window.initGui();

	while (!window.windowShouldClose() && !exitOnESC)
	{
		window.startFrame();

		glClearColor(0.5f, 0.5f, 0.5f, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		glm::mat4 projection = glm::perspective(glm::radians(camera.Zoom), (float)window.windowWidth() / (float)window.windowHeight(), 0.1f, 100.0f);
		glm::mat4 view = camera.GetViewMatrix();
		glm::mat4 model = glm::mat4(1.0f);

		// 1. geometry pass: render scene's geometry/color data into gbuffer
		// -----------------------------------------------------------------
		glBindFramebuffer(GL_FRAMEBUFFER, gBuffer);
			glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
			glUseProgram(shaderGeometryPass.mId);
			shaderGeometryPass.SetUniform("projection", &projection);
			shaderGeometryPass.SetUniform("view", &view);
			for (unsigned int i = 0; i < objectPositions.size(); i++)
			{
				model = glm::mat4(1.0f);
				model = glm::translate(model, objectPositions[i]);
				model = glm::scale(model, glm::vec3(0.25f));
				shaderGeometryPass.SetUniform("model", &model);
				nanosuit.Draw(shaderGeometryPass);
			}
		glBindFramebuffer(GL_FRAMEBUFFER, 0);

		// 2. lighting pass: calculate lighting by iterating over a screen filled quad pixel-by-pixel using the gbuffer's content.
		// -----------------------------------------------------------------------------------------------------------------------
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		glUseProgram(shaderLightingPass.mId);
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, gPosition);
		glActiveTexture(GL_TEXTURE1);
		glBindTexture(GL_TEXTURE_2D, gNormal);
		glActiveTexture(GL_TEXTURE2);
		glBindTexture(GL_TEXTURE_2D, gAlbedoSpec);
		// send light relevant uniforms
		for (unsigned int i = 0; i < lightPositions.size(); i++)
		{
			shaderLightingPass.SetUniform( ("lights[" + std::to_string(i) + "].Position").c_str(), &(lightPositions[i]) );
			shaderLightingPass.SetUniform( ("lights[" + std::to_string(i) + "].Color").c_str(), &(lightColors[i]) );
			// update attenuation parameters and calculate radius
			float constant = 1.0f; // note that we don't send this to the shader, we assume it is always 1.0 (in our case)
			float linear = 0.7f;
			float quadratic = 1.8f;
			shaderLightingPass.SetUniform( ("lights[" + std::to_string(i) + "].Linear").c_str(), &linear );
			shaderLightingPass.SetUniform( ("lights[" + std::to_string(i) + "].Quadratic").c_str(), &quadratic );
		}
		shaderLightingPass.SetUniform("viewPos", &(camera.Position));
		// finally render quad
		renderQuad();

		// 2.5. copy content of geometry's depth buffer to default framebuffer's depth buffer
		// ----------------------------------------------------------------------------------
		glBindFramebuffer(GL_READ_FRAMEBUFFER, gBuffer);
		glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0); // write to default framebuffer
		// blit to default framebuffer. Note that this may or may not work as the internal formats of both the FBO and default framebuffer have to match.
		// the internal formats are implementation defined. This works on all of my systems, but if it doesn't on yours you'll likely have to write to the 		
		// depth buffer in another shader stage (or somehow see to match the default framebuffer's internal format with the FBO's internal format).
		glBlitFramebuffer(0, 0, window.windowWidth(), window.windowHeight(), 0, 0, window.windowWidth(), window.windowHeight(), GL_DEPTH_BUFFER_BIT, GL_NEAREST);
		glBindFramebuffer(GL_FRAMEBUFFER, 0);

		// 3. render lights on top of scene
		// --------------------------------
		glUseProgram(shaderLightBox.mId);
		shaderLightBox.SetUniform("projection", &projection);
		shaderLightBox.SetUniform("view", &view);
		for (unsigned int i = 0; i < lightPositions.size(); i++)
		{
			model = glm::mat4(1.0f);
			model = glm::translate(model, lightPositions[i]);
			model = glm::scale(model, glm::vec3(0.125f));
			shaderLightBox.SetUniform("model", &model);
			shaderLightBox.SetUniform("lightColor", &lightColors[i]);
			renderCube();
		}

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

		window.endFrame();
	}

	window.exitGui();

	window.exitWindow();

	return 0;
}