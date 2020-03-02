#include "Picker.h"

#if (RAYTRACING_REFLECTIONS)

#include "../../../Common/Renderer/VulkanRenderer.h"
#include "../../../Common/Renderer/Application.h"

#include <iostream>
#include <array>
#include <chrono>

struct UniformBufferObject
{
	alignas(16) glm::mat4 model;
	alignas(16) glm::mat4 view;
	alignas(16) glm::mat4 proj;
	alignas(16) glm::vec4 lightPos;
} ubo;

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

VkShaderModule RayGenModule;
VkShaderModule MissModule;
VkShaderModule ClosestHitModule;

#pragma endregion


class Application : public VulkanRenderer
{
	bool firstMouse = true;
	float lastX = 0.0f;
	float lastY = 0.0f;

	PH_Buffer uniformBuffer;
	VkPipelineLayout pipelineLayout;
	VkDescriptorPool descriptorPool;
	VkDescriptorSetLayout descriptorSetLayout;
	VkDescriptorSet descriptorSet;
	VkPipeline rtPipeline;

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

	PH_Buffer shaderBindingTable;
	PH_Image storageImage;
	PH_Model model;
	uint32_t noOfTextures;

	VertexLayout vertexLayout = VertexLayout({
		VERTEX_COMPONENT_POSITION,
		VERTEX_COMPONENT_NORMAL,
		VERTEX_COMPONENT_COLOR,
		VERTEX_COMPONENT_UV,
		VERTEX_COMPONENT_DUMMY_FLOAT
		});

