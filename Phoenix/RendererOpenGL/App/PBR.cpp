#include "../Picker.h"

#if PBR

#include "../Common.h"
#include <random>

const int NR_LIGHTS = 100;
struct LightBlock
{
	glm::vec3 Position;
	float pad0;
	glm::vec3 Color;
	float pad1;
};

OpenGLRenderer* pOpenGLRenderer = NULL;
PBRMat_Tex rustediron;
PBRMat_Tex military_panel;
PBRMat_Tex streaked_metal;
PBRMat_Tex metalgrid;
PBRMat_Tex titanium;
glm::vec3 planeColor(0.5f, 0.5f, 0.6f);
PBRMat planeMaterial(planeColor, 0.0f, 1.0f, 1.0f);
SkinnedMesh nanosuit;

float lerp(float a, float b, float f)
{
	return a + f * (b - a);
}

void RenderScene(ShaderProgram& pbrShader)
{
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
	int zero = 0, one = 1;
	pbrShader.SetUniform("isNotTextured", &one);
	planeMaterial.UpdateMaterial(&pbrShader);
	
	pbrShader.SetUniform("model", &model);
	pOpenGLRenderer->RenderCube();
	
	model = glm::mat4(1.0f);
	model = glm::translate(model, glm::vec3(0.0f, 0.0f, 0.0f));
	model = glm::scale(model, glm::vec3(1.0f, 1.0f, 1.0f));
	pbrShader.SetUniform("model", &model);
	pOpenGLRenderer->RenderCube();

	/*model = glm::mat4(1.0f);
	model = glm::translate(model, glm::vec3(0.0f, 10.0f, 0.0f));
	model = glm::scale(model, glm::vec3(0.25f));
	pbrShader.SetUniform("model", &model);
	nanosuit.Render(pbrShader);*/

	pbrShader.SetUniform("isNotTextured", &zero);
}

