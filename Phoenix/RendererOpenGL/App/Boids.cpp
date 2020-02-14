#include "../Picker.h"

#if BOIDS

#include "../Common.h"

// lighting
glm::vec3 lightPos(-10.0f, 8.0f, -10.0f);

OpenGLRenderer* pOpenGLRenderer = NULL;

void Run()
{
	window.initWindow();
	lastX = window.windowWidth() / 2.0f;
	lastY = window.windowHeight() / 2.0f;

	camera.Position = glm::vec3(0.0f, 0.0f, 10.0f);

	pOpenGLRenderer = new OpenGLRenderer();

	glEnable(GL_DEPTH_TEST);
	glEnable(GL_MULTISAMPLE);

	ShaderProgram meshShader("../../Phoenix/RendererOpenGL/App/Resources/Shaders/mesh.vert",
							 "../../Phoenix/RendererOpenGL/App/Resources/Shaders/mesh.frag");
	
	window.initGui();
	float near_plane = 0.1f, far_plane = 100.0f;
	int one = 1;

	while (!window.windowShouldClose() && !exitOnESC)
	{
		window.startFrame();

		{
			glClearColor(0.5f, 0.5f, 0.5f, 1.0f);
			glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

			glm::mat4 projection = glm::perspective(glm::radians(camera.Zoom), (float)window.windowWidth() / (float)window.windowHeight(), near_plane, far_plane);
			glm::mat4 view = camera.GetViewMatrix();
			glUseProgram(meshShader.mId);
			meshShader.SetUniform("projection", &projection);
			meshShader.SetUniform("view", &view);
			meshShader.SetUniform("instanced", &one);

			InstanceData data[2];
			data[0].color = glm::vec3(1.0f, 0.0f, 0.0f);
			data[0].model = glm::mat4(0.5f);
			data[0].model = glm::translate(data[0].model, glm::vec3(-5.0f, 0.0f, 0.0f));
			data[1].color = glm::vec3(0.0f, 1.0f, 0.0f);
			data[1].model = glm::mat4(0.5f);
			data[1].model = glm::translate(data[0].model, glm::vec3(5.0f, 0.0f, 0.0f));

			pOpenGLRenderer->UpdateQuadInstanceBuffer(2*sizeof(InstanceData), data);
			pOpenGLRenderer->RenderQuadInstanced(2);

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

	window.exitWindow();
}

#endif