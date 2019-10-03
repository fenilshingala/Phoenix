#include "LightingSystem.h"
#include "../Components/LightComponent.h"

#include "../../../RendererOpenGL/RendererOpenGL.h"
#include "../../../../Middleware/ECS/EntityManager.h"

struct instancedData
{
	glm::mat4 lightModel;
	glm::vec3 lightColor;
};

unsigned int uboLightsBlock;
unsigned int instanceVBO;
tinystl::vector<instancedData> mInstanceData;

tinystl::vector<LightBlock> mLights;

LightingSystem::~LightingSystem()
{
	glDeleteBuffers(1, &uboLightsBlock);
}

void LightingSystem::AddLights(uint32_t* lights, uint64_t noOfLights)
{
	for (uint64_t i = 0; i < noOfLights; ++i)
	{
		Entity* pLight = pEntityManager->getEntityByID(lights[i]);
		LightBlock lightBlock = pLight->GetComponent<LightComponent>()->light;
		mLights.push_back(lightBlock);
		
		instancedData instanceData;
		instanceData.lightModel = glm::mat4(1.0f);
		instanceData.lightModel = glm::translate(instanceData.lightModel, glm::vec3(lightBlock.lightPosition.x, lightBlock.lightPosition.y, lightBlock.lightPosition.z));
		instanceData.lightModel = glm::scale(instanceData.lightModel, glm::vec3(0.03f));
		instanceData.lightColor = lightBlock.lightColor;
		mInstanceData.push_back(instanceData);
	}

	// lights uniform buffer block
	int64_t lightBlockSize = sizeof(LightBlock);
	glGenBuffers(1, &uboLightsBlock);
	glBindBuffer(GL_UNIFORM_BUFFER, uboLightsBlock);
	glBufferData(GL_UNIFORM_BUFFER, (GLsizeiptr)lightBlockSize * noOfLights, mLights.data(), GL_STATIC_DRAW);
	glBindBuffer(GL_UNIFORM_BUFFER, 0);
	// define the range of the buffer that links to a uniform binding point
	glBindBufferRange(GL_UNIFORM_BUFFER, 0, uboLightsBlock, 0, (GLsizeiptr)(lightBlockSize * noOfLights));

	// lights instancing buffer
	glGenBuffers(1, &instanceVBO);
	glBindBuffer(GL_ARRAY_BUFFER, instanceVBO);
	glBufferData(GL_ARRAY_BUFFER, noOfLights * sizeof(instancedData), mInstanceData.data(), GL_STATIC_DRAW);
	glBindBuffer(GL_ARRAY_BUFFER, 0);

	BindCubeVAO();
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
	glBindVertexArray(0);
}