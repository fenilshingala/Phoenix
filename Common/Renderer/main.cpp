#include "VulkanRenderer.h"

#include <chrono>
#include <vector>
#include <array>
#include <iostream>
#include <algorithm>

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

struct UniformBufferObject
{
	alignas(16) glm::mat4 view;
	alignas(16) glm::mat4 proj;
} ubo;

struct Vertex
{
	float pos[3];
};

#pragma region RT_HELPER_STRUCTURES

// Ray tracing acceleration structure
struct AccelerationStructure {
	VkDeviceMemory memory;
	VkAccelerationStructureNV accelerationStructure;
	uint64_t handle;
};

// Ray tracing geometry instance
struct GeometryInstance {
	glm::mat3x4 transform;
	uint32_t instanceId : 24;
	uint32_t mask : 8;
	uint32_t instanceOffset : 24;
	uint32_t flags : 8;
	uint64_t accelerationStructureHandle;
};

// Indices for the different ray tracing shader types used in this example
#define INDEX_RAYGEN 0
#define INDEX_MISS 1
#define INDEX_CLOSEST_HIT 2

#pragma endregion


class Application : public VulkanRenderer
{
	std::vector<Vertex> vertices;
	std::vector<uint16_t> indices;

	PH_Image texture;
	VkSampler sampler;
	//VkShaderModule vertexShaderModule;
	//VkShaderModule fragmentShaderModule;
	VkShaderModule RayGenModule;
	VkShaderModule MissModule;
	VkShaderModule ClosestHitModule;

	PH_Buffer vertexBuffer;
	PH_Buffer indexBuffer;
	PH_Buffer uniformBuffer;
	VkPipelineLayout pipelineLayout;
	VkDescriptorSetLayout descriptorSetLayout;
	VkDescriptorSet descriptorSet;
	//VkPipeline graphicsPipeline;

#pragma region RT_MEMBER_VARIABLES

	PFN_vkCreateAccelerationStructureNV vkCreateAccelerationStructureNV;
	PFN_vkDestroyAccelerationStructureNV vkDestroyAccelerationStructureNV;
	PFN_vkBindAccelerationStructureMemoryNV vkBindAccelerationStructureMemoryNV;
	PFN_vkGetAccelerationStructureHandleNV vkGetAccelerationStructureHandleNV;
	PFN_vkGetAccelerationStructureMemoryRequirementsNV vkGetAccelerationStructureMemoryRequirementsNV;
	PFN_vkCmdBuildAccelerationStructureNV vkCmdBuildAccelerationStructureNV;
	PFN_vkCreateRayTracingPipelinesNV vkCreateRayTracingPipelinesNV;
	PFN_vkGetRayTracingShaderGroupHandlesNV vkGetRayTracingShaderGroupHandlesNV;
	PFN_vkCmdTraceRaysNV vkCmdTraceRaysNV;

	VkPhysicalDeviceRayTracingPropertiesNV rayTracingProperties{};

	AccelerationStructure bottomLevelAS;
	AccelerationStructure topLevelAS;

	uint32_t indexCount;
	PH_Buffer shaderBindingTable;
	PH_Image storageImage;

	const uint32_t shaderIndexRaygen = 0;
	const uint32_t shaderIndexMiss = 1;
	const uint32_t shaderIndexClosestHit = 2;
	
	VkPipeline rtPipeline;

#pragma endregion

#pragma region RT_HELPER_FUNCTIONS

