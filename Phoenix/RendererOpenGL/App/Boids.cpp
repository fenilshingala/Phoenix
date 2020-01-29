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

	camera.Position = glm::vec3(0.0f, 2.0f, 10.0f);

	pOpenGLRenderer = new OpenGLRenderer();

	glEnable(GL_DEPTH_TEST);
	glEnable(GL_MULTISAMPLE);

	ShaderProgram meshShader("../../Phoenix/RendererOpenGL/App/Resources/Shaders/mesh.vert",
							 "../../Phoenix/RendererOpenGL/App/Resources/Shaders/mesh.frag");
	
	window.initGui();

	while (!window.windowShouldClose() && !exitOnESC)
	{
		window.startFrame();

		{
			
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