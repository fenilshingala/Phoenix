#include "../RendererOpenGL/Picker.h"

#if PHYSICS

#include "../RendererOpenGL/Common.h"
#include "../../Middleware/ECS/EntityManager.h"

#include "Components/ModelComponent.h"

#include "Components/LightComponent.h"
#include "Systems/LightingSystem.h"

EntityManager* pEntityManager = NULL;
OpenGLRenderer* pOpenGLRenderer = NULL;

LightingSystem* pLightingSystem = NULL;

const unsigned int NR_LIGHTS = 100;
EntityID lights[NR_LIGHTS];

ShaderProgram* shaderGeometryPass = NULL;
ShaderProgram* shaderLightingPass = NULL;
ShaderProgram* shaderLightBox = NULL;

ModelComponent* pSponzaModel = NULL;

float constant = 1.0f; // note that we don't send this to the shader, we assume it is always 1.0 (in our case)
float linear = -10.0f, prevLinear = -10.0f;
float quadratic = 19.1f, prevQuadratic = 19.1f;

//---------------------------------- G-Buffer
unsigned int gBuffer, gPosition, gNormal, gAlbedoSpec;
void configureGBuffer()
{
	// configure g-buffer framebuffer
	// ------------------------------
	glGenFramebuffers(1, &gBuffer);
	glBindFramebuffer(GL_FRAMEBUFFER, gBuffer);
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
		assert(0);
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void Init()
{
	window.initWindow();
	lastX = window.windowWidth() / 2.0f;
	lastY = window.windowHeight() / 2.0f;

	glEnable(GL_DEPTH_TEST);

	pEntityManager = new EntityManager();
	pOpenGLRenderer = new OpenGLRenderer();
	pLightingSystem = new LightingSystem(pEntityManager);

	EntityID sponzaId = pEntityManager->createEntity();
	Entity* pSponza = pEntityManager->getEntityByID(sponzaId);

	// add model component
	pEntityManager->addComponent<ModelComponent>(sponzaId);
	pSponzaModel = pSponza->GetComponent<ModelComponent>();
	pSponzaModel->filepath = "../../Phoenix/RendererOpenGL/Objects/sponza/sponza.obj";
	pSponzaModel->InitModel();

	// SHADERS
	shaderGeometryPass = new ShaderProgram("../../Phoenix/RendererOpenGL/Shaders/g_buffer.vert", "../../Phoenix/RendererOpenGL/Shaders/g_buffer.frag");
	shaderLightingPass = new ShaderProgram("../../Phoenix/RendererOpenGL/Shaders/deferred_shading.vert", "../../Phoenix/RendererOpenGL/Shaders/deferred_shading.frag");
	shaderLightBox = new ShaderProgram("../../Phoenix/RendererOpenGL/Shaders/deferred_light_box.vert", "../../Phoenix/RendererOpenGL/Shaders/deferred_light_box.frag");

	configureGBuffer();

	srand(13);
	for (int i = 0; i < NR_LIGHTS; ++i)
	{
		lights[i] = pEntityManager->createEntity();
		pEntityManager->addComponent<LightComponent>(lights[i]);
		Entity* pLight = pEntityManager->getEntityByID(lights[i]);

		LightComponent* pLightComp = pLight->GetComponent<LightComponent>();

		// position
		float xPos = (float)(((rand() % 100) / 100.0f) * 2.5f - 1.5f);
		float yPos = (float)(((rand() % 100) / 100.0f) * 0.5f - 1.5f);
		float zPos = (float)(((rand() % 100) / 100.0f) * 20.0f - 7.0f);

		pLightComp->light.lightPosition = glm::vec3(xPos, yPos, zPos);

		// color
		float rColor = (float)(((rand() % 100) / 200.0f) + 0.5f); // between 0.5 and 1.0
		float gColor = (float)(((rand() % 100) / 200.0f) + 0.5f); // between 0.5 and 1.0
		float bColor = (float)(((rand() % 100) / 200.0f) + 0.5f); // between 0.5 and 1.0

		pLightComp->light.lightColor = glm::vec3(rColor, gColor, bColor);

		pLightComp->light.Linear = -10.0f;
		pLightComp->light.Quadratic = 19.1f;
		pLightComp->light.pad0 = pLightComp->light.pad1 = pLightComp->light.pad2 = pLightComp->light.pad3 = 0.0f;
	}

	pLightingSystem->AddLights(lights, NR_LIGHTS);


	// shader configuration
	// --------------------
	int zero = 0, one = 1, two = 2;
	glUseProgram(shaderLightingPass->mId);
	shaderLightingPass->SetUniform("gPosition", &zero);
	shaderLightingPass->SetUniform("gNormal", &one);
	shaderLightingPass->SetUniform("gAlbedoSpec", &two);

	{
		glUseProgram(shaderGeometryPass->mId);

		int notInstanced = 1;
		shaderGeometryPass->SetUniform("notInstanced", &notInstanced);

		glm::mat4 model = glm::mat4(1.0f);
		model = glm::mat4(1.0f);
		model = glm::rotate(model, glm::radians(90.0f), glm::vec3(0.0f, 1.0f, 0.0f));
		model = glm::translate(model, glm::vec3(0.0, -2.0, 0.0));
		model = glm::scale(model, glm::vec3(0.01f));
		shaderGeometryPass->SetUniform("model", &model);
	}

	window.initGui();
}

void Exit()
{
	window.exitGui();
	window.exitWindow();

	delete pLightingSystem;
	delete pOpenGLRenderer;
	delete pEntityManager;
	delete shaderGeometryPass;
	delete shaderLightingPass;
	delete shaderLightBox;
}

void Update()
{

}

void Draw()
{
	window.beginGuiFrame();
	bool truebool = true;

	// GUI - LIGHTS
	ImGui::Begin("LIGHT SETTINGS!", &truebool);

	ImGui::SliderFloat("linear", &linear, -20.0f, 30.0f);
	ImGui::SliderFloat("quadratic", &quadratic, 0.0f, 30.0f);

	ImGui::End();

	glClearColor(0.5f, 0.5f, 0.5f, 1.0f);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	glm::mat4 projection = glm::perspective(glm::radians(camera.Zoom), (float)window.windowWidth() / (float)window.windowHeight(), 0.1f, 100.0f);
	glm::mat4 view = camera.GetViewMatrix();

	// 1. geometry pass: render scene's geometry/color data into gbuffer
	// -----------------------------------------------------------------
	glBindFramebuffer(GL_FRAMEBUFFER, gBuffer);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	glUseProgram(shaderGeometryPass->mId);
	shaderGeometryPass->SetUniform("projection", &projection);
	shaderGeometryPass->SetUniform("view", &view);
	
	//pSponzaModel->Draw(*shaderGeometryPass);
	std::list<Component*> modelComps = pEntityManager->GetComponents(ModelComponent::getTypeStatic());
	for (std::list<Component*>::iterator it = modelComps.begin(); it != modelComps.end(); ++it)
	{
		ModelComponent* pModelComp = static_cast<ModelComponent*>(*it);
		pModelComp->Draw(*shaderGeometryPass);
	}

	glBindFramebuffer(GL_FRAMEBUFFER, 0);

	// 2. lighting pass: calculate lighting by iterating over a screen filled quad pixel-by-pixel using the gbuffer's content.
	// -----------------------------------------------------------------------------------------------------------------------
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	glUseProgram(shaderLightingPass->mId);
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, gPosition);
	glActiveTexture(GL_TEXTURE1);
	glBindTexture(GL_TEXTURE_2D, gNormal);
	glActiveTexture(GL_TEXTURE2);
	glBindTexture(GL_TEXTURE_2D, gAlbedoSpec);

	shaderLightingPass->SetUniform("viewPos", &(camera.Position));
	// finally render quad
	pOpenGLRenderer->RenderQuad();

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
	glUseProgram(shaderLightBox->mId);
	shaderLightBox->SetUniform("projection", &projection);
	shaderLightBox->SetUniform("view", &view);

	// render instanced cubes as lights
	pOpenGLRenderer->RenderCubeInstanced(NR_LIGHTS);

	// GUI - FPS COUNTERS
	{
		str = "controlled fps: " + std::to_string(window.frameRate());
		ImGui::Begin("BLEH!", &truebool);

		ImGui::Text(str.c_str());
		str = "actual fps: " + std::to_string(window.actualFrameRate());
		ImGui::Text(str.c_str());

		ImGui::End();
	}

	window.endGuiFrame();
	window.swapWindow();
	window.update();
	processInputs();
}

void Run()
{
	Init();

	while (!window.windowShouldClose() && !exitOnESC)
	{
		window.startFrame();

		Update();
		Draw();

		window.endFrame();
	}

	Exit();
}

#endif