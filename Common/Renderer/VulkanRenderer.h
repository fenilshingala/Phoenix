#pragma once

#include <vulkan/vulkan.h>
#include "keyBindings.h"

#include <vector>
#include <string>

struct SDL_window;

struct PH_Image
{
	VkImage			image;
	VkDeviceMemory	imageMemory;
	VkImageView		imageView;
	std::string		path;
	int				width;
	int				height;
	int				nChannels;
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
	VulkanRenderer() :
	WIDTH(800), HEIGHT(600), suitableWidth(800), suitableHeight(600), exitProgram(false), window(NULL), framebufferResized(false), framebufferMinimized(false),
	instance(VK_NULL_HANDLE), physicalDevice(VK_NULL_HANDLE), device(VK_NULL_HANDLE), graphicsQueue(VK_NULL_HANDLE), presentQueue(VK_NULL_HANDLE), surface(VK_NULL_HANDLE),
	swapChain(VK_NULL_HANDLE), swapChainImages(), swapChainImageViews(), swapChainImageFormat(VK_FORMAT_UNDEFINED), swapChainExtent({800, 600}),
	renderPass(VK_NULL_HANDLE), swapChainFramebuffers(), commandPool(VK_NULL_HANDLE), commandBuffers(),
	depthImage(VK_NULL_HANDLE), depthImageMemory(VK_NULL_HANDLE), depthImageView(VK_NULL_HANDLE),
	imageAvailableSemaphores(), renderFinishedSemaphores(), validationLayers(), debugMessenger(VK_NULL_HANDLE),
	MAX_FRAMES_IN_FLIGHT(2), currentFrame(0), inFlightFences(VK_NULL_HANDLE), imageIndex(0)
	{
		validationLayers.emplace_back("VK_LAYER_LUNARG_standard_validation");
	}

	~VulkanRenderer()
	{
		validationLayers.clear();
	}

	// WINDOW
	void initWindow();
	void destroyWindow();
	void pollEvents();
	bool isKeyPressed(const uint32_t _key);
	bool isKeyTriggered(const uint32_t _key);
	int windowShouldClose() const;

	// VULKAN
	void initVulkan();
	void cleanupVulkan();
	void waitDeviceIdle();
	
	// TEXTURES
	void PH_CreateTexture(PH_Image& ph_image);
	void PH_DeleteTexture(PH_Image& ph_image);

	// BUFFERS
	void PH_CreateBuffer(PH_BufferCreateInfo info, PH_Buffer& ph_buffer);
	void PH_DeleteBuffer(PH_Buffer& ph_buffer);
	void PH_UpdateBuffer(PH_BufferUpdateInfo info);
	
	// SAMPLERS
	VkSampler PH_CreateSampler();
	void PH_DeleteSampler(VkSampler& sampler);
	
	// SHADER MODULES
	VkShaderModule PH_CreateShaderModule(const char* path);
	void PH_DestroyShaderModule(VkShaderModule& shaderModule);
	
	// DescriptorSet Layout
	void PH_CreateDescriptorSetLayout(VkDescriptorSetLayoutCreateInfo layoutInfo, VkDescriptorSetLayout& descriptorSetLayout);
	void PH_DeleteDescriptorSetLayout(VkDescriptorSetLayout& descriptorSetLayout);

	// Pipeline Layout
	void PH_CreatePipelineLayout(VkPipelineLayoutCreateInfo pipelineLayoutInfo, VkPipelineLayout& pipelineLayout);
	void PH_DeletePipelineLayout(VkPipelineLayout& pipelineLayout);

	// Graphics Pipeline
	void PH_CreateGraphicsPipeline(VkGraphicsPipelineCreateInfo pipelineInfo, VkPipeline& pipelineLayout);
	void PH_DeleteGraphicsPipeline(VkPipeline& graphicsPipeline);
	
