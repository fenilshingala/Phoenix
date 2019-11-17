#include "../Picker.h"

#if PBR

#include "../Common.h"

const int NR_LIGHTS = 100;
struct LightBlock
{
	glm::vec3 Position;
	float pad0;
	glm::vec3 Color;
	float pad1;
};

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

	ShaderProgram pbrShader("../../Phoenix/RendererOpenGL/App/Resources/Shaders/pbr.vert",
							"../../Phoenix/RendererOpenGL/App/Resources/Shaders/pbr.frag");

	int zero = 0, one = 1, two = 2, three = 3, four = 4;
	glUseProgram(pbrShader.mId);
	pbrShader.SetUniform("albedoMap", &zero);
	pbrShader.SetUniform("normalMap", &one);
	pbrShader.SetUniform("metallicMap", &two);
	pbrShader.SetUniform("roughnessMap", &three);
	pbrShader.SetUniform("aoMap", &four);

	ShaderProgram meshShader("../../Phoenix/RendererOpenGL/App/Resources/Shaders/mesh.vert",
							 "../../Phoenix/RendererOpenGL/App/Resources/Shaders/mesh.frag");

	PBRMat_Tex rustediron;
	rustediron.LoadPBRTexture("../../Phoenix/RendererOpenGL/App/Resources/PBR/rustediron1-alt2-Unreal-Engine/albedo.png", ALBEDO);
	rustediron.LoadPBRTexture("../../Phoenix/RendererOpenGL/App/Resources/PBR/rustediron1-alt2-Unreal-Engine/normal.png", NORMAL);
	rustediron.LoadPBRTexture("../../Phoenix/RendererOpenGL/App/Resources/PBR/rustediron1-alt2-Unreal-Engine/metallic.png", METALLIC);
	rustediron.LoadPBRTexture("../../Phoenix/RendererOpenGL/App/Resources/PBR/rustediron1-alt2-Unreal-Engine/roughness.png", ROUGHNESS);
	rustediron.LoadPBRTexture("../../Phoenix/RendererOpenGL/App/Resources/PBR/rustediron1-alt2-Unreal-Engine/ao.jpg", AO);

	PBRMat_Tex military_panel;
	military_panel.LoadPBRTexture("../../Phoenix/RendererOpenGL/App/Resources/PBR/military-panel1-ue/albedo.png", ALBEDO);
	military_panel.LoadPBRTexture("../../Phoenix/RendererOpenGL/App/Resources/PBR/military-panel1-ue/normal.png", NORMAL);
	military_panel.LoadPBRTexture("../../Phoenix/RendererOpenGL/App/Resources/PBR/military-panel1-ue/metallic.png", METALLIC);
	military_panel.LoadPBRTexture("../../Phoenix/RendererOpenGL/App/Resources/PBR/military-panel1-ue/roughness.png", ROUGHNESS);
	military_panel.LoadPBRTexture("../../Phoenix/RendererOpenGL/App/Resources/PBR/military-panel1-ue/ao.png", AO);

	PBRMat_Tex streaked_metal;
	streaked_metal.LoadPBRTexture("../../Phoenix/RendererOpenGL/App/Resources/PBR/streaked-metal1-ue/albedo.png", ALBEDO);
	streaked_metal.LoadPBRTexture("../../Phoenix/RendererOpenGL/App/Resources/PBR/streaked-metal1-ue/normal.png", NORMAL);
	streaked_metal.LoadPBRTexture("../../Phoenix/RendererOpenGL/App/Resources/PBR/streaked-metal1-ue/metallic.png", METALLIC);
	streaked_metal.LoadPBRTexture("../../Phoenix/RendererOpenGL/App/Resources/PBR/streaked-metal1-ue/roughness.png", ROUGHNESS);
	streaked_metal.LoadPBRTexture("../../Phoenix/RendererOpenGL/App/Resources/PBR/streaked-metal1-ue/ao.png", AO);

	PBRMat_Tex metalgrid;
	metalgrid.LoadPBRTexture("../../Phoenix/RendererOpenGL/App/Resources/PBR/metalgrid4-ue/albedo.png", ALBEDO);
	metalgrid.LoadPBRTexture("../../Phoenix/RendererOpenGL/App/Resources/PBR/metalgrid4-ue/normal.png", NORMAL);
	metalgrid.LoadPBRTexture("../../Phoenix/RendererOpenGL/App/Resources/PBR/metalgrid4-ue/metallic.png", METALLIC);
	metalgrid.LoadPBRTexture("../../Phoenix/RendererOpenGL/App/Resources/PBR/metalgrid4-ue/roughness.png", ROUGHNESS);
	metalgrid.LoadPBRTexture("../../Phoenix/RendererOpenGL/App/Resources/PBR/metalgrid4-ue/ao.png", AO);

	PBRMat_Tex titanium;
	titanium.LoadPBRTexture("../../Phoenix/RendererOpenGL/App/Resources/PBR/Titanium-Scuffed-Unreal-Engine/albedo.png", ALBEDO);
	titanium.LoadPBRTexture("../../Phoenix/RendererOpenGL/App/Resources/PBR/Titanium-Scuffed-Unreal-Engine/normal.png", NORMAL);
	titanium.LoadPBRTexture("../../Phoenix/RendererOpenGL/App/Resources/PBR/Titanium-Scuffed-Unreal-Engine/metallic.png", METALLIC);
	titanium.LoadPBRTexture("../../Phoenix/RendererOpenGL/App/Resources/PBR/Titanium-Scuffed-Unreal-Engine/roughness.png", ROUGHNESS);
	titanium.LoadPBRTexture("../../Phoenix/RendererOpenGL/App/Resources/PBR/Titanium-Scuffed-Unreal-Engine/ao.jpg", AO);

	std::vector<LightBlock> lights;
	lights.push_back({glm::vec3(-3.0f, 0.0f, 5.0f),		 0.0f,
					  glm::vec3(150.0f, 150.0f, 150.0f), 0.0f});

	lights.push_back({ glm::vec3(3.0f, 0.0f, 5.0f),		  0.0f,
					   glm::vec3(150.0f, 150.0f, 150.0f), 0.0f});
	
	// lights uniform buffer block
	int64_t lightBlockSize = sizeof(LightBlock);
	unsigned int uboLightsBlock;
	glGenBuffers(1, &uboLightsBlock);
	glBindBuffer(GL_UNIFORM_BUFFER, uboLightsBlock);
	glBufferData(GL_UNIFORM_BUFFER, lightBlockSize * NR_LIGHTS, NULL, GL_STATIC_DRAW);
	glBindBuffer(GL_UNIFORM_BUFFER, 0);
	glBindBufferRange(GL_UNIFORM_BUFFER, 0, uboLightsBlock, 0, lightBlockSize * NR_LIGHTS);

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

			glUseProgram(pbrShader.mId);
			pbrShader.SetUniform("projection", &projection);
			pbrShader.SetUniform("view", &view);
			pbrShader.SetUniform("camPos", &camera.Position);
			

			/*glm::vec3 color(150.0f, 150.0f, 150.0f);
			glUniform3fv(glGetUniformLocation(pbrShader.mId, "lightPositions[0]"), 1, (GLfloat*)&lightPos[0]);
			glUniform3fv(glGetUniformLocation(pbrShader.mId, "lightColors[0]"), 1, (GLfloat*)&color[0]);*/

			glBindBuffer(GL_UNIFORM_BUFFER, uboLightsBlock);
			glBufferSubData(GL_UNIFORM_BUFFER, 0, lightBlockSize * lights.size(), lights.data());
			glBindBuffer(GL_UNIFORM_BUFFER, 0);

			int activeLights = (int)lights.size();
			pbrShader.SetUniform("activeLights", &activeLights);
			
			// titanium
			glm::mat4 model = glm::mat4(1.0f);
			model = glm::translate(model, glm::vec3(-6.0f, 0.0f, 0.0f));
			pbrShader.SetUniform("model", &model);
			titanium.BindTextures();
			pOpenGLRenderer->RenderSphere();

			// streaked_metal
			model = glm::mat4(1.0f);
			model = glm::translate(model, glm::vec3(-3.0f, 0.0f, 0.0f));
			pbrShader.SetUniform("model", &model);
			streaked_metal.BindTextures();
			pOpenGLRenderer->RenderSphere();

			// rustediron
			model = glm::mat4(1.0f);
			pbrShader.SetUniform("model", &model);
			rustediron.BindTextures();
			pOpenGLRenderer->RenderSphere();

			// metalgrid
			model = glm::mat4(1.0f);
			model = glm::translate(model, glm::vec3(3.0f, 0.0f, 0.0f));
			pbrShader.SetUniform("model", &model);
			metalgrid.BindTextures();
			pOpenGLRenderer->RenderSphere();

			// military_panel
			model = glm::mat4(1.0f);
			model = glm::translate(model, glm::vec3(6.0f, 0.0f, 0.0f));
			pbrShader.SetUniform("model", &model);
			military_panel.BindTextures();
			pOpenGLRenderer->RenderSphere();

			// plane

			// representation of light
			glUseProgram(meshShader.mId);
			meshShader.SetUniform("projection", &projection);
			meshShader.SetUniform("view", &view);
			for (int k = 0; k < activeLights; ++k)
			{
				model = glm::mat4(1.0f);
				model = glm::translate(model, glm::vec3(lights[k].Position));
				model = glm::scale(model, glm::vec3(0.1f));
				meshShader.SetUniform("model", &model);
				meshShader.SetUniform("color", &lights[k].Color);
				pOpenGLRenderer->RenderSphere();
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

				ImGui::Begin("Lights");
				int num = 1;
				for (num; num <= activeLights; ++num)
				{
					ImGui::InputFloat(("Light" + std::to_string(num) + " PosX").c_str(), &lights[num - 1].Position.x, 1.0f, 0.0f, 3);
					ImGui::InputFloat(("Light" + std::to_string(num) + " PosY").c_str(), &lights[num - 1].Position.y, 1.0f, 0.0f, 3);
					ImGui::InputFloat(("Light" + std::to_string(num) + " PosZ").c_str(), &lights[num - 1].Position.z, 1.0f, 0.0f, 3);
				}
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