	const uint32_t shaderIndexRaygen = 0;
	const uint32_t shaderIndexMiss = 1;
	const uint32_t shaderIndexClosestHit = 2;

#pragma region Acceleration_Structures

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
		memoryAllocateInfo.memoryTypeIndex = findMemoryType(memoryRequirements2.memoryRequirements.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
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
		memoryAllocateInfo.memoryTypeIndex = findMemoryType(memoryRequirements2.memoryRequirements.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
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
		Settings settings;
		settings.maxInFlightFrames = 3;
		settings.app_name = "Ray Tracing - Reflections";

		initRenderer(settings);

		camera.rotation_speed *= 0.25f;
		camera.translation_speed *= 10.0f;
		camera.type = CameraType::FirstPerson;
		camera.set_perspective(60.0f, (float)swapChainExtent.width / (float)swapChainExtent.height, 0.1f, 512.0f);
		camera.set_rotation(glm::vec3(0.0f, 0.0f, 0.0f));
		camera.set_translation(glm::vec3(0.0f, 0.0f, -1.5f));

		//PH_LoadModel("../../Phoenix/App/Test/Models/reflection_test.dae", vertexLayout, &model);
		PH_LoadModel("../../Phoenix/RendererOpenGL/App/Resources/Objects/sponza/sponza.obj", vertexLayout, &model);
		
		for (uint32_t i = 0; i < model.mMeshTexturesMap.size(); ++i)
		{
			for (uint32_t j = 0; j < model.mMeshTexturesMap[i].size(); ++j)
			{
				++noOfTextures;
			}
		}

		// Shader Modules
		{
			PH_CreateShaderModule("../../Phoenix/App/Test/Shaders/SpirV/ref_raygen.rgen.spv", &RayGenModule);
			PH_CreateShaderModule("../../Phoenix/App/Test/Shaders/SpirV/ref_miss.rmiss.spv", &MissModule);
			PH_CreateShaderModule("../../Phoenix/App/Test/Shaders/SpirV/ref_closesthit.rchit.spv", &ClosestHitModule);
		}

#pragma region RT_Scene

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


		VkGeometryNV geometry{};
		geometry.sType = VK_STRUCTURE_TYPE_GEOMETRY_NV;
		geometry.geometryType = VK_GEOMETRY_TYPE_TRIANGLES_NV;
		geometry.geometry.triangles.sType = VK_STRUCTURE_TYPE_GEOMETRY_TRIANGLES_NV;
		geometry.geometry.triangles.vertexData = model.vertices.buffer;
		geometry.geometry.triangles.vertexOffset = 0;
		geometry.geometry.triangles.vertexCount = static_cast<uint32_t>(model.vertexCount);
		geometry.geometry.triangles.vertexStride = vertexLayout.stride();
		geometry.geometry.triangles.vertexFormat = VK_FORMAT_R32G32B32_SFLOAT;
		geometry.geometry.triangles.indexData = model.indices.buffer;
		geometry.geometry.triangles.indexOffset = 0;
		geometry.geometry.triangles.indexCount = static_cast<uint32_t>(model.indexCount);
		geometry.geometry.triangles.indexType = VK_INDEX_TYPE_UINT32;
		geometry.geometry.triangles.transformData = VK_NULL_HANDLE;
		geometry.geometry.triangles.transformOffset = 0;
		geometry.geometry.aabbs = {};
		geometry.geometry.aabbs.sType = { VK_STRUCTURE_TYPE_GEOMETRY_AABB_NV };
		geometry.flags = VK_GEOMETRY_OPAQUE_BIT_NV;

		createBottomLevelAccelerationStructure(&geometry);


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

		PH_CreateBuffer(instanceBufferCreateInfo, &instanceBuffer);

		createTopLevelAccelerationStructure();


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
		PH_CreateBuffer(scratchBufferCreateInfo, &scratchBuffer);

		VkCommandBuffer cmdBuffer = beginSingleTimeCommands();


		VkAccelerationStructureInfoNV buildInfo{};
		buildInfo.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_INFO_NV;
		buildInfo.type = VK_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL_NV;
		buildInfo.geometryCount = 1;
		buildInfo.pGeometries = &geometry;
		buildInfo.instanceCount = 0;
		buildInfo.flags = 0;
		buildInfo.pNext = nullptr;

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


		buildInfo.type = VK_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL_NV;
		buildInfo.geometryCount = 0;
		buildInfo.pGeometries = nullptr;
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

		PH_DeleteBuffer(&scratchBuffer);
		PH_DeleteBuffer(&instanceBuffer);

#pragma endregion
	}

	void Exit() override
	{
		// acceleration structures
		vkFreeMemory(device, bottomLevelAS.memory, nullptr);
		vkFreeMemory(device, topLevelAS.memory, nullptr);
		vkDestroyAccelerationStructureNV(device, bottomLevelAS.accelerationStructure, nullptr);
		vkDestroyAccelerationStructureNV(device, topLevelAS.accelerationStructure, nullptr);

		// shader modules
		PH_DestroyShaderModule(&ClosestHitModule);
		PH_DestroyShaderModule(&MissModule);
		PH_DestroyShaderModule(&RayGenModule);

		// MODEL
		PH_DeleteModel(&model);

		exitRenderer();
	}

	void Load()	override
	{
#pragma region StorageImage

		PH_ImageCreateInfo storageImageInfo;
		storageImageInfo.width = swapChainExtent.width;
		storageImageInfo.height = swapChainExtent.height;
		storageImageInfo.format = swapChainImageFormat;
		storageImageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
		storageImageInfo.usageFlags = VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_STORAGE_BIT;
		storageImageInfo.memoryProperty = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
		storageImageInfo.aspectBits = VK_IMAGE_ASPECT_COLOR_BIT;

		PH_CreateTexture(storageImageInfo, &storageImage);

		VkCommandBuffer commandBuffer = beginSingleTimeCommands();
		transitionImageLayout(commandBuffer, storageImage.image, storageImage.format, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_GENERAL);
		endSingleTimeCommands(commandBuffer);

#pragma endregion

		// Uniform Buffers
		{
			PH_BufferCreateInfo uniformBufferInfo;
			uniformBufferInfo.bufferSize = (VkDeviceSize)(sizeof(UniformBufferObject));
			uniformBufferInfo.bufferUsageFlags = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;
			uniformBufferInfo.memoryPropertyFlags = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;

			PH_CreateBuffer(uniformBufferInfo, &uniformBuffer);
		}

		createDescriptorSetLayout();
		createPipeline();
		createShaderBindingTable();
		createDescriptorPool();
		createDescriptorSets();
		RecordCommandBuffers();
	}

	void UnLoad() override
	{
		// SBT
		PH_DeleteBuffer(&shaderBindingTable);

		// pipeline
		vkDestroyPipeline(device, rtPipeline, nullptr);
		PH_DeletePipelineLayout(&pipelineLayout);

		// Descriptors
		PH_DeleteDescriptorPool(&descriptorPool);
		PH_DeleteDescriptorSetLayout(&descriptorSetLayout);

		// Buffers and Images
		PH_DeleteBuffer(&uniformBuffer);
		PH_DeleteTexture(&storageImage);
	}

	void createDescriptorSetLayout() override
	{
		VkDescriptorSetLayoutBinding accelerationStructureLayoutBinding{};
		accelerationStructureLayoutBinding.binding = 0;
		accelerationStructureLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_NV;
		accelerationStructureLayoutBinding.descriptorCount = 1;
		accelerationStructureLayoutBinding.stageFlags = VK_SHADER_STAGE_RAYGEN_BIT_NV | VK_SHADER_STAGE_CLOSEST_HIT_BIT_NV;
		accelerationStructureLayoutBinding.pImmutableSamplers = nullptr;

		VkDescriptorSetLayoutBinding resultImageLayoutBinding{};
		resultImageLayoutBinding.binding = 1;
		resultImageLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
		resultImageLayoutBinding.descriptorCount = 1;
		resultImageLayoutBinding.stageFlags = VK_SHADER_STAGE_RAYGEN_BIT_NV;
		resultImageLayoutBinding.pImmutableSamplers = nullptr;

		VkDescriptorSetLayoutBinding uniformBufferBinding{};
		uniformBufferBinding.binding = 2;
		uniformBufferBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		uniformBufferBinding.descriptorCount = 1;
		uniformBufferBinding.stageFlags = VK_SHADER_STAGE_RAYGEN_BIT_NV | VK_SHADER_STAGE_CLOSEST_HIT_BIT_NV | VK_SHADER_STAGE_MISS_BIT_NV;
		uniformBufferBinding.pImmutableSamplers = nullptr;

		VkDescriptorSetLayoutBinding vertexBufferBinding{};
		vertexBufferBinding.binding = 3;
		vertexBufferBinding.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
		vertexBufferBinding.descriptorCount = 1;
		vertexBufferBinding.stageFlags = VK_SHADER_STAGE_CLOSEST_HIT_BIT_NV;
		vertexBufferBinding.pImmutableSamplers = nullptr;

		VkDescriptorSetLayoutBinding indexBufferBinding{};
		indexBufferBinding.binding = 4;
		indexBufferBinding.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
		indexBufferBinding.descriptorCount = 1;
		indexBufferBinding.stageFlags = VK_SHADER_STAGE_CLOSEST_HIT_BIT_NV;
		indexBufferBinding.pImmutableSamplers = nullptr;

		VkDescriptorSetLayoutBinding samplerLayoutBinding = {};
		samplerLayoutBinding.binding = 5;
		samplerLayoutBinding.descriptorCount = noOfTextures;
		samplerLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
		samplerLayoutBinding.stageFlags = VK_SHADER_STAGE_CLOSEST_HIT_BIT_NV;
		samplerLayoutBinding.pImmutableSamplers = nullptr;

		std::vector<VkDescriptorSetLayoutBinding> bindings({
			accelerationStructureLayoutBinding,
			resultImageLayoutBinding,
			uniformBufferBinding,
			vertexBufferBinding,
			indexBufferBinding,
			samplerLayoutBinding
			});

		VkDescriptorSetLayoutCreateInfo layoutInfo{};
		layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
		layoutInfo.bindingCount = static_cast<uint32_t>(bindings.size());
		layoutInfo.pBindings = bindings.data();
		
		PH_CreateDescriptorSetLayout(layoutInfo, &descriptorSetLayout);
	}

	void createPipeline() override
	{
		VkPipelineLayoutCreateInfo pipelineLayoutCreateInfo{};
		pipelineLayoutCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
		pipelineLayoutCreateInfo.setLayoutCount = 1;
		pipelineLayoutCreateInfo.pSetLayouts = &descriptorSetLayout;
		pipelineLayoutCreateInfo.flags = 0;
		pipelineLayoutCreateInfo.pNext = nullptr;
		pipelineLayoutCreateInfo.pPushConstantRanges = nullptr;
		pipelineLayoutCreateInfo.pushConstantRangeCount = 0;

		PH_CreatePipelineLayout(pipelineLayoutCreateInfo, &pipelineLayout);

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

		for (VkPipelineShaderStageCreateInfo& shaderStage : shaderStages)
		{
			shaderStage.flags = 0;
			shaderStage.pNext = nullptr;
			shaderStage.pSpecializationInfo = VK_NULL_HANDLE;
		}

		// Pass recursion depth for reflections to ray generation shader via specialization constant
		VkSpecializationMapEntry specializationMapEntry{};
		specializationMapEntry.constantID = 0;
		specializationMapEntry.offset = 0;
		specializationMapEntry.size = sizeof(uint32_t);
		
		uint32_t maxRecursion = 4;
		VkSpecializationInfo specializationInfo{};
		specializationInfo.mapEntryCount = 1;
		specializationInfo.pMapEntries = &specializationMapEntry;
		specializationInfo.dataSize = sizeof(maxRecursion);
		specializationInfo.pData = &maxRecursion;

		shaderStages[shaderIndexRaygen].pSpecializationInfo = &specializationInfo;

		/*
			Setup ray tracing shader groups
		*/
		std::array<VkRayTracingShaderGroupCreateInfoNV, 3> groups{};
		for (VkRayTracingShaderGroupCreateInfoNV& group : groups)
		{
			// Init all groups with some default values
			group.sType = VK_STRUCTURE_TYPE_RAY_TRACING_SHADER_GROUP_CREATE_INFO_NV;
			group.generalShader = VK_SHADER_UNUSED_NV;
			group.closestHitShader = VK_SHADER_UNUSED_NV;
			group.anyHitShader = VK_SHADER_UNUSED_NV;
			group.intersectionShader = VK_SHADER_UNUSED_NV;
			group.pNext = nullptr;
		}

		// Links shaders and types to ray tracing shader groups
		groups[INDEX_RAYGEN].type = VK_RAY_TRACING_SHADER_GROUP_TYPE_GENERAL_NV;
		groups[INDEX_RAYGEN].generalShader = shaderIndexRaygen;

		groups[INDEX_MISS].type = VK_RAY_TRACING_SHADER_GROUP_TYPE_GENERAL_NV;
		groups[INDEX_MISS].generalShader = shaderIndexMiss;

		groups[INDEX_CLOSEST_HIT].type = VK_RAY_TRACING_SHADER_GROUP_TYPE_TRIANGLES_HIT_GROUP_NV;
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

	void createDescriptorPool() override
	{
		std::vector<VkDescriptorPoolSize> poolSizes = {
			{ VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_NV, 1 },
			{ VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 1 },
			{ VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1 },
			{ VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 2 },
			{ VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, noOfTextures }
		};

		PH_CreateDescriptorPool((uint32_t)poolSizes.size(), poolSizes.data(), 1, &descriptorPool);
	}

	void createDescriptorSets() override
	{
		PH_CreateDescriptorSets(descriptorSetLayout, 1, descriptorPool, &descriptorSet);

		/////////////////////////////////////////////////////// ACCELERATION STRUCTURE
		VkWriteDescriptorSetAccelerationStructureNV descriptorAccelerationStructureInfo{};
		descriptorAccelerationStructureInfo.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET_ACCELERATION_STRUCTURE_NV;
		descriptorAccelerationStructureInfo.accelerationStructureCount = 1;
		descriptorAccelerationStructureInfo.pAccelerationStructures = &topLevelAS.accelerationStructure;

		VkWriteDescriptorSet accelerationStructureWrite{};
		accelerationStructureWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		accelerationStructureWrite.dstSet = descriptorSet;
		accelerationStructureWrite.descriptorType = VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_NV;
		accelerationStructureWrite.dstBinding = 0;
		accelerationStructureWrite.descriptorCount = 1;
		// The specialized acceleration structure descriptor has to be chained
		accelerationStructureWrite.pNext = &descriptorAccelerationStructureInfo;
		accelerationStructureWrite.dstArrayElement = 0;
		accelerationStructureWrite.pBufferInfo = nullptr;
		accelerationStructureWrite.pImageInfo = nullptr;
		accelerationStructureWrite.pTexelBufferView = nullptr;

		/////////////////////////////////////////////////////// STORAGE IMAGE
		VkDescriptorImageInfo storageImageDescriptor{};
		storageImageDescriptor.imageView = storageImage.imageView;
		storageImageDescriptor.imageLayout = VK_IMAGE_LAYOUT_GENERAL;

		VkWriteDescriptorSet resultImageWrite{};
		resultImageWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		resultImageWrite.dstSet = descriptorSet;
		resultImageWrite.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
		resultImageWrite.dstBinding = 1;
		resultImageWrite.descriptorCount = 1;
		resultImageWrite.pImageInfo = &storageImageDescriptor;
		resultImageWrite.dstArrayElement = 0;
		resultImageWrite.pNext = nullptr;
		resultImageWrite.pBufferInfo = nullptr;
		resultImageWrite.pTexelBufferView = nullptr;

		/////////////////////////////////////////////////////// UNIFORM BUFFER
		VkDescriptorBufferInfo bufferInfo;
		bufferInfo.buffer = uniformBuffer.buffer;
		bufferInfo.offset = 0;
		bufferInfo.range = sizeof(UniformBufferObject);

		VkWriteDescriptorSet uniformBufferWrite{};
		uniformBufferWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		uniformBufferWrite.dstSet = descriptorSet;
		uniformBufferWrite.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		uniformBufferWrite.dstBinding = 2;
		uniformBufferWrite.descriptorCount = 1;
		uniformBufferWrite.pBufferInfo = &bufferInfo;
		uniformBufferWrite.dstArrayElement = 0;
		uniformBufferWrite.pNext = nullptr;
		uniformBufferWrite.pImageInfo = nullptr;
		uniformBufferWrite.pTexelBufferView = nullptr;

		/////////////////////////////////////////////////////// VERTEX BUFFER
		VkDescriptorBufferInfo vertexBufferDescriptor{};
		vertexBufferDescriptor.buffer = model.vertices.buffer;
		vertexBufferDescriptor.range = VK_WHOLE_SIZE;

		VkWriteDescriptorSet vertexBufferWrite;
		vertexBufferWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		vertexBufferWrite.dstSet = descriptorSet;
		vertexBufferWrite.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
		vertexBufferWrite.dstBinding = 3;
		vertexBufferWrite.descriptorCount = 1;
		vertexBufferWrite.pBufferInfo = &vertexBufferDescriptor;
		vertexBufferWrite.dstArrayElement = 0;
		vertexBufferWrite.pNext = nullptr;
		vertexBufferWrite.pImageInfo = nullptr;
		vertexBufferWrite.pTexelBufferView = nullptr;

		/////////////////////////////////////////////////////// INDEX BUFFER
		VkDescriptorBufferInfo indexBufferDescriptor{};
		indexBufferDescriptor.buffer = model.indices.buffer;
		indexBufferDescriptor.range = VK_WHOLE_SIZE;

		VkWriteDescriptorSet indexBufferWrite;
		indexBufferWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		indexBufferWrite.dstSet = descriptorSet;
		indexBufferWrite.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
		indexBufferWrite.dstBinding = 4;
		indexBufferWrite.descriptorCount = 1;
		indexBufferWrite.pBufferInfo = &indexBufferDescriptor;
		indexBufferWrite.dstArrayElement = 0;
		indexBufferWrite.pNext = nullptr;
		indexBufferWrite.pImageInfo = nullptr;
		indexBufferWrite.pTexelBufferView = nullptr;

		/////////////////////////////////////////////////////// TEXTURE SAMPLERS
		std::vector<VkWriteDescriptorSet> texImageDescriptor{};

		for (uint32_t i = 0; i < (uint32_t)model.parts.size(); ++i)
		{
			uint32_t matIndex = model.parts[i].materialIndex;

			VkDescriptorImageInfo imageInfo = {};
			imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
			imageInfo.imageView = model.mMeshTexturesMap[matIndex].begin()->imageView;
			imageInfo.sampler = model.mMeshTexturesMap[matIndex].begin()->sampler;
				
			VkWriteDescriptorSet samplerWrite;
			samplerWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
			samplerWrite.dstSet = descriptorSet;
			samplerWrite.dstBinding = 5;
			samplerWrite.dstArrayElement = 0;
			samplerWrite.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
			samplerWrite.descriptorCount = noOfTextures;
			samplerWrite.pImageInfo = &imageInfo;
			samplerWrite.dstArrayElement = 0;
			samplerWrite.pNext = nullptr;
			samplerWrite.pImageInfo = nullptr;
			samplerWrite.pTexelBufferView = nullptr;

			texImageDescriptor.push_back(samplerWrite);
		}

		std::vector<VkWriteDescriptorSet> writeDescriptorSets = {
			accelerationStructureWrite,
			resultImageWrite,
			uniformBufferWrite,
			vertexBufferWrite,
			indexBufferWrite
		};
		writeDescriptorSets.insert(writeDescriptorSets.end(), texImageDescriptor.begin(), texImageDescriptor.end());

		PH_UpdateDeDescriptorSets(writeDescriptorSets);
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
		PH_CreateBuffer(SBTcreateInfo, &shaderBindingTable);

		auto shaderHandleStorage = new uint8_t[sbtSize];
		// Get shader identifiers
		VkResult result = vkGetRayTracingShaderGroupHandlesNV(device, rtPipeline, 0, 3, sbtSize, shaderHandleStorage);
		void* dataPtr = nullptr;
		vkMapMemory(device, shaderBindingTable.bufferMemory, 0, shaderBindingTable.bufferSize, 0, &dataPtr);
			uint8_t* data = static_cast<uint8_t*>(dataPtr);
			// Copy the shader identifiers to the shader binding table
			data += copyShaderIdentifier(data, shaderHandleStorage, INDEX_RAYGEN);
			data += copyShaderIdentifier(data, shaderHandleStorage, INDEX_MISS);
			data += copyShaderIdentifier(data, shaderHandleStorage, INDEX_CLOSEST_HIT);
		vkUnmapMemory(device, shaderBindingTable.bufferMemory);
	}

	void RecordCommandBuffers() override
	{
		for (uint32_t i = 0; i < (uint32_t)commandBuffers.size(); ++i)
		{
			VkCommandBufferBeginInfo beginInfo = {};
			beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
			beginInfo.flags = VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT;
			beginInfo.pInheritanceInfo = nullptr; // Optional

			if (vkBeginCommandBuffer(commandBuffers[i], &beginInfo) != VK_SUCCESS)
			{
				throw std::runtime_error("failed to begin recording command buffer!");
			}

			vkCmdBindPipeline(commandBuffers[i], VK_PIPELINE_BIND_POINT_RAY_TRACING_NV, rtPipeline);
			vkCmdBindDescriptorSets(commandBuffers[i], VK_PIPELINE_BIND_POINT_RAY_TRACING_NV, pipelineLayout, 0, 1, &descriptorSet, 0, 0);

			// Calculate shader binding offsets, which is pretty straight forward in our example 
			VkDeviceSize bindingOffsetRayGenShader = rayTracingProperties.shaderGroupHandleSize * INDEX_RAYGEN;
			VkDeviceSize bindingOffsetMissShader = rayTracingProperties.shaderGroupHandleSize * INDEX_MISS;
			VkDeviceSize bindingOffsetHitShader = rayTracingProperties.shaderGroupHandleSize * INDEX_CLOSEST_HIT;
			VkDeviceSize bindingStride = rayTracingProperties.shaderGroupHandleSize;

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
			transitionImageLayout(commandBuffers[i], swapChainImages[i], swapChainImageFormat, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);

			// Prepare ray tracing output image as transfer source
			transitionImageLayout(commandBuffers[i], storageImage.image, storageImage.format, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL);

			VkImageCopy copyRegion{};
			copyRegion.srcSubresource = { VK_IMAGE_ASPECT_COLOR_BIT, 0, 0, 1 };
			copyRegion.srcOffset = { 0, 0, 0 };
			copyRegion.dstSubresource = { VK_IMAGE_ASPECT_COLOR_BIT, 0, 0, 1 };
			copyRegion.dstOffset = { 0, 0, 0 };
			copyRegion.extent = { swapChainExtent.width, swapChainExtent.height, 1 };
			vkCmdCopyImage(commandBuffers[i], storageImage.image, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, swapChainImages[i], VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &copyRegion);

			// Transition swap chain image back for presentation
			transitionImageLayout(commandBuffers[i], swapChainImages[i], swapChainImageFormat, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_PRESENT_SRC_KHR);

			// Transition ray tracing output image back to general layout
			transitionImageLayout(commandBuffers[i], storageImage.image, storageImage.format, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, VK_IMAGE_LAYOUT_GENERAL);

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
		float timer = std::chrono::duration<float, std::chrono::seconds::period>(currentTime - startTime).count();

		UniformBufferObject ubo = {};
		ubo.model = glm::mat4(1.0f);
		ubo.proj = glm::inverse(camera.matrices.perspective);
		ubo.view = glm::inverse(camera.matrices.view);
		ubo.lightPos = glm::vec4(cos(glm::radians(timer * 360.0f)) * 40.0f, -20.0f + sin(glm::radians(timer * 360.0f)) * 20.0f, 25.0f + sin(glm::radians(timer * 360.0f)) * 5.0f, 0.0f);
		
		PH_BufferUpdateInfo bufferUpdate;
		bufferUpdate.buffer = uniformBuffer;
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
		UpdateCamera();

		uint32_t imageIndex = PH_PrepareNextFrame();

		if (imageIndex != -1) // invalid
		{
			UpdateUniformBuffer(imageIndex);

			PH_SubmitFrame();
		}
	}
	
};

PHOENIX_MAIN(Application)

#endif