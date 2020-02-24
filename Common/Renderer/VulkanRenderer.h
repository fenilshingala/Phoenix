#pragma once

#include <vulkan/vulkan.h>

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <vector>
#include <string>

#include "Window.h"
#include "Camera.hpp"

struct Settings
{
	int windowWidth = 0;
	int windowHeight = 0;
	uint32_t maxInFlightFrames = 2;
};

struct PH_ImageCreateInfo
{
	std::string				path;
	int						width;
	int						height;
	int						nChannels;
	VkFormat				format = VK_FORMAT_UNDEFINED;
	VkImageUsageFlags		usageFlags;
	VkImageTiling			tiling;
	VkMemoryPropertyFlags	memoryProperty;
	VkImageAspectFlagBits	aspectBits;
};

struct PH_Image
{
	VkImage				image;
	VkDeviceMemory		imageMemory;
	VkImageView			imageView;

	VkFormat			format = VK_FORMAT_UNDEFINED;
	int					width;
	int					height;
	int					nChannels;
	std::string			path;
};

struct PH_BufferCreateInfo
{
	VkBufferUsageFlags		bufferUsageFlags;
	VkMemoryPropertyFlags	memoryPropertyFlags;
	void*					data = nullptr;
	VkDeviceSize			bufferSize;
};

struct PH_Buffer
{
	VkBuffer		buffer;
	VkDeviceSize	bufferSize;
	VkDeviceMemory	bufferMemory;
};

struct PH_BufferUpdateInfo
{
	PH_Buffer		buffer;
	void*			data = nullptr;
	VkDeviceSize	dataSize;
};


class VulkanRenderer
{
public:
	VulkanRenderer();

	~VulkanRenderer();

	///////////////////////////////////////////
	//// VULKAN
	void initVulkan();
	void cleanupVulkan();
	void waitDeviceIdle();
	
	// TEXTURES
	void PH_CreateTexture(PH_ImageCreateInfo info, PH_Image* ph_image);
	void PH_DeleteTexture(PH_Image* ph_image);

	// BUFFERS
	void PH_CreateBuffer(PH_BufferCreateInfo info, PH_Buffer* ph_buffer);
	void PH_DeleteBuffer(PH_Buffer* ph_buffer);
	void PH_UpdateBuffer(PH_BufferUpdateInfo info);
	
	// SAMPLERS
	VkSampler PH_CreateSampler();
	void PH_DeleteSampler(VkSampler* sampler);
	
	// SHADER MODULES
	void PH_CreateShaderModule(const char* path, VkShaderModule* shaderModule);
	void PH_DestroyShaderModule(VkShaderModule* shaderModule);
	
	// DescriptorSet Layout
	void PH_CreateDescriptorSetLayout(VkDescriptorSetLayoutCreateInfo layoutInfo, VkDescriptorSetLayout* descriptorSetLayout);
	void PH_DeleteDescriptorSetLayout(VkDescriptorSetLayout* descriptorSetLayout);

	// Pipeline Layout
	void PH_CreatePipelineLayout(VkPipelineLayoutCreateInfo pipelineLayoutInfo, VkPipelineLayout* pipelineLayout);
	void PH_DeletePipelineLayout(VkPipelineLayout* pipelineLayout);

	// Graphics Pipeline
	void PH_CreateGraphicsPipeline(VkGraphicsPipelineCreateInfo pipelineInfo, VkPipeline* pipeline);
	void PH_DeleteGraphicsPipeline(VkPipeline* pipeline);

	void PH_CreateDescriptorPool(uint32_t noOfPools, VkDescriptorPoolSize* poolSizes, uint32_t maxSets, VkDescriptorPool* descriptorPool);
	void PH_DeleteDescriptorPool(VkDescriptorPool* descriptorPool);
	
	// Descriptor Sets
	void PH_CreateDescriptorSets(VkDescriptorSetLayout descriptorSetLayout, uint32_t descriptorCount, VkDescriptorPool descriptorPool, VkDescriptorSet* descriptorSets);
	void PH_UpdateDeDescriptorSets(std::vector<VkWriteDescriptorSet> descriptorWrites);
	
	const uint32_t PH_PrepareNextFrame();
	void PH_SubmitFrame();

	VkCommandBuffer beginSingleTimeCommands();
	void endSingleTimeCommands(VkCommandBuffer commandBuffer);
	void transitionImageLayout(VkCommandBuffer cmd, VkImage image, VkFormat format, VkImageLayout oldLayout, VkImageLayout newLayout);
	uint32_t findMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties);


	// VIRTUALS
	virtual void createRenderPass();				// default render pass, can be overriden by app
	virtual void createDescriptorSetLayout() = 0;
	virtual void createPipeline() = 0;
	virtual void RecordCommandBuffers() = 0;
	virtual void createDescriptorPool() = 0;
	virtual void createDescriptorSets() = 0;
	virtual void Init() = 0;
	virtual void Exit() = 0;
	virtual void Load() = 0;
	virtual void UnLoad() = 0;
	virtual void DrawFrame() = 0;

	Window* pWindow;
	
	VkPhysicalDevice physicalDevice;
	VkDevice device; // logical
	
	std::vector<VkImage> swapChainImages;
	VkFormat swapChainImageFormat;
	VkExtent2D swapChainExtent;
	std::vector<VkFramebuffer> swapChainFramebuffers;
	std::vector<VkCommandBuffer> commandBuffers;

	VkRenderPass renderPass;
	
	Camera camera;

private:
	void recreateSwapChain();
	void cleanupSwapChain();

	void createInstance();
	void pickPhysicalDevice();
	void createLogicalDevice();
	void createSwapChain();
	void createImageViews();
	void createCommandPool();
	void createSyncObjects();
	void createCommandBuffers();

	void createDepthResources();
	void createFramebuffers();

	void createBuffer(VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties, VkBuffer& buffer, VkDeviceMemory& bufferMemory);
	void copyBuffer(VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size);
	void copyBufferToImage(VkBuffer buffer, VkImage image, uint32_t width, uint32_t height);

	void createImage(uint32_t width, uint32_t height, VkFormat format, VkImageTiling tiling, VkImageUsageFlags usage,
		VkMemoryPropertyFlags properties, VkImage& image, VkDeviceMemory& imageMemory);
	VkImageView createImageView(VkImage image, VkFormat format, VkImageAspectFlags aspectFlags);

	void destroyInstance();

	VkExtent2D chooseSwapExtent(const VkSurfaceCapabilitiesKHR& capabilities, int& _WIDTH, int& _HEIGHT);
	bool checkValidationLayerSupport();
	void populateDebugMessengerCreateInfo(VkDebugUtilsMessengerCreateInfoEXT& debugCreateInfo);

	VkInstance instance;
	
	VkQueue graphicsQueue;
	VkQueue presentQueue;
	
	VkSurfaceKHR surface;
	
	VkSwapchainKHR swapChain;
	std::vector<VkImageView> swapChainImageViews;
	
	VkCommandPool commandPool;

	VkImage depthImage;
	VkDeviceMemory depthImageMemory;
	VkImageView depthImageView;

	std::vector<VkSemaphore> imageAvailableSemaphores;
	std::vector<VkSemaphore> renderFinishedSemaphores;

	std::vector<const char*> validationLayers;
	VkDebugUtilsMessengerEXT debugMessenger;

	// used for syncronization objects
	int MAX_FRAMES_IN_FLIGHT;
	size_t currentFrame;
	std::vector<VkFence> inFlightFences;

	// current swapchain image index
	uint32_t imageIndex;
};