void Run()
{
	window.initWindow();
	lastX = window.windowWidth() / 2.0f;
	lastY = window.windowHeight() / 2.0f;
	int zero = 0, one = 1, two = 2, three = 3, four = 4, five = 5, six = 6, seven = 7, eight = 8;

	camera.Position = glm::vec3(0.0f, 2.0f, 10.0f);

	pOpenGLRenderer = new OpenGLRenderer();
	bool isAmbientOcclusion = false;

	glEnable(GL_DEPTH_TEST);
	glDepthFunc(GL_LEQUAL);
	glEnable(GL_TEXTURE_CUBE_MAP_SEAMLESS);
	glEnable(GL_MULTISAMPLE);

	ShaderProgram gBufferShader("../../Phoenix/RendererOpenGL/App/Resources/Shaders/gBufferAO.vert",
								"../../Phoenix/RendererOpenGL/App/Resources/Shaders/gBufferAO.frag");

	glUseProgram(gBufferShader.mId);
	gBufferShader.SetUniform("albedoMap", &zero);
	gBufferShader.SetUniform("normalMap", &one);
	gBufferShader.SetUniform("metallicMap", &two);
	gBufferShader.SetUniform("roughnessMap", &three);
	gBufferShader.SetUniform("aoMap", &four);

	ShaderProgram shaderSSAO("../../Phoenix/RendererOpenGL/App/Resources/Shaders/ssao.vert",
							 "../../Phoenix/RendererOpenGL/App/Resources/Shaders/ssao.frag");
	// SSAO uniform modifiers
	int kernelSize = 64;
	float radius = 0.5f;
	float bias = 0.025f;

	ShaderProgram shaderSSAOBlur("../../Phoenix/RendererOpenGL/App/Resources/Shaders/ssao.vert",
								 "../../Phoenix/RendererOpenGL/App/Resources/Shaders/ssao_blur.frag");


	glUseProgram(shaderSSAO.mId);
	shaderSSAO.SetUniform("gPosition", &zero);
	shaderSSAO.SetUniform("gNormal", &one);
	shaderSSAO.SetUniform("texNoise", &two);
	glUseProgram(shaderSSAOBlur.mId);
	shaderSSAOBlur.SetUniform("ssaoInput", &zero);

	// configure g-buffer framebuffer
	// ------------------------------
	unsigned int gBuffer;
	glGenFramebuffers(1, &gBuffer);
	glBindFramebuffer(GL_FRAMEBUFFER, gBuffer);
	unsigned int gPosition, gNormal, gAlbedo, gMaterial;
	// position color buffer
	glGenTextures(1, &gPosition);
	glBindTexture(GL_TEXTURE_2D, gPosition);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB16F, window.windowWidth(), window.windowHeight(), 0, GL_RGB, GL_FLOAT, NULL);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, gPosition, 0);
	// normal color buffer
	glGenTextures(1, &gNormal);
	glBindTexture(GL_TEXTURE_2D, gNormal);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB16F, window.windowWidth(), window.windowHeight(), 0, GL_RGB, GL_FLOAT, NULL);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT1, GL_TEXTURE_2D, gNormal, 0);
	// color + specular color buffer
	glGenTextures(1, &gAlbedo);
	glBindTexture(GL_TEXTURE_2D, gAlbedo);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, window.windowWidth(), window.windowHeight(), 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT2, GL_TEXTURE_2D, gAlbedo, 0);
	//material buffer
	glGenTextures(1, &gMaterial);
	glBindTexture(GL_TEXTURE_2D, gMaterial);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB16F, window.windowWidth(), window.windowHeight(), 0, GL_RGB, GL_UNSIGNED_BYTE, NULL);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT3, GL_TEXTURE_2D, gMaterial, 0);
	// tell OpenGL which color attachments we'll use (of this framebuffer) for rendering 
	unsigned int attachments[4] = { GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1, GL_COLOR_ATTACHMENT2, GL_COLOR_ATTACHMENT3 };
	glDrawBuffers(4, attachments);
	// create and attach depth buffer (renderbuffer)
	unsigned int rboDepth;
	glGenRenderbuffers(1, &rboDepth);
	glBindRenderbuffer(GL_RENDERBUFFER, rboDepth);
	glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT, window.windowWidth(), window.windowHeight());
	glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, rboDepth);
	// finally check if framebuffer is complete
	if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
		assert(0);
	glBindFramebuffer(GL_FRAMEBUFFER, 0);


	// also create framebuffer to hold SSAO processing stage 
	// -----------------------------------------------------
	unsigned int ssaoFBO, ssaoBlurFBO;
	glGenFramebuffers(1, &ssaoFBO);  glGenFramebuffers(1, &ssaoBlurFBO);
	glBindFramebuffer(GL_FRAMEBUFFER, ssaoFBO);
	unsigned int ssaoColorBuffer, ssaoColorBufferBlur;
	// SSAO color buffer
	glGenTextures(1, &ssaoColorBuffer);
	glBindTexture(GL_TEXTURE_2D, ssaoColorBuffer);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RED, window.windowWidth(), window.windowHeight(), 0, GL_RGB, GL_FLOAT, NULL);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, ssaoColorBuffer, 0);
	if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
		assert(0);
	// and blur stage
	glBindFramebuffer(GL_FRAMEBUFFER, ssaoBlurFBO);
	glGenTextures(1, &ssaoColorBufferBlur);
	glBindTexture(GL_TEXTURE_2D, ssaoColorBufferBlur);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RED, window.windowWidth(), window.windowHeight(), 0, GL_RGB, GL_FLOAT, NULL);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, ssaoColorBufferBlur, 0);
	if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
		assert(0);
	glBindFramebuffer(GL_FRAMEBUFFER, 0);

	// generate sample kernel
	// ----------------------
	std::uniform_real_distribution<GLfloat> randomFloats(0.0, 1.0); // generates random floats between 0.0 and 1.0
	std::default_random_engine generator;
	std::vector<glm::vec3> ssaoKernel;
	for (unsigned int i = 0; i < 64; ++i)
	{
		glm::vec3 sample(randomFloats(generator) * 2.0 - 1.0, randomFloats(generator) * 2.0 - 1.0, randomFloats(generator));
		sample = glm::normalize(sample);
		sample *= randomFloats(generator);
		float scale = float(i) / 64.0f;

		// scale samples s.t. they're more aligned to center of kernel
		scale = lerp(0.1f, 1.0f, scale * scale);
		sample *= scale;
		ssaoKernel.push_back(sample);
	}

	// generate noise texture
	// ----------------------
	std::vector<glm::vec3> ssaoNoise;
	for (unsigned int i = 0; i < 16; i++)
	{
		glm::vec3 noise(randomFloats(generator) * 2.0 - 1.0, randomFloats(generator) * 2.0 - 1.0, 0.0f); // rotate around z-axis (in tangent space)
		ssaoNoise.push_back(noise);
	}
	unsigned int noiseTexture; glGenTextures(1, &noiseTexture);
	glBindTexture(GL_TEXTURE_2D, noiseTexture);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB32F, 4, 4, 0, GL_RGB, GL_FLOAT, &ssaoNoise[0]);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);


	ShaderProgram pbrShader("../../Phoenix/RendererOpenGL/App/Resources/Shaders/pbrAO.vert",
							"../../Phoenix/RendererOpenGL/App/Resources/Shaders/pbrAO.frag");
	
	glUseProgram(pbrShader.mId);
	pbrShader.SetUniform("gPosition", &zero);
	pbrShader.SetUniform("gNormal", &one);
	pbrShader.SetUniform("gAlbedo", &two);
	pbrShader.SetUniform("gMaterial", &three);
	pbrShader.SetUniform("irradianceMap", &five);
	pbrShader.SetUniform("prefilterMap", &six);
	pbrShader.SetUniform("brdfLUT", &seven);
	pbrShader.SetUniform("ssao", &eight);

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

	rustediron.LoadPBRTexture("../../Phoenix/RendererOpenGL/App/Resources/PBR/rustediron1-alt2-Unreal-Engine/albedo.png", ALBEDO);
	rustediron.LoadPBRTexture("../../Phoenix/RendererOpenGL/App/Resources/PBR/rustediron1-alt2-Unreal-Engine/normal.png", NORMAL);
	rustediron.LoadPBRTexture("../../Phoenix/RendererOpenGL/App/Resources/PBR/rustediron1-alt2-Unreal-Engine/metallic.png", METALLIC);
	rustediron.LoadPBRTexture("../../Phoenix/RendererOpenGL/App/Resources/PBR/rustediron1-alt2-Unreal-Engine/roughness.png", ROUGHNESS);
	rustediron.LoadPBRTexture("../../Phoenix/RendererOpenGL/App/Resources/PBR/rustediron1-alt2-Unreal-Engine/ao.jpg", AO);

	military_panel.LoadPBRTexture("../../Phoenix/RendererOpenGL/App/Resources/PBR/military-panel1-ue/albedo.png", ALBEDO);
	military_panel.LoadPBRTexture("../../Phoenix/RendererOpenGL/App/Resources/PBR/military-panel1-ue/normal.png", NORMAL);
	military_panel.LoadPBRTexture("../../Phoenix/RendererOpenGL/App/Resources/PBR/military-panel1-ue/metallic.png", METALLIC);
	military_panel.LoadPBRTexture("../../Phoenix/RendererOpenGL/App/Resources/PBR/military-panel1-ue/roughness.png", ROUGHNESS);
	military_panel.LoadPBRTexture("../../Phoenix/RendererOpenGL/App/Resources/PBR/military-panel1-ue/ao.png", AO);

	streaked_metal.LoadPBRTexture("../../Phoenix/RendererOpenGL/App/Resources/PBR/streaked-metal1-ue/albedo.png", ALBEDO);
	streaked_metal.LoadPBRTexture("../../Phoenix/RendererOpenGL/App/Resources/PBR/streaked-metal1-ue/normal.png", NORMAL);
	streaked_metal.LoadPBRTexture("../../Phoenix/RendererOpenGL/App/Resources/PBR/streaked-metal1-ue/metallic.png", METALLIC);
	streaked_metal.LoadPBRTexture("../../Phoenix/RendererOpenGL/App/Resources/PBR/streaked-metal1-ue/roughness.png", ROUGHNESS);
	streaked_metal.LoadPBRTexture("../../Phoenix/RendererOpenGL/App/Resources/PBR/streaked-metal1-ue/ao.png", AO);

	metalgrid.LoadPBRTexture("../../Phoenix/RendererOpenGL/App/Resources/PBR/metalgrid4-ue/albedo.png", ALBEDO);
	metalgrid.LoadPBRTexture("../../Phoenix/RendererOpenGL/App/Resources/PBR/metalgrid4-ue/normal.png", NORMAL);
	metalgrid.LoadPBRTexture("../../Phoenix/RendererOpenGL/App/Resources/PBR/metalgrid4-ue/metallic.png", METALLIC);
	metalgrid.LoadPBRTexture("../../Phoenix/RendererOpenGL/App/Resources/PBR/metalgrid4-ue/roughness.png", ROUGHNESS);
	metalgrid.LoadPBRTexture("../../Phoenix/RendererOpenGL/App/Resources/PBR/metalgrid4-ue/ao.png", AO);

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

	// then before rendering, configure the viewport to the original framebuffer's screen dimensions
	glViewport(0, 0, window.windowWidth(), window.windowHeight());

	nanosuit.LoadMesh("../../Phoenix/RendererOpenGL/App/Resources/Objects/nanosuit/nanosuit.obj");

	window.initGui();

	while (!window.windowShouldClose() && !exitOnESC)
	{
		window.startFrame();

		{
			float near_plane = 0.1f, far_plane = 100.0f;

			glm::mat4 projection = glm::perspective(glm::radians(camera.Zoom), (float)window.windowWidth() / (float)window.windowHeight(), near_plane, far_plane);
			glm::mat4 view = camera.GetViewMatrix();

			glClearColor(0.1f, 0.1f, 0.1f, 0.0f);
			glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);


			// G BUFFER PASS
			glBindFramebuffer(GL_FRAMEBUFFER, gBuffer);
				glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
				glUseProgram(gBufferShader.mId);
				gBufferShader.SetUniform("projection", &projection);
				gBufferShader.SetUniform("view", &view);
				RenderScene(gBufferShader);
			glBindFramebuffer(GL_FRAMEBUFFER, 0);


			// SSAO
			// 2. generate SSAO texture
			// ------------------------
			glBindFramebuffer(GL_FRAMEBUFFER, ssaoFBO);
			glClear(GL_COLOR_BUFFER_BIT);
			glUseProgram(shaderSSAO.mId);
			// Send kernel + rotation
			for (unsigned int i = 0; i < 64; ++i)
			{
				glUniform3fv(glGetUniformLocation(shaderSSAO.mId, ("samples[" + std::to_string(i) + "]").c_str()),
							1, (GLfloat*)&ssaoKernel[i]);
				//shaderSSAO.setVec3("samples[" + std::to_string(i) + "]", ssaoKernel[i]);
			}
			glm::vec2 noiseScale((float)(window.windowWidth() / 4), (float)(window.windowHeight() / 4));
			shaderSSAO.SetUniform("projection", &projection);
			shaderSSAO.SetUniform("kernelSize", &kernelSize);
			shaderSSAO.SetUniform("radius", &radius);
			shaderSSAO.SetUniform("bias", &bias);
			shaderSSAO.SetUniform("noiseScale", &noiseScale);
			glActiveTexture(GL_TEXTURE0);
			glBindTexture(GL_TEXTURE_2D, gPosition);
			glActiveTexture(GL_TEXTURE1);
			glBindTexture(GL_TEXTURE_2D, gNormal);
			glActiveTexture(GL_TEXTURE2);
			glBindTexture(GL_TEXTURE_2D, noiseTexture);
			pOpenGLRenderer->RenderQuad();
			glBindFramebuffer(GL_FRAMEBUFFER, 0);

			// 3. blur SSAO texture to remove noise
			// ------------------------------------
			glBindFramebuffer(GL_FRAMEBUFFER, ssaoBlurFBO);
			glClear(GL_COLOR_BUFFER_BIT);
			glUseProgram(shaderSSAOBlur.mId);
			glActiveTexture(GL_TEXTURE0);
			glBindTexture(GL_TEXTURE_2D, ssaoColorBuffer);
			pOpenGLRenderer->RenderQuad();
			glBindFramebuffer(GL_FRAMEBUFFER, 0);

			// update lights data
			glBindBuffer(GL_UNIFORM_BUFFER, uboLightsBlock);
			glBufferSubData(GL_UNIFORM_BUFFER, 0, lightBlockSize * lights.size(), lights.data());
			glBindBuffer(GL_UNIFORM_BUFFER, 0);

			// LIGHTING PASS
			glUseProgram(pbrShader.mId);
			pbrShader.SetUniform("projection", &projection);
			pbrShader.SetUniform("view", &view);
			pbrShader.SetUniform("camPos", &camera.Position);

			// bind pre-computed IBL data
			glActiveTexture(GL_TEXTURE0);
			glBindTexture(GL_TEXTURE_2D, gPosition);
			glActiveTexture(GL_TEXTURE1);
			glBindTexture(GL_TEXTURE_2D, gNormal);
			glActiveTexture(GL_TEXTURE2);
			glBindTexture(GL_TEXTURE_2D, gAlbedo);
			glActiveTexture(GL_TEXTURE3);
			glBindTexture(GL_TEXTURE_2D, gMaterial);
			glActiveTexture(GL_TEXTURE5);
			glBindTexture(GL_TEXTURE_CUBE_MAP, irradianceMap);
			glActiveTexture(GL_TEXTURE6);
			glBindTexture(GL_TEXTURE_CUBE_MAP, prefilterMap);
			glActiveTexture(GL_TEXTURE7);
			glBindTexture(GL_TEXTURE_2D, brdfLUTTexture);
			glActiveTexture(GL_TEXTURE8); // add extra SSAO texture to lighting pass
			glBindTexture(GL_TEXTURE_2D, ssaoColorBufferBlur);

			int activeLights = (int)lights.size();
			pbrShader.SetUniform("activeLights", &activeLights);
			int isAO = (int)isAmbientOcclusion;
			pbrShader.SetUniform("isAmbientOcclusion", &isAO);
			
			glDepthFunc(GL_ONE_MINUS_SRC_ALPHA);
			pOpenGLRenderer->RenderQuad();
			glDepthFunc(GL_LEQUAL);

			// representation of light
			glUseProgram(meshShader.mId);
			meshShader.SetUniform("projection", &projection);
			meshShader.SetUniform("view", &view);
			for (int k = 0; k < activeLights; ++k)
			{
				glm::mat4 model = glm::mat4(1.0f);
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
				ImGui::Checkbox("Ambient Occlusion", &isAmbientOcclusion);
				ImGui::InputInt("Kernel Size", &kernelSize, 1, 0);
				ImGui::InputFloat("Radius", &radius, 1.0f, 0.0f, 3);
				ImGui::InputFloat("Bias", &bias, 1.0f, 0.0f, 3);
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