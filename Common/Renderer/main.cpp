#include "VulkanRenderer.h"

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <chrono>

struct Vertex
{
	glm::vec3 pos;
	glm::vec3 color;
	glm::vec2 texCoord;

	Vertex(glm::vec3 _pos, glm::vec3 _color, glm::vec2 _texCoord)
	{
		pos = _pos; color = _color; texCoord = _texCoord;
	}

	static VkVertexInputBindingDescription getBindingDescription()
	{
		VkVertexInputBindingDescription bindingDescription = {};
		bindingDescription.binding = 0;
		bindingDescription.stride = sizeof(Vertex);
		bindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

		return bindingDescription;
	}

	static tinystl::vector<VkVertexInputAttributeDescription> getAttributeDescriptions()
	{
		tinystl::vector<VkVertexInputAttributeDescription> attributeDescriptions(3);

		attributeDescriptions[0].binding = 0;
		attributeDescriptions[0].location = 0;
		attributeDescriptions[0].format = VK_FORMAT_R32G32B32_SFLOAT;
		attributeDescriptions[0].offset = offsetof(Vertex, pos);

		attributeDescriptions[1].binding = 0;
		attributeDescriptions[1].location = 1;
		attributeDescriptions[1].format = VK_FORMAT_R32G32B32_SFLOAT;
		attributeDescriptions[1].offset = offsetof(Vertex, color);

		attributeDescriptions[2].binding = 0;
		attributeDescriptions[2].location = 2;
		attributeDescriptions[2].format = VK_FORMAT_R32G32_SFLOAT;
		attributeDescriptions[2].offset = offsetof(Vertex, texCoord);

		return attributeDescriptions;
	}
};

tinystl::vector<Vertex> vertices;
tinystl::vector<uint16_t> indices;

PH_SwapChain swapchain;

struct UniformBufferObject
{
	alignas(16) glm::mat4 model;
	alignas(16) glm::mat4 view;
	alignas(16) glm::mat4 proj;
};

PH_Buffer  uniformBuffer;
PH_Texture texture;
VkSampler textureSampler;

class Application
{
public:
	Application()
	{
		vertices.emplace_back(Vertex(glm::vec3(-0.5f, -0.5f, 0.0f), glm::vec3(1.0f, 0.0f, 0.0f), glm::vec2(1.0f, 0.0f)));
		vertices.emplace_back(Vertex(glm::vec3(0.5f, -0.5f, 0.0f), glm::vec3(0.0f, 1.0f, 0.0f), glm::vec2(0.0f, 0.0f)));
		vertices.emplace_back(Vertex(glm::vec3(0.5f, 0.5f, 0.0f), glm::vec3(0.0f, 0.0f, 1.0f), glm::vec2(0.0f, 1.0f)));
		vertices.emplace_back(Vertex(glm::vec3(-0.5f, 0.5f, 0.0f), glm::vec3(1.0f, 1.0f, 1.0f), glm::vec2(1.0f, 1.0f)));

		vertices.emplace_back(Vertex(glm::vec3(-0.5f, -0.5f, -0.5f), glm::vec3(1.0f, 0.0f, 0.0f), glm::vec2(1.0f, 0.0f)));
		vertices.emplace_back(Vertex(glm::vec3(0.5f, -0.5f, -0.5f), glm::vec3(0.0f, 1.0f, 0.0f), glm::vec2(0.0f, 0.0f)));
		vertices.emplace_back(Vertex(glm::vec3(0.5f, 0.5f, -0.5f), glm::vec3(0.0f, 0.0f, 1.0f), glm::vec2(0.0f, 1.0f)));
		vertices.emplace_back(Vertex(glm::vec3(-0.5f, 0.5f, -0.5f), glm::vec3(1.0f, 1.0f, 1.0f), glm::vec2(1.0f, 1.0f)));

		indices.emplace_back(0);  indices.emplace_back(1);  indices.emplace_back(2);  indices.emplace_back(2);  indices.emplace_back(3);  indices.emplace_back(0);
		indices.emplace_back(4);  indices.emplace_back(5);  indices.emplace_back(6);  indices.emplace_back(6);  indices.emplace_back(7);  indices.emplace_back(4);
	}

