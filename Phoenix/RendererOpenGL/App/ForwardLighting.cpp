#include "../Picker.h"

#if FORWARD_LIGHTING

#include "../Common.h"

// lighting
glm::vec3 lightPos(-10.0f, 8.0f, -10.0f);

OpenGLRenderer* pOpenGLRenderer = NULL;

void RenderScene(ShaderProgram& shader)
{
	glm::mat4 model = glm::mat4(1.0f);
	model = glm::scale(model, glm::vec3(10.0f, 0.5f, 10.0f));
	shader.SetUniform("model", &model);

	pOpenGLRenderer->RenderCube();

	model = glm::mat4(1.0f);
	model = glm::translate(model, glm::vec3(0.0f, 2.0f, 0.0f));
	shader.SetUniform("model", &model);

	pOpenGLRenderer->RenderCube();
}

void Run()
{
	window.initWindow();
	lastX = window.windowWidth()  / 2.0f;
	lastY = window.windowHeight() / 2.0f;

	camera.Position = glm::vec3(0.0f, 2.0f, 10.0f);

	pOpenGLRenderer = new OpenGLRenderer();

	glEnable(GL_DEPTH_TEST);
	glEnable(GL_MULTISAMPLE);

	ShaderProgram lightingShader("../../Phoenix/RendererOpenGL/App/Resources/Shaders/basic_lighting.vert",
								 "../../Phoenix/RendererOpenGL/App/Resources/Shaders/basic_lighting.frag");
	ShaderProgram lampShader("../../Phoenix/RendererOpenGL/App/Resources/Shaders/mesh.vert",
							 "../../Phoenix/RendererOpenGL/App/Resources/Shaders/mesh.frag");

	ShaderProgram simpleDepthShader("../../Phoenix/RendererOpenGL/App/Resources/Shaders/shadow_depth.vert",
								    "../../Phoenix/RendererOpenGL/App/Resources/Shaders/shadow_depth.frag");

	ShaderProgram debugDepthQuad("../../Phoenix/RendererOpenGL/App/Resources/Shaders/shadow_debug.vert",
								 "../../Phoenix/RendererOpenGL/App/Resources/Shaders/shadow_debug.frag");
	// configure depth map FBO
	// -----------------------
	const unsigned int SHADOW_WIDTH = window.windowWidth(), SHADOW_HEIGHT = window.windowHeight();
	unsigned int depthMapFBO;
	glGenFramebuffers(1, &depthMapFBO);
	// create depth texture
	unsigned int depthMap;
	glGenTextures(1, &depthMap);
	glBindTexture(GL_TEXTURE_2D, depthMap);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, SHADOW_WIDTH, SHADOW_HEIGHT, 0, GL_RGBA, GL_FLOAT, NULL);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
	// attach depth texture as FBO's depth buffer
	glBindFramebuffer(GL_FRAMEBUFFER, depthMapFBO);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, depthMap, 0);
	unsigned int attachment[1] = { GL_COLOR_ATTACHMENT0 };
	glDrawBuffers(1, attachment);
	//glReadBuffer(GL_NONE);
	glBindFramebuffer(GL_FRAMEBUFFER, 0);


	// shader configuration
	// --------------------
	int zero = 0;
	glUseProgram(lightingShader.mId);
	lightingShader.SetUniform("shadowMap", &zero);
	glUseProgram(debugDepthQuad.mId);
	debugDepthQuad.SetUniform("depthMap", &zero);

	window.initGui();

	while (!window.windowShouldClose() && !exitOnESC)
	{
		window.startFrame();

		{
			float near_plane = 0.1f, far_plane = 100.0f;
		
			glm::mat4 projection = glm::perspective(glm::radians(camera.Zoom), (float)window.windowWidth() / (float)window.windowHeight(), near_plane, far_plane);
			glm::mat4 view = camera.GetViewMatrix();

			glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
			glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		
			// 1. render depth of scene to texture (from light's perspective)
			// --------------------------------------------------------------
			glm::mat4 lightProjection, lightView;
			glm::mat4 lightSpaceMatrix;
			lightProjection  = glm::ortho(-10.0f, 10.0f, -10.0f, 10.0f, near_plane, far_plane);
			lightView	     = glm::lookAt(lightPos, glm::vec3(0.0f), glm::vec3(0.0, 1.0, 0.0));
			lightSpaceMatrix = lightProjection * lightView;
			
			// render scene from light's point of view
			glUseProgram(simpleDepthShader.mId);
			simpleDepthShader.SetUniform("lightSpaceMatrix", &lightSpaceMatrix);

			glViewport(0, 0, SHADOW_WIDTH, SHADOW_HEIGHT);

			glBindFramebuffer(GL_FRAMEBUFFER, depthMapFBO);
			
			glClear(GL_DEPTH_BUFFER_BIT);
			glCullFace(GL_FRONT);
			RenderScene(simpleDepthShader);
			glCullFace(GL_BACK);

			glBindFramebuffer(GL_FRAMEBUFFER, 0);
			

			// reset viewport
			glViewport(0, 0, window.windowWidth(), window.windowHeight());
			glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

			// draw our first triangle
			glUseProgram(lightingShader.mId);

			glm::vec3 objectColor = glm::vec3(1.0f, 0.5f, 0.31f);
			lightingShader.SetUniform("objectColor", &objectColor);
			glm::vec3 lightColor = glm::vec3(1.0f, 1.0f, 1.0f);
			lightingShader.SetUniform("lightColor", &lightColor);
			lightingShader.SetUniform("lightPos", &lightPos);
			lightingShader.SetUniform("viewPos", &camera.Position);
			lightingShader.SetUniform("lightSpaceMatrix", &lightSpaceMatrix);

			lightingShader.SetUniform("projection", &projection);
			lightingShader.SetUniform("view", &view);

			glActiveTexture(GL_TEXTURE0);
			glBindTexture(GL_TEXTURE_2D, depthMap);
			RenderScene(lightingShader);

			// also draw the lamp object
			glUseProgram(lampShader.mId);

			lampShader.SetUniform("projection", &projection);
			lampShader.SetUniform("view", &view);
			glm::mat4 model = glm::mat4(1.0f);
			model = glm::translate(model, lightPos);
			model = glm::scale(model, glm::vec3(0.2f)); // a smaller cube
			lampShader.SetUniform("model", &model);

			pOpenGLRenderer->RenderCube();

			// render Depth map to quad for visual debugging
			// ---------------------------------------------
			
			int xOffset = window.windowWidth()  - window.windowWidth() / 6  - 10;
			int yOffset = window.windowHeight() - window.windowHeight() / 6 - 10;
			glViewport(xOffset, yOffset, window.windowWidth()/6, window.windowHeight()/6);

			glUseProgram(debugDepthQuad.mId);
			debugDepthQuad.SetUniform("near_plane", &near_plane);
			debugDepthQuad.SetUniform("far_plane", &far_plane);
			glActiveTexture(GL_TEXTURE0);
			glBindTexture(GL_TEXTURE_2D, depthMap);
			pOpenGLRenderer->RenderQuad();

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

	/*glDeleteVertexArrays(1, &cubeVAO);
	glDeleteVertexArrays(1, &lightVAO);
	glDeleteBuffers(1, &VBO);*/

	window.exitWindow();
}

#endif