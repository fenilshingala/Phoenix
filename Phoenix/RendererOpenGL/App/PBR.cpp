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
	glDepthFunc(GL_LEQUAL);
	glEnable(GL_TEXTURE_CUBE_MAP_SEAMLESS);
	glEnable(GL_MULTISAMPLE);

	ShaderProgram pbrShader("../../Phoenix/RendererOpenGL/App/Resources/Shaders/pbr.vert",
							"../../Phoenix/RendererOpenGL/App/Resources/Shaders/pbr.frag");
	
	int zero = 0, one = 1, two = 2, three = 3, four = 4, five = 5, six = 6, seven = 7;
	glUseProgram(pbrShader.mId);
	pbrShader.SetUniform("albedoMap", &zero);
	pbrShader.SetUniform("normalMap", &one);
	pbrShader.SetUniform("metallicMap", &two);
	pbrShader.SetUniform("roughnessMap", &three);
	pbrShader.SetUniform("aoMap", &four);
	pbrShader.SetUniform("irradianceMap", &five);
	pbrShader.SetUniform("prefilterMap", &six);
	pbrShader.SetUniform("brdfLUT", &seven);

	ShaderProgram meshShader("../../Phoenix/RendererOpenGL/App/Resources/Shaders/mesh.vert",
							 "../../Phoenix/RendererOpenGL/App/Resources/Shaders/mesh.frag");

	ShaderProgram equirectangularToCubemapShader("../../Phoenix/RendererOpenGL/App/Resources/Shaders/cubemap.vert",
										   "../../Phoenix/RendererOpenGL/App/Resources/Shaders/equirectangular_to_cubemap.frag");
	ShaderProgram irradianceShader("../../Phoenix/RendererOpenGL/App/Resources/Shaders/cubemap.vert",
								   "../../Phoenix/RendererOpenGL/App/Resources/Shaders/irradiance_convolution.frag");
	ShaderProgram prefilterShader("../../Phoenix/RendererOpenGL/App/Resources/Shaders/cubemap.vert",
								  "../../Phoenix/RendererOpenGL/App/Resources/Shaders/prefilter.frag");
	ShaderProgram brdfShader("../../Phoenix/RendererOpenGL/App/Resources/Shaders/brdf.vert",
							 "../../Phoenix/RendererOpenGL/App/Resources/Shaders/brdf.frag");
	ShaderProgram backgroundShader("../../Phoenix/RendererOpenGL/App/Resources/Shaders/background.vert",
								   "../../Phoenix/RendererOpenGL/App/Resources/Shaders/background.frag");

	glUseProgram(backgroundShader.mId);
	backgroundShader.SetUniform("environmentMap", &zero);

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
	lights.push_back({glm::vec3(-4.0f, 5.0f, 0.0f),		 0.0f,
					  glm::vec3(150.0f, 0.0f, 150.0f), 0.0f});

	lights.push_back({ glm::vec3(3.0f, 5.0f, 0.0f),		  0.0f,
					   glm::vec3(150.0f, 150.0f, 150.0f), 0.0f});
	
	// lights uniform buffer block
	int64_t lightBlockSize = sizeof(LightBlock);
	unsigned int uboLightsBlock;
	glGenBuffers(1, &uboLightsBlock);
	glBindBuffer(GL_UNIFORM_BUFFER, uboLightsBlock);
	glBufferData(GL_UNIFORM_BUFFER, lightBlockSize * NR_LIGHTS, NULL, GL_STATIC_DRAW);
	glBindBuffer(GL_UNIFORM_BUFFER, 0);
	glBindBufferRange(GL_UNIFORM_BUFFER, 0, uboLightsBlock, 0, lightBlockSize * NR_LIGHTS);


	// pbr: setup framebuffer
	// ----------------------
	unsigned int captureFBO;
	unsigned int captureRBO;
	glGenFramebuffers(1, &captureFBO);
	glGenRenderbuffers(1, &captureRBO);

	glBindFramebuffer(GL_FRAMEBUFFER, captureFBO);
	glBindRenderbuffer(GL_RENDERBUFFER, captureRBO);
	glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT24, 512, 512);
	glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, captureRBO);

	// pbr: load the HDR environment map
	// ---------------------------------
	unsigned int hdrTexture = (unsigned int)LoadTexture("../../Phoenix/RendererOpenGL/App/Resources/Textures/Barce_Rooftop_C_3k.hdr", true);

	// pbr: setup cubemap to render to and attach to framebuffer
	// ---------------------------------------------------------
	unsigned int envCubemap;
	glGenTextures(1, &envCubemap);
	glBindTexture(GL_TEXTURE_CUBE_MAP, envCubemap);
	for (unsigned int i = 0; i < 6; ++i)
	{
		glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, GL_RGB16F, 512, 512, 0, GL_RGB, GL_FLOAT, nullptr);
	}
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR); // enable pre-filter mipmap sampling (combatting visible dots artifact)
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

	// pbr: set up projection and view matrices for capturing data onto the 6 cubemap face directions
	// ----------------------------------------------------------------------------------------------
	glm::mat4 captureProjection = glm::perspective(glm::radians(90.0f), 1.0f, 0.1f, 10.0f);
	glm::mat4 captureViews[] =
	{
		glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(1.0f,  0.0f,  0.0f), glm::vec3(0.0f, -1.0f,  0.0f)),
		glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(-1.0f,  0.0f,  0.0f), glm::vec3(0.0f, -1.0f,  0.0f)),
		glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f,  1.0f,  0.0f), glm::vec3(0.0f,  0.0f,  1.0f)),
		glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, -1.0f,  0.0f), glm::vec3(0.0f,  0.0f, -1.0f)),
		glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f,  0.0f,  1.0f), glm::vec3(0.0f, -1.0f,  0.0f)),
		glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f,  0.0f, -1.0f), glm::vec3(0.0f, -1.0f,  0.0f))
	};

	// pbr: convert HDR equirectangular environment map to cubemap equivalent
	// ----------------------------------------------------------------------
	glUseProgram(equirectangularToCubemapShader.mId);
	equirectangularToCubemapShader.SetUniform("equirectangularMap", &zero);
	equirectangularToCubemapShader.SetUniform("projection", &captureProjection);
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, hdrTexture);

	glViewport(0, 0, 512, 512); // don't forget to configure the viewport to the capture dimensions.
	glBindFramebuffer(GL_FRAMEBUFFER, captureFBO);
	for (unsigned int i = 0; i < 6; ++i)
	{
		//equirectangularToCubemapShader.setMat4("view", captureViews[i]);
		glUniformMatrix4fv(glGetUniformLocation(equirectangularToCubemapShader.mId, "view"), 1, GL_FALSE, (GLfloat*)&captureViews[i]);
		glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, envCubemap, 0);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		pOpenGLRenderer->RenderCube();
	}
	glBindFramebuffer(GL_FRAMEBUFFER, 0);

	// then let OpenGL generate mipmaps from first mip face (combatting visible dots artifact)
	glBindTexture(GL_TEXTURE_CUBE_MAP, envCubemap);
	glGenerateMipmap(GL_TEXTURE_CUBE_MAP);

	// pbr: create an irradiance cubemap, and re-scale capture FBO to irradiance scale.
	// --------------------------------------------------------------------------------
	unsigned int irradianceMap;
	glGenTextures(1, &irradianceMap);
	glBindTexture(GL_TEXTURE_CUBE_MAP, irradianceMap);
	for (unsigned int i = 0; i < 6; ++i)
	{
		glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, GL_RGB16F, 32, 32, 0, GL_RGB, GL_FLOAT, nullptr);
	}
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

	glBindFramebuffer(GL_FRAMEBUFFER, captureFBO);
	glBindRenderbuffer(GL_RENDERBUFFER, captureRBO);
	glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT24, 32, 32);

	// pbr: solve diffuse integral by convolution to create an irradiance (cube)map.
	// -----------------------------------------------------------------------------
	glUseProgram(irradianceShader.mId);
	irradianceShader.SetUniform("environmentMap", &zero);
	irradianceShader.SetUniform("projection", &captureProjection);
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_CUBE_MAP, envCubemap);

	glViewport(0, 0, 32, 32); // don't forget to configure the viewport to the capture dimensions.
	glBindFramebuffer(GL_FRAMEBUFFER, captureFBO);
	for (unsigned int i = 0; i < 6; ++i)
	{
		//irradianceShader.setMat4("view", captureViews[i]);
		glUniformMatrix4fv(glGetUniformLocation(irradianceShader.mId, "view"), 1, GL_FALSE, (GLfloat*)&captureViews[i]);
		glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, irradianceMap, 0);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		pOpenGLRenderer->RenderCube();
	}
	glBindFramebuffer(GL_FRAMEBUFFER, 0);

	// pbr: create a pre-filter cubemap, and re-scale capture FBO to pre-filter scale.
	// --------------------------------------------------------------------------------
	unsigned int prefilterMap;
	glGenTextures(1, &prefilterMap);
	glBindTexture(GL_TEXTURE_CUBE_MAP, prefilterMap);
	for (unsigned int i = 0; i < 6; ++i)
	{
		glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, GL_RGB16F, 128, 128, 0, GL_RGB, GL_FLOAT, nullptr);
	}
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR); // be sure to set minifcation filter to mip_linear 
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	// generate mipmaps for the cubemap so OpenGL automatically allocates the required memory.
	glGenerateMipmap(GL_TEXTURE_CUBE_MAP);

	// pbr: run a quasi monte-carlo simulation on the environment lighting to create a prefilter (cube)map.
	// ----------------------------------------------------------------------------------------------------
	glUseProgram(prefilterShader.mId);
	prefilterShader.SetUniform("environmentMap", &zero);
	prefilterShader.SetUniform("projection", &captureProjection);
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_CUBE_MAP, envCubemap);

	glBindFramebuffer(GL_FRAMEBUFFER, captureFBO);
	unsigned int maxMipLevels = 5;
	for (unsigned int mip = 0; mip < maxMipLevels; ++mip)
	{
		// reisze framebuffer according to mip-level size.
		unsigned int mipWidth = unsigned int(128 * std::pow(0.5, mip));
		unsigned int mipHeight = unsigned int(128 * std::pow(0.5, mip));
		glBindRenderbuffer(GL_RENDERBUFFER, captureRBO);
		glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT24, mipWidth, mipHeight);
		glViewport(0, 0, mipWidth, mipHeight);

		float roughness = (float)mip / (float)(maxMipLevels - 1);
		prefilterShader.SetUniform("roughness", &roughness);
		for (unsigned int i = 0; i < 6; ++i)
		{
			prefilterShader.SetUniform("view", &captureViews[i]);
			glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, prefilterMap, mip);

			glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
			pOpenGLRenderer->RenderCube();
		}
	}
	glBindFramebuffer(GL_FRAMEBUFFER, 0);

	// pbr: generate a 2D LUT from the BRDF equations used.
	// ----------------------------------------------------
	unsigned int brdfLUTTexture;
	glGenTextures(1, &brdfLUTTexture);

	// pre-allocate enough memory for the LUT texture.
	glBindTexture(GL_TEXTURE_2D, brdfLUTTexture);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RG16F, 512, 512, 0, GL_RG, GL_FLOAT, 0);
	// be sure to set wrapping mode to GL_CLAMP_TO_EDGE
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

	// then re-configure capture framebuffer object and render screen-space quad with BRDF shader.
	glBindFramebuffer(GL_FRAMEBUFFER, captureFBO);
	glBindRenderbuffer(GL_RENDERBUFFER, captureRBO);
	glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT24, 512, 512);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, brdfLUTTexture, 0);

	glViewport(0, 0, 512, 512);
	glUseProgram(brdfShader.mId);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	pOpenGLRenderer->RenderQuad();

	glBindFramebuffer(GL_FRAMEBUFFER, 0);


	// initialize static shader uniforms before rendering
	// --------------------------------------------------
	glm::mat4 projection = glm::perspective(glm::radians(camera.Zoom), (float)window.windowWidth() / (float)window.windowHeight(), 0.1f, 100.0f);
	glUseProgram(pbrShader.mId);
	pbrShader.SetUniform("projection", &projection);
	glUseProgram(backgroundShader.mId);
	backgroundShader.SetUniform("projection", &projection);

	// then before rendering, configure the viewport to the original framebuffer's screen dimensions
	glViewport(0, 0, window.windowWidth(), window.windowHeight());

	glm::vec3 planeColor(0.5f, 0.5f, 0.6f);
	// FOR PLANE
	PBRMat planeMaterial(planeColor, 0.0f, 1.0f, 1.0f);

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

			// bind pre-computed IBL data
			glActiveTexture(GL_TEXTURE5);
			glBindTexture(GL_TEXTURE_CUBE_MAP, irradianceMap);
			glActiveTexture(GL_TEXTURE6);
			glBindTexture(GL_TEXTURE_CUBE_MAP, prefilterMap);
			glActiveTexture(GL_TEXTURE7);
			glBindTexture(GL_TEXTURE_2D, brdfLUTTexture);

			// update lights data
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
			model = glm::mat4(1.0f);
			model = glm::translate(model, glm::vec3(0.0f, -2.0f, 0.0f));
			model = glm::scale(model, glm::vec3(10.0f, 0.5f, 10.0f));
			glUseProgram(pbrShader.mId);
			pbrShader.SetUniform("isNotTextured", &one);
			planeMaterial.UpdateMaterial(&pbrShader);
			pbrShader.SetUniform("model", &model);
			pOpenGLRenderer->RenderCube();
			pbrShader.SetUniform("isNotTextured", &zero);

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

			// render skybox (render as last to prevent overdraw)
			glUseProgram(backgroundShader.mId);
			backgroundShader.SetUniform("projection", &projection);
			backgroundShader.SetUniform("view", &view);
			glActiveTexture(GL_TEXTURE0);
			glBindTexture(GL_TEXTURE_CUBE_MAP, envCubemap);
			//glBindTexture(GL_TEXTURE_CUBE_MAP, irradianceMap); // display irradiance map
			//glBindTexture(GL_TEXTURE_CUBE_MAP, prefilterMap); // display prefilter map
			pOpenGLRenderer->RenderCube();

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

				glm::vec3 col = planeMaterial.getAlbedo();
				float metallic = planeMaterial.getMetallic();
				float roughness = planeMaterial.getRoughness();
				float ao = planeMaterial.getAo();

				ImGui::Begin("Plane");
				ImGui::InputFloat("ColorX", &col.x, 1.0f, 0.0f, 3);
				ImGui::InputFloat("ColorY", &col.y, 1.0f, 0.0f, 3);
				ImGui::InputFloat("ColorZ", &col.z, 1.0f, 0.0f, 3);
				ImGui::InputFloat("Metallic", &metallic, 1.0f, 0.0f, 3);
				ImGui::InputFloat("Roughness", &roughness, 1.0f, 0.0f, 3);
				ImGui::InputFloat("Ao", &ao, 1.0f, 0.0f, 3);
				ImGui::End();

				planeMaterial.setAlbedo(col);
				planeMaterial.setMetallic(metallic);
				planeMaterial.setRoughness(roughness);
				planeMaterial.setAo(ao);

				ImGui::Begin("Lights");
				int num = 1;
				for (num; num <= activeLights; ++num)
				{
					ImGui::InputFloat(("Light" + std::to_string(num) + " PosX").c_str(), &lights[num - 1].Position.x, 1.0f, 0.0f, 3);
					ImGui::InputFloat(("Light" + std::to_string(num) + " PosY").c_str(), &lights[num - 1].Position.y, 1.0f, 0.0f, 3);
					ImGui::InputFloat(("Light" + std::to_string(num) + " PosZ").c_str(), &lights[num - 1].Position.z, 1.0f, 0.0f, 3);

					ImGui::InputFloat(("Light" + std::to_string(num) + " ColorX").c_str(), &lights[num - 1].Color.x, 1.0f, 0.0f, 3);
					ImGui::InputFloat(("Light" + std::to_string(num) + " ColorY").c_str(), &lights[num - 1].Color.y, 1.0f, 0.0f, 3);
					ImGui::InputFloat(("Light" + std::to_string(num) + " ColorZ").c_str(), &lights[num - 1].Color.z, 1.0f, 0.0f, 3);
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