	/*
		The bottom level acceleration structure contains the scene's geometry (vertices, triangles)
	*/
	void createBottomLevelAccelerationStructure(const VkGeometryNV* geometries)
	{
		VkAccelerationStructureInfoNV accelerationStructureInfo{};
		accelerationStructureInfo.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_INFO_NV;
		accelerationStructureInfo.type = VK_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL_NV;
		accelerationStructureInfo.instanceCount = 0;
		accelerationStructureInfo.geometryCount = 1;
		accelerationStructureInfo.pGeometries = geometries;

		VkAccelerationStructureCreateInfoNV accelerationStructureCI{};
		accelerationStructureCI.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_CREATE_INFO_NV;
		accelerationStructureCI.info = accelerationStructureInfo;
		VkResult result = vkCreateAccelerationStructureNV(device, &accelerationStructureCI, nullptr, &bottomLevelAS.accelerationStructure);
		if (result != VK_SUCCESS)
		{
			throw std::runtime_error("failed to set create Acceleration Structure!");
		}

		VkAccelerationStructureMemoryRequirementsInfoNV memoryRequirementsInfo{};
		memoryRequirementsInfo.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_MEMORY_REQUIREMENTS_INFO_NV;
		memoryRequirementsInfo.type = VK_ACCELERATION_STRUCTURE_MEMORY_REQUIREMENTS_TYPE_OBJECT_NV;
		memoryRequirementsInfo.accelerationStructure = bottomLevelAS.accelerationStructure;

		VkMemoryRequirements2 memoryRequirements2{};
		vkGetAccelerationStructureMemoryRequirementsNV(device, &memoryRequirementsInfo, &memoryRequirements2);

		VkMemoryAllocateInfo memoryAllocateInfo{};
		memoryAllocateInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
		memoryAllocateInfo.allocationSize = memoryRequirements2.memoryRequirements.size;
		memoryAllocateInfo.memoryTypeIndex = findMemoryType(physicalDevice, memoryRequirements2.memoryRequirements.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
		result = vkAllocateMemory(device, &memoryAllocateInfo, nullptr, &bottomLevelAS.memory);
		if (result != VK_SUCCESS)
		{
			throw std::runtime_error("failed to allocate memory for BLAS!");
		}

		VkBindAccelerationStructureMemoryInfoNV accelerationStructureMemoryInfo{};
		accelerationStructureMemoryInfo.sType = VK_STRUCTURE_TYPE_BIND_ACCELERATION_STRUCTURE_MEMORY_INFO_NV;
		accelerationStructureMemoryInfo.accelerationStructure = bottomLevelAS.accelerationStructure;
		accelerationStructureMemoryInfo.memory = bottomLevelAS.memory;
		result = vkBindAccelerationStructureMemoryNV(device, 1, &accelerationStructureMemoryInfo);
		if (result != VK_SUCCESS)
		{
			throw std::runtime_error("failed to bind BLAS!");
		}

		result = vkGetAccelerationStructureHandleNV(device, bottomLevelAS.accelerationStructure, sizeof(uint64_t), &bottomLevelAS.handle);
		if (result != VK_SUCCESS)
		{
			throw std::runtime_error("failed to get BLAS handle!");
		}
	}

	/*
		The top level acceleration structure contains the scene's object instances
	*/
	void createTopLevelAccelerationStructure()
	{
		VkAccelerationStructureInfoNV accelerationStructureInfo{};
		accelerationStructureInfo.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_INFO_NV;
		accelerationStructureInfo.type = VK_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL_NV;
		accelerationStructureInfo.instanceCount = 1;
		accelerationStructureInfo.geometryCount = 0;

		VkAccelerationStructureCreateInfoNV accelerationStructureCI{};
		accelerationStructureCI.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_CREATE_INFO_NV;
		accelerationStructureCI.info = accelerationStructureInfo;
		VkResult result = vkCreateAccelerationStructureNV(device, &accelerationStructureCI, nullptr, &topLevelAS.accelerationStructure);

		VkAccelerationStructureMemoryRequirementsInfoNV memoryRequirementsInfo{};
		memoryRequirementsInfo.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_MEMORY_REQUIREMENTS_INFO_NV;
		memoryRequirementsInfo.type = VK_ACCELERATION_STRUCTURE_MEMORY_REQUIREMENTS_TYPE_OBJECT_NV;
		memoryRequirementsInfo.accelerationStructure = topLevelAS.accelerationStructure;

		VkMemoryRequirements2 memoryRequirements2{};
		vkGetAccelerationStructureMemoryRequirementsNV(device, &memoryRequirementsInfo, &memoryRequirements2);

		VkMemoryAllocateInfo memoryAllocateInfo{};
		memoryAllocateInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
		memoryAllocateInfo.allocationSize = memoryRequirements2.memoryRequirements.size;
		memoryAllocateInfo.memoryTypeIndex = findMemoryType(physicalDevice, memoryRequirements2.memoryRequirements.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
		result = vkAllocateMemory(device, &memoryAllocateInfo, nullptr, &topLevelAS.memory);
		if (result != VK_SUCCESS)
		{
			throw std::runtime_error("failed to allocate memory for TLAS!");
		}

		VkBindAccelerationStructureMemoryInfoNV accelerationStructureMemoryInfo{};
		accelerationStructureMemoryInfo.sType = VK_STRUCTURE_TYPE_BIND_ACCELERATION_STRUCTURE_MEMORY_INFO_NV;
		accelerationStructureMemoryInfo.accelerationStructure = topLevelAS.accelerationStructure;
		accelerationStructureMemoryInfo.memory = topLevelAS.memory;
		result = vkBindAccelerationStructureMemoryNV(device, 1, &accelerationStructureMemoryInfo);
		if (result != VK_SUCCESS)
		{
			throw std::runtime_error("failed to bind TLAS!");
		}

		result = vkGetAccelerationStructureHandleNV(device, topLevelAS.accelerationStructure, sizeof(uint64_t), &topLevelAS.handle);
		if (result != VK_SUCCESS)
		{
			throw std::runtime_error("failed to get TLAS handle!");
		}
	}

#pragma endregion

public:
	Application()
	{}

	~Application()
	{}

	void Init()
	{
		vertices = {
			{ {  1.0f,  1.0f, 0.0f } },
			{ { -1.0f,  1.0f, 0.0f } },
			{ {  0.0f, -1.0f, 0.0f } }
		};

		indices = { 0, 1, 2 };

		// Vertex Buffer
		{
			PH_BufferCreateInfo vertexBufferInfo;
			vertexBufferInfo.data = vertices.data();
			vertexBufferInfo.bufferSize = (VkDeviceSize)(sizeof(vertices[0]) * vertices.size());
			vertexBufferInfo.bufferUsageFlags = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
			vertexBufferInfo.memoryPropertyFlags = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;
			PH_CreateBuffer(vertexBufferInfo, vertexBuffer);
		}

		// Index Buffer
		{
			PH_BufferCreateInfo indexBufferInfo;
			indexBufferInfo.data = indices.data();
			indexBufferInfo.bufferSize = (VkDeviceSize)(sizeof(indices[0]) * indices.size());
			indexBufferInfo.bufferUsageFlags = VK_BUFFER_USAGE_INDEX_BUFFER_BIT;
			indexBufferInfo.memoryPropertyFlags = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;
			PH_CreateBuffer(indexBufferInfo, indexBuffer);
		}

		// Shader Modules
		{
			RayGenModule = PH_CreateShaderModule("../../Phoenix/App/Test/Shaders/SpirV/raygen.rgen.spv");
			MissModule = PH_CreateShaderModule("../../Phoenix/App/Test/Shaders/SpirV/miss.rmiss.spv");
			ClosestHitModule = PH_CreateShaderModule("../../Phoenix/App/Test/Shaders/SpirV/closesthit.rchit.spv");
		}

#pragma region RT_INIT_createScene

		// Query the ray tracing properties of the current implementation, we will need them later on
		rayTracingProperties.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_RAY_TRACING_PROPERTIES_NV;
		VkPhysicalDeviceProperties2 deviceProps2{};
		deviceProps2.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2;
		deviceProps2.pNext = &rayTracingProperties;
		vkGetPhysicalDeviceProperties2(physicalDevice, &deviceProps2);

		// Get VK_NV_ray_tracing related function pointers
		vkCreateAccelerationStructureNV = reinterpret_cast<PFN_vkCreateAccelerationStructureNV>(vkGetDeviceProcAddr(device, "vkCreateAccelerationStructureNV"));
		vkDestroyAccelerationStructureNV = reinterpret_cast<PFN_vkDestroyAccelerationStructureNV>(vkGetDeviceProcAddr(device, "vkDestroyAccelerationStructureNV"));
		vkBindAccelerationStructureMemoryNV = reinterpret_cast<PFN_vkBindAccelerationStructureMemoryNV>(vkGetDeviceProcAddr(device, "vkBindAccelerationStructureMemoryNV"));
		vkGetAccelerationStructureHandleNV = reinterpret_cast<PFN_vkGetAccelerationStructureHandleNV>(vkGetDeviceProcAddr(device, "vkGetAccelerationStructureHandleNV"));
		vkGetAccelerationStructureMemoryRequirementsNV = reinterpret_cast<PFN_vkGetAccelerationStructureMemoryRequirementsNV>(vkGetDeviceProcAddr(device, "vkGetAccelerationStructureMemoryRequirementsNV"));
		vkCmdBuildAccelerationStructureNV = reinterpret_cast<PFN_vkCmdBuildAccelerationStructureNV>(vkGetDeviceProcAddr(device, "vkCmdBuildAccelerationStructureNV"));
		vkCreateRayTracingPipelinesNV = reinterpret_cast<PFN_vkCreateRayTracingPipelinesNV>(vkGetDeviceProcAddr(device, "vkCreateRayTracingPipelinesNV"));
		vkGetRayTracingShaderGroupHandlesNV = reinterpret_cast<PFN_vkGetRayTracingShaderGroupHandlesNV>(vkGetDeviceProcAddr(device, "vkGetRayTracingShaderGroupHandlesNV"));
		vkCmdTraceRaysNV = reinterpret_cast<PFN_vkCmdTraceRaysNV>(vkGetDeviceProcAddr(device, "vkCmdTraceRaysNV"));

		/*
			Create the bottom level acceleration structure containing the actual scene geometry
		*/
		VkGeometryNV geometry{};
		geometry.sType = VK_STRUCTURE_TYPE_GEOMETRY_NV;
		geometry.geometryType = VK_GEOMETRY_TYPE_TRIANGLES_NV;
		geometry.geometry.triangles.sType = VK_STRUCTURE_TYPE_GEOMETRY_TRIANGLES_NV;
		geometry.geometry.triangles.vertexData = vertexBuffer.buffer;
		geometry.geometry.triangles.vertexOffset = 0;
		geometry.geometry.triangles.vertexCount = static_cast<uint32_t>(vertices.size());
		geometry.geometry.triangles.vertexStride = sizeof(Vertex);
		geometry.geometry.triangles.vertexFormat = VK_FORMAT_R32G32B32_SFLOAT;
		geometry.geometry.triangles.indexData = indexBuffer.buffer;
		geometry.geometry.triangles.indexOffset = 0;
		geometry.geometry.triangles.indexCount = indexCount;
		geometry.geometry.triangles.indexType = VK_INDEX_TYPE_UINT16;
		geometry.geometry.triangles.transformData = VK_NULL_HANDLE;
		geometry.geometry.triangles.transformOffset = 0;
		geometry.geometry.aabbs = {};
		geometry.geometry.aabbs.sType = { VK_STRUCTURE_TYPE_GEOMETRY_AABB_NV };
		geometry.flags = VK_GEOMETRY_OPAQUE_BIT_NV;

		createBottomLevelAccelerationStructure(&geometry);

		/*
			Create the top-level acceleration structure that contains geometry instance information
		*/

		// Single instance with a 3x4 transform matrix for the ray traced triangle
		PH_Buffer instanceBuffer;

		glm::mat3x4 transform = {
			1.0f, 0.0f, 0.0f, 0.0f,
			0.0f, 1.0f, 0.0f, 0.0f,
			0.0f, 0.0f, 1.0f, 0.0f,
		};

		GeometryInstance geometryInstance{};
		geometryInstance.transform = transform;
		geometryInstance.instanceId = 0;
		geometryInstance.mask = 0xff;
		geometryInstance.instanceOffset = 0;
		geometryInstance.flags = VK_GEOMETRY_INSTANCE_TRIANGLE_CULL_DISABLE_BIT_NV;
		geometryInstance.accelerationStructureHandle = bottomLevelAS.handle;

		PH_BufferCreateInfo instanceBufferCreateInfo;
		instanceBufferCreateInfo.bufferSize = sizeof(GeometryInstance);
		instanceBufferCreateInfo.bufferUsageFlags = VK_BUFFER_USAGE_RAY_TRACING_BIT_NV;
		instanceBufferCreateInfo.memoryPropertyFlags = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;
		instanceBufferCreateInfo.data = &geometryInstance;

		PH_CreateBuffer(instanceBufferCreateInfo, instanceBuffer);

		createTopLevelAccelerationStructure();

		/*
			Build the acceleration structure
		*/

		// Acceleration structure build requires some scratch space to store temporary information
		VkAccelerationStructureMemoryRequirementsInfoNV memoryRequirementsInfo{};
		memoryRequirementsInfo.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_MEMORY_REQUIREMENTS_INFO_NV;
		memoryRequirementsInfo.type = VK_ACCELERATION_STRUCTURE_MEMORY_REQUIREMENTS_TYPE_BUILD_SCRATCH_NV;

		VkMemoryRequirements2 memReqBottomLevelAS;
		memoryRequirementsInfo.accelerationStructure = bottomLevelAS.accelerationStructure;
		vkGetAccelerationStructureMemoryRequirementsNV(device, &memoryRequirementsInfo, &memReqBottomLevelAS);

		VkMemoryRequirements2 memReqTopLevelAS;
		memoryRequirementsInfo.accelerationStructure = topLevelAS.accelerationStructure;
		vkGetAccelerationStructureMemoryRequirementsNV(device, &memoryRequirementsInfo, &memReqTopLevelAS);

		const VkDeviceSize scratchBufferSize = std::max(memReqBottomLevelAS.memoryRequirements.size, memReqTopLevelAS.memoryRequirements.size);

		PH_Buffer scratchBuffer;
		PH_BufferCreateInfo scratchBufferCreateInfo;
		scratchBufferCreateInfo.bufferSize = scratchBufferSize;
		scratchBufferCreateInfo.bufferUsageFlags = VK_BUFFER_USAGE_RAY_TRACING_BIT_NV;
		scratchBufferCreateInfo.memoryPropertyFlags = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
		PH_CreateBuffer(scratchBufferCreateInfo, scratchBuffer);

		//VkCommandBuffer cmdBuffer = vulkanDevice->createCommandBuffer(VK_COMMAND_BUFFER_LEVEL_PRIMARY, true);
		VkCommandBuffer cmdBuffer = beginSingleTimeCommands();

		/*
			Build bottom level acceleration structure
		*/
		VkAccelerationStructureInfoNV buildInfo{};
		buildInfo.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_INFO_NV;
		buildInfo.type = VK_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL_NV;
		buildInfo.geometryCount = 1;
		buildInfo.pGeometries = &geometry;

		vkCmdBuildAccelerationStructureNV(
			cmdBuffer,
			&buildInfo,
			VK_NULL_HANDLE,
			0,
			VK_FALSE,
			bottomLevelAS.accelerationStructure,
			VK_NULL_HANDLE,
			scratchBuffer.buffer,
			0);

		VkMemoryBarrier memoryBarrier{};
		memoryBarrier.sType = VK_STRUCTURE_TYPE_MEMORY_BARRIER;
		memoryBarrier.srcAccessMask = VK_ACCESS_ACCELERATION_STRUCTURE_WRITE_BIT_NV | VK_ACCESS_ACCELERATION_STRUCTURE_READ_BIT_NV;
		memoryBarrier.dstAccessMask = VK_ACCESS_ACCELERATION_STRUCTURE_WRITE_BIT_NV | VK_ACCESS_ACCELERATION_STRUCTURE_READ_BIT_NV;
		vkCmdPipelineBarrier(cmdBuffer, VK_PIPELINE_STAGE_ACCELERATION_STRUCTURE_BUILD_BIT_NV, VK_PIPELINE_STAGE_ACCELERATION_STRUCTURE_BUILD_BIT_NV, 0, 1, &memoryBarrier, 0, 0, 0, 0);

		/*
			Build top-level acceleration structure
		*/
		buildInfo.type = VK_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL_NV;
		buildInfo.pGeometries = 0;
		buildInfo.geometryCount = 0;
		buildInfo.instanceCount = 1;

		vkCmdBuildAccelerationStructureNV(
			cmdBuffer,
			&buildInfo,
			instanceBuffer.buffer,
			0,
			VK_FALSE,
			topLevelAS.accelerationStructure,
			VK_NULL_HANDLE,
			scratchBuffer.buffer,
			0);

		vkCmdPipelineBarrier(cmdBuffer, VK_PIPELINE_STAGE_ACCELERATION_STRUCTURE_BUILD_BIT_NV, VK_PIPELINE_STAGE_ACCELERATION_STRUCTURE_BUILD_BIT_NV, 0, 1, &memoryBarrier, 0, 0, 0, 0);

		endSingleTimeCommands(cmdBuffer);

		PH_DeleteBuffer(scratchBuffer);
		PH_DeleteBuffer(instanceBuffer);

#pragma endregion

	}

	void Exit()
	{
		// shader modules
		PH_DestroyShaderModule(ClosestHitModule);
		PH_DestroyShaderModule(MissModule);
		PH_DestroyShaderModule(RayGenModule);

		// sampler
		PH_DeleteSampler(sampler);

		// Texture Image
		PH_DeleteTexture(texture);

		// Index Buffer
		PH_DeleteBuffer(indexBuffer);

		// Vertex Buffer
		PH_DeleteBuffer(vertexBuffer);
	}

	void Load() override
	{
#pragma region createStorageImage
		VkExtent2D swapChainExtent = GetSwapChainExtent();

		PH_ImageCreateInfo storageImageInfo;
		storageImageInfo.width = swapChainExtent.width;
		storageImageInfo.height = swapChainExtent.height;
		storageImageInfo.format = GetSwapChainImageFormat();
		storageImageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
		storageImageInfo.usageFlags = VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_STORAGE_BIT;
		storageImageInfo.memoryProperty = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
		storageImageInfo.aspectBits = VK_IMAGE_ASPECT_COLOR_BIT;

		PH_CreateTexture(storageImageInfo, storageImage);
#pragma endregion

		// Uniform Buffers
		{
			PH_BufferCreateInfo uniformBufferInfo;
			uniformBufferInfo.bufferSize = (VkDeviceSize)(sizeof(UniformBufferObject));
			uniformBufferInfo.bufferUsageFlags = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;	
			uniformBufferInfo.memoryPropertyFlags = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;
			PH_CreateBuffer(uniformBufferInfo, uniformBuffer);
		}

		createDescriptorSetLayout();
		createGraphicsPipeline();
		createShaderBindingTable();
		createDescriptorSets();
		RecordCommandBuffers();
	}

	void UnLoad() override
	{
		// cleanup
		PH_DeleteDescriptorSetLayout(descriptorSetLayout);
		PH_DeletePipelineLayout(pipelineLayout);
		//PH_DeleteGraphicsPipeline(graphicsPipeline);

		/*const int maxImages = GetNoOfSwapChains();
		for (int i = 0; i < maxImages; ++i)
		{*/
			PH_DeleteBuffer(uniformBuffer);
		//}
	}

	//void createRenderPass() override {}

	void createDescriptorPool() override
	{
		std::vector<VkDescriptorPoolSize> poolSizes = {
			{ VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_NV, 1 },
			{ VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 1 },
			{ VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1 }
		};

		VkDescriptorPoolCreateInfo descriptorPoolInfo{};
		descriptorPoolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
		descriptorPoolInfo.poolSizeCount = static_cast<uint32_t>(poolSizes.size());
		descriptorPoolInfo.pPoolSizes = poolSizes.data();
		descriptorPoolInfo.maxSets = 1;

		if (vkCreateDescriptorPool(device, &descriptorPoolInfo, nullptr, &descriptorPool) != VK_SUCCESS)
		{
			throw std::runtime_error("failed to create descriptor pool!");
		}
	}

	void createDescriptorSetLayout() override
	{
		VkDescriptorSetLayoutBinding accelerationStructureLayoutBinding{};
		accelerationStructureLayoutBinding.binding = 0;
		accelerationStructureLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_NV;
		accelerationStructureLayoutBinding.descriptorCount = 1;
		accelerationStructureLayoutBinding.stageFlags = VK_SHADER_STAGE_RAYGEN_BIT_NV;

		VkDescriptorSetLayoutBinding resultImageLayoutBinding{};
		resultImageLayoutBinding.binding = 1;
		resultImageLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
		resultImageLayoutBinding.descriptorCount = 1;
		resultImageLayoutBinding.stageFlags = VK_SHADER_STAGE_RAYGEN_BIT_NV;

		VkDescriptorSetLayoutBinding uniformBufferBinding{};
		uniformBufferBinding.binding = 2;
		uniformBufferBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		uniformBufferBinding.descriptorCount = 1;
		uniformBufferBinding.stageFlags = VK_SHADER_STAGE_RAYGEN_BIT_NV;

		std::vector<VkDescriptorSetLayoutBinding> bindings({
			accelerationStructureLayoutBinding,
			resultImageLayoutBinding,
			uniformBufferBinding
			});

		VkDescriptorSetLayoutCreateInfo layoutInfo{};
		layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
		layoutInfo.bindingCount = static_cast<uint32_t>(bindings.size());
		layoutInfo.pBindings = bindings.data();
		
		PH_CreateDescriptorSetLayout(layoutInfo, descriptorSetLayout);
	}

	void createGraphicsPipeline() override
	{
		VkPipelineLayoutCreateInfo pipelineLayoutCreateInfo{};
		pipelineLayoutCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
		pipelineLayoutCreateInfo.setLayoutCount = 1;
		pipelineLayoutCreateInfo.pSetLayouts = &descriptorSetLayout;

		PH_CreatePipelineLayout(pipelineLayoutCreateInfo, pipelineLayout);

		VkPipelineShaderStageCreateInfo raygenInfo;
		raygenInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
		raygenInfo.stage = VK_SHADER_STAGE_RAYGEN_BIT_NV;
		raygenInfo.module = RayGenModule;
		raygenInfo.pName = "main";

		VkPipelineShaderStageCreateInfo missInfo;
		missInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
		missInfo.stage = VK_SHADER_STAGE_MISS_BIT_NV;
		missInfo.module = MissModule;
		missInfo.pName = "main";

		VkPipelineShaderStageCreateInfo closestHitInfo;
		closestHitInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
		closestHitInfo.stage = VK_SHADER_STAGE_CLOSEST_HIT_BIT_NV;
		closestHitInfo.module = ClosestHitModule;
		closestHitInfo.pName = "main";

		std::array<VkPipelineShaderStageCreateInfo, 3> shaderStages;
		shaderStages[shaderIndexRaygen] = raygenInfo;
		shaderStages[shaderIndexMiss] = missInfo;
		shaderStages[shaderIndexClosestHit] = closestHitInfo;

		/*
			Setup ray tracing shader groups
		*/
		std::array<VkRayTracingShaderGroupCreateInfoNV, 3> groups{};
		for (VkRayTracingShaderGroupCreateInfoNV& group : groups) {
			// Init all groups with some default values
			group.sType = VK_STRUCTURE_TYPE_RAY_TRACING_SHADER_GROUP_CREATE_INFO_NV;
			group.generalShader = VK_SHADER_UNUSED_NV;
			group.closestHitShader = VK_SHADER_UNUSED_NV;
			group.anyHitShader = VK_SHADER_UNUSED_NV;
			group.intersectionShader = VK_SHADER_UNUSED_NV;
		}

		// Links shaders and types to ray tracing shader groups
		groups[INDEX_RAYGEN].type = VK_RAY_TRACING_SHADER_GROUP_TYPE_GENERAL_NV;
		groups[INDEX_RAYGEN].generalShader = shaderIndexRaygen;
		groups[INDEX_MISS].type = VK_RAY_TRACING_SHADER_GROUP_TYPE_GENERAL_NV;
		groups[INDEX_MISS].generalShader = shaderIndexMiss;
		groups[INDEX_CLOSEST_HIT].type = VK_RAY_TRACING_SHADER_GROUP_TYPE_TRIANGLES_HIT_GROUP_NV;
		groups[INDEX_CLOSEST_HIT].generalShader = VK_SHADER_UNUSED_NV;
		groups[INDEX_CLOSEST_HIT].closestHitShader = shaderIndexClosestHit;

		VkRayTracingPipelineCreateInfoNV rayPipelineInfo{};
		rayPipelineInfo.sType = VK_STRUCTURE_TYPE_RAY_TRACING_PIPELINE_CREATE_INFO_NV;
		rayPipelineInfo.stageCount = static_cast<uint32_t>(shaderStages.size());
		rayPipelineInfo.pStages = shaderStages.data();
		rayPipelineInfo.groupCount = static_cast<uint32_t>(groups.size());
		rayPipelineInfo.pGroups = groups.data();
		rayPipelineInfo.maxRecursionDepth = 1;
		rayPipelineInfo.layout = pipelineLayout;
		rayPipelineInfo.basePipelineHandle = VK_NULL_HANDLE;
		rayPipelineInfo.basePipelineIndex = 0;
		rayPipelineInfo.flags = 0;
		rayPipelineInfo.pNext = nullptr;

		VkResult result = vkCreateRayTracingPipelinesNV(device, VK_NULL_HANDLE, 1, &rayPipelineInfo, nullptr, &rtPipeline);
		if (result != VK_SUCCESS)
		{
			std::cerr << "Ray Tracing Pipeline creation error!";
		}
	}

	void createDescriptorSets() override
	{
		VkDescriptorSetAllocateInfo allocInfo = {};
		allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
		allocInfo.descriptorPool = descriptorPool;
		allocInfo.descriptorSetCount = 1;
		allocInfo.pSetLayouts = &descriptorSetLayout;

		if (vkAllocateDescriptorSets(device, &allocInfo, &descriptorSet) != VK_SUCCESS)
		{
			throw std::runtime_error("failed to allocate descriptor sets!");
		}

		VkWriteDescriptorSetAccelerationStructureNV descriptorAccelerationStructureInfo{};
		descriptorAccelerationStructureInfo.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET_ACCELERATION_STRUCTURE_NV;
		descriptorAccelerationStructureInfo.accelerationStructureCount = 1;
		descriptorAccelerationStructureInfo.pAccelerationStructures = &topLevelAS.accelerationStructure;

		VkWriteDescriptorSet accelerationStructureWrite{};
		accelerationStructureWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		// The specialized acceleration structure descriptor has to be chained
		accelerationStructureWrite.pNext = &descriptorAccelerationStructureInfo;
		accelerationStructureWrite.dstSet = descriptorSet;
		accelerationStructureWrite.dstBinding = 0;
		accelerationStructureWrite.descriptorCount = 1;
		accelerationStructureWrite.descriptorType = VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_NV;

		VkDescriptorImageInfo storageImageDescriptor{};
		storageImageDescriptor.imageView = storageImage.imageView;
		storageImageDescriptor.imageLayout = VK_IMAGE_LAYOUT_GENERAL;

		VkWriteDescriptorSet resultImageWrite{};
		resultImageWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		resultImageWrite.dstSet = descriptorSet;
		resultImageWrite.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
		resultImageWrite.dstBinding = 1;
		resultImageWrite.pImageInfo = &storageImageDescriptor;
		resultImageWrite.descriptorCount = 1;

		VkDescriptorBufferInfo bufferInfo;
		bufferInfo.buffer = uniformBuffer.buffer;
		bufferInfo.offset = 0;
		bufferInfo.range = sizeof(UniformBufferObject);

		VkWriteDescriptorSet uniformBufferWrite{};
		uniformBufferWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		uniformBufferWrite.dstSet = descriptorSet;
		uniformBufferWrite.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		uniformBufferWrite.dstBinding = 2;
		uniformBufferWrite.pBufferInfo = &bufferInfo;
		uniformBufferWrite.descriptorCount = 1;

		std::vector<VkWriteDescriptorSet> writeDescriptorSets = {
			accelerationStructureWrite,
			resultImageWrite,
			uniformBufferWrite
		};
		
		PH_UpdateDeDescriptorSets(writeDescriptorSets);
	}

	void UpdateUniformBuffer(uint32_t imageIndex)
	{
		const VkExtent2D swapChainExtent = GetSwapChainExtent();

		static std::chrono::time_point<std::chrono::steady_clock>	startTime   = std::chrono::high_resolution_clock::now();
		std::chrono::time_point<std::chrono::steady_clock>			currentTime = std::chrono::high_resolution_clock::now();
		float time = std::chrono::duration<float, std::chrono::seconds::period>(currentTime - startTime).count();

		UniformBufferObject ubo = {};
		ubo.view = glm::lookAt(glm::vec3(2.0f, 2.0f, 2.0f), glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 0.0f, 1.0f));
		ubo.proj = glm::perspective(glm::radians(45.0f), swapChainExtent.width / (float)swapChainExtent.height, 0.1f, 10.0f);
		ubo.proj[1][1] *= -1;

		PH_BufferUpdateInfo bufferUpdate;
		bufferUpdate.buffer		= uniformBuffer;
		bufferUpdate.data		= &ubo;
		bufferUpdate.dataSize	= sizeof(UniformBufferObject);

		PH_UpdateBuffer(bufferUpdate);
	}

	VkDeviceSize copyShaderIdentifier(uint8_t* data, const uint8_t* shaderHandleStorage, uint32_t groupIndex)
	{
		const uint32_t shaderGroupHandleSize = rayTracingProperties.shaderGroupHandleSize;
		memcpy(data, shaderHandleStorage + groupIndex * shaderGroupHandleSize, shaderGroupHandleSize);
		data += shaderGroupHandleSize;
		return shaderGroupHandleSize;
	}

	void createShaderBindingTable()
	{
		// Create buffer for the shader binding table
		const uint32_t sbtSize = rayTracingProperties.shaderGroupHandleSize * 3;
		PH_BufferCreateInfo SBTcreateInfo;
		SBTcreateInfo.bufferSize = sbtSize;
		SBTcreateInfo.bufferUsageFlags = VK_BUFFER_USAGE_RAY_TRACING_BIT_NV;
		SBTcreateInfo.memoryPropertyFlags = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT;
		PH_CreateBuffer(SBTcreateInfo, shaderBindingTable);

		auto shaderHandleStorage = new uint8_t[sbtSize];
		// Get shader identifiers
		VkResult result = vkGetRayTracingShaderGroupHandlesNV(device, rtPipeline, 0, 3, sbtSize, shaderHandleStorage);
		uint8_t* data;
		vkMapMemory(device, shaderBindingTable.bufferMemory, 0, shaderBindingTable.bufferSize, 0, (void**)&data);
			// Copy the shader identifiers to the shader binding table
			VkDeviceSize offset = 0;
			data += copyShaderIdentifier(data, shaderHandleStorage, INDEX_RAYGEN);
			data += copyShaderIdentifier(data, shaderHandleStorage, INDEX_MISS);
			data += copyShaderIdentifier(data, shaderHandleStorage, INDEX_CLOSEST_HIT);
		vkUnmapMemory(device, shaderBindingTable.bufferMemory);
	}

	void RecordCommandBuffers() override
	{
		std::vector<VkCommandBuffer> commandBuffers = GetCommandBuffers();

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

			/*
				Dispatch the ray tracing commands
			*/
			vkCmdBindPipeline(commandBuffers[i], VK_PIPELINE_BIND_POINT_RAY_TRACING_NV, rtPipeline);
			vkCmdBindDescriptorSets(commandBuffers[i], VK_PIPELINE_BIND_POINT_RAY_TRACING_NV, pipelineLayout, 0, 1, &descriptorSet, 0, 0);

			// Calculate shader binding offsets, which is pretty straight forward in our example 
			VkDeviceSize bindingOffsetRayGenShader = rayTracingProperties.shaderGroupHandleSize * INDEX_RAYGEN;
			VkDeviceSize bindingOffsetMissShader = rayTracingProperties.shaderGroupHandleSize * INDEX_MISS;
			VkDeviceSize bindingOffsetHitShader = rayTracingProperties.shaderGroupHandleSize * INDEX_CLOSEST_HIT;
			VkDeviceSize bindingStride = rayTracingProperties.shaderGroupHandleSize;

			VkExtent2D swapChainExtent = GetSwapChainExtent();
			vkCmdTraceRaysNV(commandBuffers[i],
				shaderBindingTable.buffer, bindingOffsetRayGenShader,
				shaderBindingTable.buffer, bindingOffsetMissShader, bindingStride,
				shaderBindingTable.buffer, bindingOffsetHitShader, bindingStride,
				VK_NULL_HANDLE, 0, 0,
				swapChainExtent.width, swapChainExtent.height, 1);

			/*
				Copy raytracing output to swap chain image
			*/

			// Prepare current swapchain image as transfer destination
			transitionImageLayout(swapChainImages[i], GetSwapChainImageFormat(), VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);

			// Prepare ray tracing output image as transfer source
			transitionImageLayout(storageImage.image, GetSwapChainImageFormat(), VK_IMAGE_LAYOUT_GENERAL, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL);
			
			VkImageCopy copyRegion{};
			copyRegion.srcSubresource = { VK_IMAGE_ASPECT_COLOR_BIT, 0, 0, 1 };
			copyRegion.srcOffset = { 0, 0, 0 };
			copyRegion.dstSubresource = { VK_IMAGE_ASPECT_COLOR_BIT, 0, 0, 1 };
			copyRegion.dstOffset = { 0, 0, 0 };
			copyRegion.extent = { swapChainExtent.width, swapChainExtent.height, 1 };
			vkCmdCopyImage(commandBuffers[i], storageImage.image, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, swapChainImages[i], VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &copyRegion);

			// Transition swap chain image back for presentation
			transitionImageLayout(swapChainImages[i], GetSwapChainImageFormat(), VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_PRESENT_SRC_KHR);
			
			// Transition ray tracing output image back to general layout
			transitionImageLayout(storageImage.image, GetSwapChainImageFormat(), VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, VK_IMAGE_LAYOUT_GENERAL);
			
			if (vkEndCommandBuffer(commandBuffers[i]) != VK_SUCCESS)
			{
				throw std::runtime_error("failed to record command buffer!");
			}


			//VkRenderPassBeginInfo renderPassInfo = {};
			//renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
			//renderPassInfo.renderPass = GetDefaultRenderPass();
			//renderPassInfo.framebuffer = *(GetSwapChainFrameBuffers()+ i);
			//renderPassInfo.renderArea.offset = { 0, 0 };
			//renderPassInfo.renderArea.extent = GetSwapChainExtent();

			//std::vector<VkClearValue> clearValues;
			//VkClearValue clearValue;
			//clearValue.color = { 0.0f, 0.0f, 0.0f, 1.0f };
			//clearValues.emplace_back(clearValue);

			//VkClearValue depthValue;
			//depthValue.depthStencil = { 1.0f, 0 };
			//clearValues.emplace_back(depthValue);

			//renderPassInfo.clearValueCount = static_cast<uint32_t>(clearValues.size());
			//renderPassInfo.pClearValues = clearValues.data();

			//vkCmdBeginRenderPass(commandBuffers[i], &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);
			//vkCmdBindPipeline(commandBuffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, graphicsPipeline);

			//// vertex and index buffers
			//VkBuffer vertexBuffers[] = { vertexBuffer.buffer };
			//VkDeviceSize offsets[] = { 0 };
			//vkCmdBindVertexBuffers(commandBuffers[i], 0, 1, vertexBuffers, offsets);
			//vkCmdBindIndexBuffer(commandBuffers[i], indexBuffer.buffer, 0, VK_INDEX_TYPE_UINT16);

			//// descriptor sets - uniform buffer
			//vkCmdBindDescriptorSets(commandBuffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout, 0, 1, &descriptorSets[i], 0, nullptr);

			//vkCmdDrawIndexed(commandBuffers[i], static_cast<uint32_t>(indices.size()), 1, 0, 0, 0);
			//vkCmdEndRenderPass(commandBuffers[i]);

			//if (vkEndCommandBuffer(commandBuffers[i]) != VK_SUCCESS)
			//{
			//	throw std::runtime_error("failed to record command buffer!");
			//}
		}
	}

	void DrawFrame() override
	{
		uint32_t imageIndex = PH_PrepareNextFrame();

		if (imageIndex != -1) // invalid
		{
			UpdateUniformBuffer(imageIndex);

			PH_SubmitFrame();
		}
	}
};

int main()
{
	try {
		Application* app = new Application();
		app->initWindow();
		app->initVulkan();

		while (!app->windowShouldClose())
		{
			app->pollEvents();
			if (app->isKeyPressed(PH_KEY_ESCAPE))
			{
				break;
			}
			app->DrawFrame();
		}

		app->waitDeviceIdle();
		app->cleanupVulkan();
		app->destroyWindow();
		delete app;
	}
	catch (const std::exception& e) {
		std::cerr << e.what() << std::endl;
		return EXIT_FAILURE;
	}

	return EXIT_SUCCESS;
}