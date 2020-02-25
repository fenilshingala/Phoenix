#include "Picker.h"

#if (VULKAN_TUTORIAL)

#include "../../../Common/Renderer/VulkanRenderer.h"
#include "../../../Common/Renderer/Application.h"

#include <chrono>
#include <stdexcept>
#include <iostream>

struct UniformBufferObject
{
	alignas(16) glm::mat4 model;
	alignas(16) glm::mat4 view;
	alignas(16) glm::mat4 proj;
} ubo;

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

	static std::vector<VkVertexInputAttributeDescription> getAttributeDescriptions()
	{
		std::vector<VkVertexInputAttributeDescription> attributeDescriptions(3);

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
std::vector<Vertex> vertices;
std::vector<uint16_t> indices;

PH_Buffer vertexBuffer;
PH_Buffer indexBuffer;
PH_Buffer uniformBuffers[3];
PH_Image texture;
VkSampler sampler;
VkShaderModule vertexShaderModule;
VkShaderModule fragmentShaderModule;
VkDescriptorPool descriptorPool;
VkDescriptorSetLayout descriptorSetLayout;
VkPipelineLayout pipelineLayout;
VkPipeline graphicsPipeline;
std::vector<VkDescriptorSet> descriptorSets;


class Application : public VulkanRenderer
{
	bool firstMouse = true;
	float lastX = 0.0f;
	float lastY = 0.0f;

public:
	Application()
	{}

	~Application()
	{}

	void Init()
	{
		camera.rotation_speed *= 0.25f;
		camera.translation_speed *= 0.5f;
		camera.type = CameraType::FirstPerson;
		camera.set_perspective(60.0f, (float)swapChainExtent.width / (float)swapChainExtent.height, 0.1f, 512.0f);
		camera.set_rotation(glm::vec3(-18.0f, -5.0f, 0.0f));
		camera.set_translation(glm::vec3(0.0f, 2.0f, -3.7f));

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

		// Vertex Buffer
		{
			PH_BufferCreateInfo vertexBufferInfo;
			vertexBufferInfo.data = vertices.data();
			vertexBufferInfo.bufferSize = (VkDeviceSize)(sizeof(vertices[0]) * vertices.size());
			vertexBufferInfo.bufferUsageFlags = VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
			vertexBufferInfo.memoryPropertyFlags = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
			PH_CreateBuffer(vertexBufferInfo, &vertexBuffer);
		}

		// Index Buffer
		{
			PH_BufferCreateInfo indexBufferInfo;
			indexBufferInfo.data = indices.data();
			indexBufferInfo.bufferSize = (VkDeviceSize)(sizeof(indices[0]) * indices.size());
			indexBufferInfo.bufferUsageFlags = VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT;
			indexBufferInfo.memoryPropertyFlags = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
			PH_CreateBuffer(indexBufferInfo, &indexBuffer);
		}

		// Texture Image
		{
			PH_ImageCreateInfo textureInfo;
			textureInfo.path = "../../Phoenix/App/Test/textures/texture.jpg";
			textureInfo.aspectBits = VK_IMAGE_ASPECT_COLOR_BIT;
			textureInfo.memoryProperty = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
			textureInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
			textureInfo.usageFlags = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
			
			PH_CreateTexture(textureInfo, &texture);
		}

		// sampler
		{
			sampler = PH_CreateSampler();
		}

		//shader module
		{
			PH_CreateShaderModule("../../Phoenix/App/Test/Shaders/SpirV/vert.spv", &vertexShaderModule);
			PH_CreateShaderModule("../../Phoenix/App/Test/Shaders/SpirV/frag.spv", &fragmentShaderModule);
		}
	}

	void Exit()
	{
		// shader modules
		PH_DestroyShaderModule(&vertexShaderModule);
		PH_DestroyShaderModule(&fragmentShaderModule);

		// sampler
		PH_DeleteSampler(&sampler);

		// Texture Image
		PH_DeleteTexture(&texture);

		// Index Buffer
		PH_DeleteBuffer(&indexBuffer);

		// Vertex Buffer
		PH_DeleteBuffer(&vertexBuffer);
	}

	void Load() override
	{
		// Uniform Buffers
		{
			uint32_t maxImages = (uint32_t)swapChainImages.size();
			PH_BufferCreateInfo uniformBufferInfo;
			uniformBufferInfo.bufferSize = (VkDeviceSize)(sizeof(UniformBufferObject));
			uniformBufferInfo.bufferUsageFlags = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;
			uniformBufferInfo.memoryPropertyFlags = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;
			for (uint32_t i = 0; i < maxImages; ++i)
			{
				PH_CreateBuffer(uniformBufferInfo, &uniformBuffers[i]);
			}
		}

		createDescriptorSetLayout();
		createPipeline();
		createDescriptorPool();
		createDescriptorSets();
		RecordCommandBuffers();
	}

	void UnLoad() override
	{
		// cleanup
		PH_DeleteDescriptorSetLayout(&descriptorSetLayout);
		PH_DeleteDescriptorPool(&descriptorPool);
		PH_DeleteGraphicsPipeline(&graphicsPipeline);
		PH_DeletePipelineLayout(&pipelineLayout);

		const int maxImages = (const int)swapChainImages.size();
		for (int i = 0; i < maxImages; ++i)
		{
			PH_DeleteBuffer(&uniformBuffers[i]);
		}
	}

	//void createRenderPass() override {}

	void createDescriptorSetLayout() override
	{
		std::vector<VkDescriptorSetLayoutBinding> bindings;

		VkDescriptorSetLayoutBinding uboLayoutBinding = {};
		uboLayoutBinding.binding = 0;
		uboLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		uboLayoutBinding.descriptorCount = 1;
		uboLayoutBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
		uboLayoutBinding.pImmutableSamplers = nullptr;
		bindings.emplace_back(uboLayoutBinding);

		VkDescriptorSetLayoutBinding samplerLayoutBinding = {};
		samplerLayoutBinding.binding = 1;
		samplerLayoutBinding.descriptorCount = 1;
		samplerLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
		samplerLayoutBinding.pImmutableSamplers = nullptr;
		samplerLayoutBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
		bindings.emplace_back(samplerLayoutBinding);

		VkDescriptorSetLayoutCreateInfo layoutInfo = {};
		layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
		layoutInfo.bindingCount = static_cast<uint32_t>(bindings.size());
		layoutInfo.pBindings = bindings.data();

		PH_CreateDescriptorSetLayout(layoutInfo, &descriptorSetLayout);
	}

	void createPipeline() override
	{
		VkPipelineShaderStageCreateInfo vertShaderStageInfo = {};
		vertShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
		vertShaderStageInfo.stage = VK_SHADER_STAGE_VERTEX_BIT;
		vertShaderStageInfo.module = vertexShaderModule;
		vertShaderStageInfo.pName = "main";

		VkPipelineShaderStageCreateInfo fragShaderStageInfo = {};
		fragShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
		fragShaderStageInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
		fragShaderStageInfo.module = fragmentShaderModule;
		fragShaderStageInfo.pName = "main";

		VkPipelineShaderStageCreateInfo shaderStages[] = { vertShaderStageInfo, fragShaderStageInfo };

		VkPipelineVertexInputStateCreateInfo vertexInputInfo = {};
		vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;

		VkVertexInputBindingDescription bindingDescription = Vertex::getBindingDescription();
		std::vector<VkVertexInputAttributeDescription> attributeDescriptions = Vertex::getAttributeDescriptions();

		vertexInputInfo.vertexBindingDescriptionCount = 1;
		vertexInputInfo.pVertexBindingDescriptions = &bindingDescription;
		vertexInputInfo.vertexAttributeDescriptionCount = static_cast<uint32_t>(attributeDescriptions.size());
		vertexInputInfo.pVertexAttributeDescriptions = attributeDescriptions.data();

		VkPipelineInputAssemblyStateCreateInfo inputAssembly = {};
		inputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
		inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
		inputAssembly.primitiveRestartEnable = VK_FALSE;

		VkViewport viewport = {};
		viewport.x = 0.0f;
		viewport.y = 0.0f;
		viewport.width = (float)swapChainExtent.width;
		viewport.height = (float)swapChainExtent.height;
		viewport.minDepth = 0.0f;
		viewport.maxDepth = 1.0f;

		VkRect2D scissor = {};
		scissor.offset = { 0, 0 };
		scissor.extent = swapChainExtent;

		VkPipelineViewportStateCreateInfo viewportState = {};
		viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
		viewportState.viewportCount = 1;
		viewportState.pViewports = &viewport;
		viewportState.scissorCount = 1;
		viewportState.pScissors = &scissor;

		VkPipelineRasterizationStateCreateInfo rasterizer = {};
		rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
		rasterizer.depthClampEnable = VK_FALSE;
		rasterizer.rasterizerDiscardEnable = VK_FALSE;
		rasterizer.polygonMode = VK_POLYGON_MODE_FILL;
		rasterizer.lineWidth = 1.0f;
		rasterizer.cullMode = VK_CULL_MODE_BACK_BIT;
		rasterizer.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
		rasterizer.depthBiasEnable = VK_FALSE;
		rasterizer.depthBiasConstantFactor = 0.0f; // Optional
		rasterizer.depthBiasClamp = 0.0f; // Optional
		rasterizer.depthBiasSlopeFactor = 0.0f; // Optional

		VkPipelineMultisampleStateCreateInfo multisampling = {};
		multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
		multisampling.sampleShadingEnable = VK_FALSE;
		multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
		multisampling.minSampleShading = 1.0f; // Optional
		multisampling.pSampleMask = nullptr; // Optional
		multisampling.alphaToCoverageEnable = VK_FALSE; // Optional
		multisampling.alphaToOneEnable = VK_FALSE; // Optional

		VkPipelineColorBlendAttachmentState colorBlendAttachment = {};
		colorBlendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
		colorBlendAttachment.blendEnable = VK_FALSE;
		colorBlendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_ONE; // Optional
		colorBlendAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_ZERO; // Optional
		colorBlendAttachment.colorBlendOp = VK_BLEND_OP_ADD; // Optional
		colorBlendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE; // Optional
		colorBlendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO; // Optional
		colorBlendAttachment.alphaBlendOp = VK_BLEND_OP_ADD; // Optional

		VkPipelineColorBlendStateCreateInfo colorBlending = {};
		colorBlending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
		colorBlending.logicOpEnable = VK_FALSE;
		colorBlending.logicOp = VK_LOGIC_OP_COPY; // Optional
		colorBlending.attachmentCount = 1;
		colorBlending.pAttachments = &colorBlendAttachment;
		colorBlending.blendConstants[0] = 0.0f; // Optional
		colorBlending.blendConstants[1] = 0.0f; // Optional
		colorBlending.blendConstants[2] = 0.0f; // Optional
		colorBlending.blendConstants[3] = 0.0f; // Optional

		VkPipelineLayoutCreateInfo pipelineLayoutInfo = {};
		pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
		pipelineLayoutInfo.setLayoutCount = 1; // Optional
		pipelineLayoutInfo.pSetLayouts = &descriptorSetLayout; // Optional
		pipelineLayoutInfo.pushConstantRangeCount = 0; // Optional
		pipelineLayoutInfo.pPushConstantRanges = nullptr; // Optional

		PH_CreatePipelineLayout(pipelineLayoutInfo, &pipelineLayout);

		VkGraphicsPipelineCreateInfo pipelineInfo = {};
		pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
		pipelineInfo.stageCount = 2;
		pipelineInfo.pStages = shaderStages;
		pipelineInfo.pVertexInputState = &vertexInputInfo;
		pipelineInfo.pInputAssemblyState = &inputAssembly;
		pipelineInfo.pViewportState = &viewportState;
		pipelineInfo.pRasterizationState = &rasterizer;
		pipelineInfo.pMultisampleState = &multisampling;
		pipelineInfo.pDepthStencilState = nullptr; // Optional
		pipelineInfo.pColorBlendState = &colorBlending;
		pipelineInfo.pDynamicState = nullptr; // Optional

		VkPipelineDepthStencilStateCreateInfo depthStencil = {};
		depthStencil.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
		depthStencil.depthTestEnable = VK_TRUE;
		depthStencil.depthWriteEnable = VK_TRUE;
		depthStencil.depthCompareOp = VK_COMPARE_OP_LESS;
		depthStencil.depthBoundsTestEnable = VK_FALSE;
		depthStencil.minDepthBounds = 0.0f; // Optional
		depthStencil.maxDepthBounds = 1.0f; // Optional
		depthStencil.stencilTestEnable = VK_FALSE;
		depthStencil.front = {}; // Optional
		depthStencil.back = {}; // Optional
		pipelineInfo.pDepthStencilState = &depthStencil;

		pipelineInfo.layout = pipelineLayout;
		pipelineInfo.renderPass = renderPass;
		pipelineInfo.subpass = 0;
		pipelineInfo.basePipelineHandle = VK_NULL_HANDLE; // Optional
		pipelineInfo.basePipelineIndex = -1; // Optional

		PH_CreateGraphicsPipeline(pipelineInfo, &graphicsPipeline);
	}

	void createDescriptorPool() override
	{
		std::vector<VkDescriptorPoolSize> poolSizes(2);
		poolSizes[0].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		poolSizes[0].descriptorCount = static_cast<uint32_t>(swapChainImages.size());
		poolSizes[1].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
		poolSizes[1].descriptorCount = static_cast<uint32_t>(swapChainImages.size());

		PH_CreateDescriptorPool((uint32_t)poolSizes.size(), poolSizes.data(), (uint32_t)swapChainImages.size(), &descriptorPool);
	}

	void createDescriptorSets() override
	{
		const int maxImages = (const int)swapChainImages.size();

		descriptorSets.resize(maxImages);
		PH_CreateDescriptorSets(descriptorSetLayout, maxImages, descriptorPool, (VkDescriptorSet*)descriptorSets.data());

		for (size_t i = 0; i < maxImages; ++i)
		{
			VkDescriptorBufferInfo bufferInfo = {};
			bufferInfo.buffer = uniformBuffers[i].buffer;
			bufferInfo.offset = 0;
			bufferInfo.range = sizeof(UniformBufferObject); // or VK_WHOLE_SIZE

			VkDescriptorImageInfo imageInfo = {};
			imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
			imageInfo.imageView = texture.imageView;
			imageInfo.sampler = sampler;

			std::vector<VkWriteDescriptorSet> descriptorWrites(2);
			descriptorWrites[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
			descriptorWrites[0].dstSet = descriptorSets[i];
			descriptorWrites[0].dstBinding = 0;
			descriptorWrites[0].dstArrayElement = 0;
			descriptorWrites[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
			descriptorWrites[0].descriptorCount = 1;
			descriptorWrites[0].pBufferInfo = &bufferInfo;
			descriptorWrites[0].pImageInfo = nullptr; // Optional
			descriptorWrites[0].pTexelBufferView = nullptr; // Optional

			descriptorWrites[1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
			descriptorWrites[1].dstSet = descriptorSets[i];
			descriptorWrites[1].dstBinding = 1;
			descriptorWrites[1].dstArrayElement = 0;
			descriptorWrites[1].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
			descriptorWrites[1].descriptorCount = 1;
			descriptorWrites[1].pImageInfo = &imageInfo;

			PH_UpdateDeDescriptorSets(descriptorWrites);
		}
	}

	void RecordCommandBuffers() override
	{
		for (uint32_t i = 0; i < commandBuffers.size(); i++)
		{
			VkCommandBufferBeginInfo beginInfo = {};
			beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
			beginInfo.flags = VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT;
			beginInfo.pInheritanceInfo = nullptr; // Optional

			if (vkBeginCommandBuffer(commandBuffers[i], &beginInfo) != VK_SUCCESS)
			{
				throw std::runtime_error("failed to begin recording command buffer!");
			}

			VkRenderPassBeginInfo renderPassInfo = {};
			renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
			renderPassInfo.renderPass = renderPass;
			renderPassInfo.framebuffer = swapChainFramebuffers[i];
			renderPassInfo.renderArea.offset = { 0, 0 };
			renderPassInfo.renderArea.extent = swapChainExtent;

			std::vector<VkClearValue> clearValues;
			VkClearValue clearValue;
			clearValue.color = { 0.0f, 0.0f, 0.0f, 1.0f };
			clearValues.emplace_back(clearValue);

			VkClearValue depthValue;
			depthValue.depthStencil = { 1.0f, 0 };
			clearValues.emplace_back(depthValue);

			renderPassInfo.clearValueCount = static_cast<uint32_t>(clearValues.size());
			renderPassInfo.pClearValues = clearValues.data();

			vkCmdBeginRenderPass(commandBuffers[i], &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);
			vkCmdBindPipeline(commandBuffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, graphicsPipeline);

			// vertex and index buffers
			VkBuffer vertexBuffers[] = { vertexBuffer.buffer };
			VkDeviceSize offsets[] = { 0 };
			vkCmdBindVertexBuffers(commandBuffers[i], 0, 1, vertexBuffers, offsets);
			vkCmdBindIndexBuffer(commandBuffers[i], indexBuffer.buffer, 0, VK_INDEX_TYPE_UINT16);

			// descriptor sets - uniform buffer
			vkCmdBindDescriptorSets(commandBuffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout, 0, 1, &descriptorSets[i], 0, nullptr);

			vkCmdDrawIndexed(commandBuffers[i], static_cast<uint32_t>(indices.size()), 1, 0, 0, 0);
			vkCmdEndRenderPass(commandBuffers[i]);

			if (vkEndCommandBuffer(commandBuffers[i]) != VK_SUCCESS)
			{
				throw std::runtime_error("failed to record command buffer!");
			}
		}
	}

	void UpdateUniformBuffer(uint32_t imageIndex)
	{
		static std::chrono::time_point<std::chrono::steady_clock>	startTime = std::chrono::high_resolution_clock::now();
		std::chrono::time_point<std::chrono::steady_clock>			currentTime = std::chrono::high_resolution_clock::now();
		float time = std::chrono::duration<float, std::chrono::seconds::period>(currentTime - startTime).count();

		UniformBufferObject ubo = {};

		ubo.model = glm::rotate(glm::mat4(1.0f), glm::radians(-90.0f), glm::vec3(1.0f, 0.0f, 0.0f));
		ubo.model = glm::rotate(ubo.model, time * glm::radians(90.0f), glm::vec3(0.0f, 0.0f, 1.0f));
		ubo.model = glm::scale(ubo.model, glm::vec3(2.0f));
		ubo.proj = camera.matrices.perspective;
		ubo.view = camera.matrices.view;

		PH_BufferUpdateInfo bufferUpdate;
		bufferUpdate.buffer = uniformBuffers[imageIndex];
		bufferUpdate.data = &ubo;
		bufferUpdate.dataSize = sizeof(UniformBufferObject);

		PH_UpdateBuffer(bufferUpdate);
	}

	bool UpdateCamera()
	{
		// CAMERA UPDATE
		float xpos = (float)pWindow->mouseX();
		float ypos = (float)pWindow->mouseY();
		if (firstMouse)
		{
			lastX = xpos;
			lastY = ypos;
			firstMouse = false;
		}

		float xoffset = xpos - lastX;
		float yoffset = lastY - ypos;

		lastX = xpos;
		lastY = ypos;

		camera.keys.up = false;
		camera.keys.down = false;
		camera.keys.left = false;
		camera.keys.right = false;

		if (pWindow->isRightClicked())
		{
			if (pWindow->isKeyPressed(PH_KEY_W))
			{
				camera.keys.up = true;
			}
			if (pWindow->isKeyPressed(PH_KEY_S))
			{
				camera.keys.down = true;
			}
			if (pWindow->isKeyPressed(PH_KEY_A))
			{
				camera.keys.left = true;
			}
			if (pWindow->isKeyPressed(PH_KEY_D))
			{
				camera.keys.right = true;
			}

			camera.rotate(glm::vec3(yoffset * 0.5f, xoffset * 0.5f, 0.0f));
			camera.update(0.016f);

			return true;
		}

		return false;
	}

	void DrawFrame() override
	{
		uint32_t imageIndex = PH_PrepareNextFrame();

		if (imageIndex != -1) // invalid
		{
			UpdateCamera();
			UpdateUniformBuffer(imageIndex);
			
			PH_SubmitFrame();
		}
	}
};

PHOENIX_MAIN(Application)


#endif