	// Descriptors
	void PH_CreateDescriptorPool(VkDescriptorPoolCreateInfo descriptorSetLayout, VkDescriptorPool& descriptorPool);
	void PH_DeleteDescriptorPool(VkDescriptorPool& descriptorPool);
	void PH_CreateDescriptorSets(VkDescriptorSetLayout descriptorSetLayout, uint32_t descriptorSetCount, VkDescriptorPool descriptorPool, std::vector<VkDescriptorSet>& descriptorSets);
	void PH_UpdateDeDescriptorSets(std::vector<VkWriteDescriptorSet> descriptorWrites);
	
	const uint32_t PH_PrepareNextFrame();
	void PH_SubmitFrame();

	// GETTERS
	inline const int GetNoOfSwapChains();
	inline const VkExtent2D GetSwapChainExtent();
	inline VkRenderPass GetDefaultRenderPass();
	inline VkFramebuffer* GetSwapChainFrameBuffers();
	inline std::vector<VkCommandBuffer>& GetCommandBuffers();

	// VIRTUALS
	virtual void createRenderPass();				// can be overriden by app
	virtual void createDescriptorSetLayout() = 0;
	virtual void createGraphicsPipeline() = 0;
	virtual void createDescriptorPool() = 0;
	virtual void createDescriptorSets() = 0;
	virtual void Init() = 0;
	virtual void Exit() = 0;
	virtual void Load() = 0;
	virtual void UnLoad() = 0;
	virtual void RecordCommandBuffers() = 0;
	virtual void DrawFrame() = 0;

private:
	// WINDOW
	int WIDTH;
	int HEIGHT;
	int suitableWidth;
	int suitableHeight;
	bool exitProgram;

	void getExtensions(uint32_t& _extensionCount, const char*** pExtensionNames);
	void createSurface();
	void waitEvents();
	void* window;
	bool framebufferResized;
	bool framebufferMinimized;

	// VULKAN
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

	VkCommandBuffer beginSingleTimeCommands();
	void endSingleTimeCommands(VkCommandBuffer commandBuffer);

	void createDepthResources();
	void createFramebuffers();

	void createBuffer(VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties, VkBuffer& buffer, VkDeviceMemory& bufferMemory);
	void copyBuffer(VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size);
	void copyBufferToImage(VkBuffer buffer, VkImage image, uint32_t width, uint32_t height);

	void createImage(uint32_t width, uint32_t height, VkFormat format, VkImageTiling tiling, VkImageUsageFlags usage,
		VkMemoryPropertyFlags properties, VkImage& image, VkDeviceMemory& imageMemory);
	VkImageView createImageView(VkImage image, VkFormat format, VkImageAspectFlags aspectFlags);
	void transitionImageLayout(VkImage image, VkFormat format, VkImageLayout oldLayout, VkImageLayout newLayout);

	void destroyInstance();

	bool checkValidationLayerSupport();
	void populateDebugMessengerCreateInfo(VkDebugUtilsMessengerCreateInfoEXT& debugCreateInfo);

	VkInstance instance;

	VkPhysicalDevice physicalDevice;
	VkDevice device; // logical
	
	VkQueue graphicsQueue;
	VkQueue presentQueue;
	
	VkSurfaceKHR surface;
	
	VkSwapchainKHR swapChain;
	std::vector<VkImage> swapChainImages;
	std::vector<VkImageView> swapChainImageViews;
	VkFormat swapChainImageFormat;
	VkExtent2D swapChainExtent;
	VkRenderPass renderPass;

	std::vector<VkFramebuffer> swapChainFramebuffers;
	VkCommandPool commandPool;
	std::vector<VkCommandBuffer> commandBuffers;

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

inline const VkExtent2D VulkanRenderer::GetSwapChainExtent()
{
	return swapChainExtent;
}

inline VkRenderPass VulkanRenderer::GetDefaultRenderPass()
{
	return renderPass;
}

inline const int VulkanRenderer::GetNoOfSwapChains()
{
	return (int)swapChainImages.size();
}

inline VkFramebuffer* VulkanRenderer::GetSwapChainFrameBuffers()
{
	return swapChainFramebuffers.data();
}

inline std::vector<VkCommandBuffer>& VulkanRenderer::GetCommandBuffers()
{
	return commandBuffers;
}