	void run() {
		renderer.initWindow();
		renderer.enableDepth();
		renderer.initVulkan();

		renderer.addSwapChain(&swapchain);

		PH_Pipeline mPipeline;

		// color
		mPipeline.mColorAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
		mPipeline.mColorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
		mPipeline.mColorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
		mPipeline.mColorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
		mPipeline.mColorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
		mPipeline.mColorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		mPipeline.mColorAttachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

		// depth
		mPipeline.mDepthAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
		mPipeline.mDepthAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
		mPipeline.mDepthAttachment.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
		mPipeline.mDepthAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
		mPipeline.mDepthAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
		mPipeline.mDepthAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		mPipeline.mDepthAttachment.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;


		// vertex bindings
		VkVertexInputBindingDescription bindingDescription = Vertex::getBindingDescription();
		tinystl::vector<VkVertexInputAttributeDescription> attributeDescriptions = Vertex::getAttributeDescriptions();

		mPipeline.mVertexInputState.vertexBindingDescriptionCount = 1;
		mPipeline.mVertexInputState.pVertexBindingDescriptions = &bindingDescription;
		mPipeline.mVertexInputState.vertexAttributeDescriptionCount = static_cast<uint32_t>(attributeDescriptions.size());
		mPipeline.mVertexInputState.pVertexAttributeDescriptions = attributeDescriptions.data();

		mPipeline.mVertex.data  = vertices.data();
		mPipeline.mVertex.size  = sizeof(vertices[0]) * vertices.size();
		mPipeline.mVertex.count = static_cast<uint32_t>(vertices.size());
		
		mPipeline.mIndex.data  = indices.data();
		mPipeline.mIndex.size  = sizeof(indices[0]) * indices.size();
		mPipeline.mIndex.count = static_cast<uint32_t>(indices.size());

		renderer.createGraphicsPipeline(&swapchain, mPipeline); //

		// UNIFORM
		uniformBuffer.bufferDesc.mBufferSize = sizeof(UniformBufferObject);
		uniformBuffer.bufferDesc.mUsage = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;
		uniformBuffer.bufferDesc.mProperties = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;

		renderer.createUniformBuffers(&swapchain, &uniformBuffer);
		
		for (size_t i = 0; i < swapchain.swapChainImages.size(); i++)
		{
			swapchain.bufferUpdateInfo.bufferInfo.buffer = uniformBuffer.mBuffers[i];
			swapchain.bufferUpdateInfo.bufferInfo.offset = 0;
			swapchain.bufferUpdateInfo.bufferInfo.range = sizeof(UniformBufferObject);
		}

		// TEXTURE
		texture.filename = "../../Phoenix/App/Test/textures/texture.jpg";

		swapchain.textureUpdateInfo.imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		
		renderer.createTextureImage(&texture);
		swapchain.textureUpdateInfo.imageInfo.imageView = texture.textureImageView;
		
		renderer.createTextureSampler(&textureSampler);
		swapchain.textureUpdateInfo.imageInfo.sampler = textureSampler;

		renderer.initVulkan2(&swapchain);

		renderer.createCommandBuffers(swapchain, mPipeline);

		while (!renderer.windowShouldClose() && !exit)
		{
			renderer.updateInputs();

			mainLoop();

			renderer.pollEvents();
		}

		renderer.waitDeviceIdle();
		renderer.destroySampler(&textureSampler);
		renderer.cleanupVulkan();
		renderer.destroyWindow();
	}

private:

	void mainLoop()
	{
		uint32_t imageIndex = renderer.acquireNextImage(&swapchain);
		if (imageIndex == -1)
			return;

		//renderer.drawFrame(&swapchain);
		// update uniform buffer
		static std::chrono::time_point<std::chrono::steady_clock>	startTime = std::chrono::high_resolution_clock::now();

		std::chrono::time_point<std::chrono::steady_clock>		  currentTime = std::chrono::high_resolution_clock::now();
		float time = std::chrono::duration<float, std::chrono::seconds::period>(currentTime - startTime).count();

		UniformBufferObject ubo = {};
		ubo.model = glm::rotate(glm::mat4(1.0f), time * glm::radians(90.0f), glm::vec3(0.0f, 0.0f, 1.0f));
		ubo.view = glm::lookAt(glm::vec3(2.0f, 2.0f, 2.0f), glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 0.0f, 1.0f));
		ubo.proj = glm::perspective(glm::radians(45.0f), swapchain.swapChainExtent.width / (float)swapchain.swapChainExtent.height, 0.1f, 10.0f);
		ubo.proj[1][1] *= -1;

		uniformBuffer.data = &ubo;
		uniformBuffer.size = sizeof(UniformBufferObject);

		renderer.updateUniformBuffer(imageIndex, uniformBuffer);
		//

		renderer.drawFrame(&swapchain, imageIndex);

		if (renderer.isKeyTriggered(PH_KEY_ESCAPE))
		{
			exit = true;
		}
	}

	bool exit = false;
	VulkanRenderer renderer;
};

int main()
{
	Application app;
	try {
		app.run();
	}
	catch (const std::exception& e) {
		std::cerr << e.what() << std::endl;
		return EXIT_FAILURE;
	}

	return EXIT_SUCCESS;

	return 0;
}