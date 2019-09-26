#include "Picker.h"

#if DEFFERED_SHADING

/////////////////////////////////////////////////////////////////////////////////////////
// --------------------------------------------------------------------------------------
#include "Common.h"
#include <iostream>

struct LightBlock
{
	glm::vec3 lightPosition;
	float pad0;
	glm::vec3 lightColor;
	float pad1;

	float Linear;
	float Quadratic;
	float pad2;
	float pad3;
};

const unsigned int NR_LIGHTS = 200;

// renderCube() renders a 1x1 3D cube in NDC.
// -------------------------------------------------
unsigned int cubeVAO = 0;
unsigned int cubeVBO = 0;
void renderCube()
{
	glBindVertexArray(cubeVAO);
	//glDrawArrays(GL_TRIANGLES, 0, 36);
	glDrawArraysInstanced(GL_TRIANGLES, 0, 36, NR_LIGHTS);
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

void Run()
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
	struct instancedData
	{
		glm::mat4 lightModel;
		glm::vec3 lightColor;
	};
	instancedData data[NR_LIGHTS];

	srand(13);
	for (int i = 0; i < NR_LIGHTS; ++i)
	{
		data[i].lightModel = glm::mat4(1.0f);
		float xPos = (float)(((rand() % 200) / 100.0f) * 6.0f - 3.0f);
		float yPos = (float)(((rand() % 200) / 100.0f) * 6.0f - 4.0f);
		float zPos = (float)(((rand() % 200) / 100.0f) * 6.0f - 3.0f);
		data[i].lightModel = glm::translate(data[i].lightModel, glm::vec3(xPos, yPos, zPos));
		data[i].lightModel = glm::scale(data[i].lightModel, glm::vec3(0.125f));

		// color
		float rColor = (float)(((rand() % 100) / 200.0f) + 0.5f); // between 0.5 and 1.0
		float gColor = (float)(((rand() % 100) / 200.0f) + 0.5f); // between 0.5 and 1.0
		float bColor = (float)(((rand() % 100) / 200.0f) + 0.5f); // between 0.5 and 1.0
		data[i].lightColor = glm::vec3(rColor, gColor, bColor);
	}
	
	unsigned int instanceVBO;
	glGenBuffers(1, &instanceVBO);
	glBindBuffer(GL_ARRAY_BUFFER, instanceVBO);
	glBufferData(GL_ARRAY_BUFFER, NR_LIGHTS * sizeof(instancedData), &data, GL_STATIC_DRAW);
	glBindBuffer(GL_ARRAY_BUFFER, 0);

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
	{
		glEnableVertexAttribArray(0);
		glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)0);
		glEnableVertexAttribArray(1);
		glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(3 * sizeof(float)));
		glEnableVertexAttribArray(2);
		glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(6 * sizeof(float)));

		{
			glBindBuffer(GL_ARRAY_BUFFER, instanceVBO); // this attribute comes from a different vertex buffer

			glEnableVertexAttribArray(3);
			glVertexAttribPointer(3, 4, GL_FLOAT, GL_FALSE, 19 * sizeof(float), (void*)0);
			glVertexAttribDivisor(3, 1); // tell OpenGL this is an instanced vertex attribute.
			
			glEnableVertexAttribArray(4);
			glVertexAttribPointer(4, 4, GL_FLOAT, GL_FALSE, 19 * sizeof(float), (void*)(4 * sizeof(float)));
			glVertexAttribDivisor(4, 1); // tell OpenGL this is an instanced vertex attribute.

			glEnableVertexAttribArray(5);
			glVertexAttribPointer(5, 4, GL_FLOAT, GL_FALSE, 19 * sizeof(float), (void*)(8 * sizeof(float)));
			glVertexAttribDivisor(5, 1); // tell OpenGL this is an instanced vertex attribute.

			glEnableVertexAttribArray(6);
			glVertexAttribPointer(6, 4, GL_FLOAT, GL_FALSE, 19 * sizeof(float), (void*)(12 * sizeof(float)));
			glVertexAttribDivisor(6, 1); // tell OpenGL this is an instanced vertex attribute.

			glEnableVertexAttribArray(7);
			glVertexAttribPointer(7, 3, GL_FLOAT, GL_FALSE, 19 * sizeof(float), (void*)(16 * sizeof(float)));
			glVertexAttribDivisor(7, 1); // tell OpenGL this is an instanced vertex attribute.

			glBindBuffer(GL_ARRAY_BUFFER, 0);
		}
	}
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
	tinystl::vector<LightBlock> lights(NR_LIGHTS);
	srand(13);
	for (unsigned int i = 0; i < NR_LIGHTS; i++)
	{
		// calculate slightly random offsets
		float xPos = (float)(((rand() % 200) / 100.0f) * 6.0f - 3.0f);
		float yPos = (float)(((rand() % 200) / 100.0f) * 6.0f - 4.0f);
		float zPos = (float)(((rand() % 200) / 100.0f) * 6.0f - 3.0f);
		lights[i].lightPosition = glm::vec3(xPos, yPos, zPos);
		// also calculate random color
		float rColor = (float)(((rand() % 100) / 200.0f) + 0.5f); // between 0.5 and 1.0
		float gColor = (float)(((rand() % 100) / 200.0f) + 0.5f); // between 0.5 and 1.0
		float bColor = (float)(((rand() % 100) / 200.0f) + 0.5f); // between 0.5 and 1.0
		lights[i].lightColor = glm::vec3(rColor, gColor, bColor);
		
		lights[i].Linear	= 0.0f;
		lights[i].Quadratic = 3.0f;
		lights[i].pad0 = lights[i].pad1 = lights[i].pad2 = lights[i].pad3 = 0.0f;
	}

	// lights uniform buffer block
	int64_t lightBlockSize = sizeof(LightBlock);
	unsigned int uboLightsBlock;
	glGenBuffers(1, &uboLightsBlock);
	glBindBuffer(GL_UNIFORM_BUFFER, uboLightsBlock);
	glBufferData(GL_UNIFORM_BUFFER, lightBlockSize * NR_LIGHTS, NULL, GL_STATIC_DRAW);
	glBindBuffer(GL_UNIFORM_BUFFER, 0);
	// define the range of the buffer that links to a uniform binding point
	glBindBufferRange(GL_UNIFORM_BUFFER, 0, uboLightsBlock, 0, lightBlockSize * NR_LIGHTS);

	// shader configuration
	// --------------------
	int zero = 0, one = 1, two = 2;
	glUseProgram(shaderLightingPass.mId);
	shaderLightingPass.SetUniform("gPosition", &zero);
	shaderLightingPass.SetUniform("gNormal", &one);
	shaderLightingPass.SetUniform("gAlbedoSpec", &two);

	window.initGui();

	float constant = 1.0f; // note that we don't send this to the shader, we assume it is always 1.0 (in our case)
	float linear = 0.0f, prevLinear = 0.7f;
	float quadratic = 3.0f, prevQuadratic = 1.8f;

	while (!window.windowShouldClose() && !exitOnESC)
	{
		window.startFrame();

		window.beginGuiFrame();
		bool truebool = true;

		// GUI - LIGHTS
		ImGui::Begin("LIGHT SETTINGS!", &truebool);

		ImGui::SliderFloat("linear", &linear, 0.0f, 2.0f);
		ImGui::SliderFloat("quadratic", &quadratic, 0.0f, 3.0f);

		ImGui::End();

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
		{
			glBindBuffer(GL_UNIFORM_BUFFER, uboLightsBlock);
			
			/*if (linear != prevLinear || quadratic != prevQuadratic)
			{
				for (uint64_t i = 0; i < lights.size(); ++i)
				{
					lights[i].Linear	= linear;
					lights[i].Quadratic = quadratic;
				}
				prevLinear	  = linear;
				prevQuadratic = quadratic;
			}*/
			glBufferSubData(GL_UNIFORM_BUFFER, 0, lightBlockSize * NR_LIGHTS, lights.data());
			glBindBuffer(GL_UNIFORM_BUFFER, 0);
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
		//for (unsigned int i = 0; i < lights.size(); i++)
		//{
			//model = glm::mat4(1.0f);
			//model = glm::translate(model, lights[i].lightPosition);
			//model = glm::scale(model, glm::vec3(0.125f));
			//shaderLightBox.SetUniform("model", &model);
			//shaderLightBox.SetUniform("lightColor", &lights[i].lightColor);
			renderCube();
		//}


		// GUI - FPS COUNTERS
		str = "controlled fps: " + std::to_string(window.frameRate());
		ImGui::Begin("BLEH!", &truebool);

		ImGui::Text(str.c_str());
		str = "actual fps: " + std::to_string(window.actualFrameRate());
		ImGui::Text(str.c_str());

		ImGui::End();

		window.endGuiFrame();

		window.swapWindow();

		window.update();

		processInputs();

		window.endFrame();
	}

	window.exitGui();

	window.exitWindow();
}
// --------------------------------------------------------------------------------------
/////////////////////////////////////////////////////////////////////////////////////////

#endif