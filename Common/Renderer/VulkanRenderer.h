#pragma once

#include <vulkan/vulkan.h>
#include "keyBindings.h"

#include <cstdlib> // EXIT_SUCCESS, EXIT_FAILURE
#include <iostream>

#include <TINYSTL/vector.h>
#include <TINYSTL/string.h>

//struct GLFWwindow;
struct SDL_window;
//typedef void(keyCallBackSignature)(GLFWwindow*, int, int, int, int);
//typedef void(resizeCallBackSignature)(GLFWwindow*, int, int);

class VulkanRenderer
{
public:
	VulkanRenderer() :
	WIDTH(800), HEIGHT(600), suitableWidth(800), suitableHeight(600), exitProgram(false), window(NULL), framebufferResized(false), framebufferMinimized(false),
	instance(VK_NULL_HANDLE), physicalDevice(VK_NULL_HANDLE), device(VK_NULL_HANDLE), depthEnabled(false),
	graphicsQueue(VK_NULL_HANDLE), presentQueue(VK_NULL_HANDLE), surface(VK_NULL_HANDLE),
	swapChain(VK_NULL_HANDLE), swapChainImages(), swapChainImageViews(), swapChainImageFormat(VK_FORMAT_UNDEFINED), swapChainExtent({800, 600}),
	shaderModules(), renderPass(VK_NULL_HANDLE), descriptorSetLayout(VK_NULL_HANDLE), pipelineLayout(VK_NULL_HANDLE), graphicsPipeline(VK_NULL_HANDLE),
	swapChainFramebuffers(), commandPool(VK_NULL_HANDLE), commandBuffers(), vertexBuffer(VK_NULL_HANDLE), vertexBufferMemory(VK_NULL_HANDLE),
	indexBuffer(VK_NULL_HANDLE), indexBufferMemory(VK_NULL_HANDLE), uniformBuffers(VK_NULL_HANDLE), uniformBuffersMemory(VK_NULL_HANDLE),
	textureImage(VK_NULL_HANDLE), textureImageMemory(VK_NULL_HANDLE), textureImageView(VK_NULL_HANDLE), textureSampler(VK_NULL_HANDLE),
	depthImage(VK_NULL_HANDLE), depthImageMemory(VK_NULL_HANDLE), depthImageView(VK_NULL_HANDLE),
	descriptorPool(VK_NULL_HANDLE), descriptorSets(), imageAvailableSemaphores(), renderFinishedSemaphores(), validationLayers(),
	debugMessenger(VK_NULL_HANDLE)
	{
		validationLayers.emplace_back("VK_LAYER_LUNARG_standard_validation");
	}

	~VulkanRenderer()
	{
		validationLayers.clear();
	}

	// WINDOW
	void initWindow();
	void pollEvents();
	void waitEvents();
	void updateInputs();
	bool isKeyPressed(uint32_t _key);
	bool isKeyTriggered(uint32_t _key);

	int windowShouldClose();
	void destroyWindow();

	// VULKAN
	void enableDepth();
	void initVulkan();
	void cleanupVulkan();
	void drawFrame();
	void waitDeviceIdle();

private:
	int WIDTH;
	int HEIGHT;
	int suitableWidth;
	int suitableHeight;
	bool exitProgram;

	// WINDOW
	void getExtensions(uint32_t& _extensionCount, const char*** pExtensionNames);
	void createSurface();
	void* window;
	bool framebufferResized;
	bool framebufferMinimized;

	// VULKAN
	void cleanupSwapChain();
	void recreateSwapChain();

	void createInstance();
	void pickPhysicalDevice();
	void createLogicalDevice();
	void createSwapChain();
	void createImageViews();
	VkShaderModule createShaderModule(const tinystl::vector<char>& code);
	void createRenderPass();
	void createDescriptorSetLayout();
	void createGraphicsPipeline();
	void createCommandPool();
	void createCommandBuffers();
	void createSyncObjects();

	VkCommandBuffer beginSingleTimeCommands();
	void endSingleTimeCommands(VkCommandBuffer commandBuffer);

	void createDepthResources();
	void createFramebuffers();

	void createBuffer(VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties, VkBuffer& buffer, VkDeviceMemory& bufferMemory);
	void copyBuffer(VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size);
	void copyBufferToImage(VkBuffer buffer, VkImage image, uint32_t width, uint32_t height);

	void createVertexBuffer();
	void createIndexBuffer();
	void createUniformBuffers();
	void updateUniformBuffer(uint32_t currentImage);
	
	void createImage(uint32_t width, uint32_t height, VkFormat format, VkImageTiling tiling, VkImageUsageFlags usage,
		VkMemoryPropertyFlags properties, VkImage& image, VkDeviceMemory& imageMemory);
	VkImageView createImageView(VkImage image, VkFormat format, VkImageAspectFlags aspectFlags);
	void transitionImageLayout(VkImage image, VkFormat format, VkImageLayout oldLayout, VkImageLayout newLayout);

	void createTextureImage();
	void createTextureImageView();
	void createTextureSampler();

	void createDescriptorPool();
	void createDescriptorSets();

	void destroyInstance();

	bool checkValidationLayerSupport();
	void populateDebugMessengerCreateInfo(VkDebugUtilsMessengerCreateInfoEXT& debugCreateInfo);

	VkInstance instance;

	VkPhysicalDevice physicalDevice;
	VkDevice device; // logical
	
	bool depthEnabled;

	VkQueue graphicsQueue;
	VkQueue presentQueue;
	
	VkSurfaceKHR surface;
	
	VkSwapchainKHR swapChain;
	tinystl::vector<VkImage> swapChainImages;
	tinystl::vector<VkImageView> swapChainImageViews;
	VkFormat swapChainImageFormat;
	VkExtent2D swapChainExtent;
	tinystl::vector<VkShaderModule> shaderModules;
	VkRenderPass renderPass;

	VkDescriptorSetLayout descriptorSetLayout;
	VkPipelineLayout pipelineLayout;
	VkPipeline graphicsPipeline;

	tinystl::vector<VkFramebuffer> swapChainFramebuffers;
	VkCommandPool commandPool;
	tinystl::vector<VkCommandBuffer> commandBuffers;

	VkBuffer vertexBuffer;
	VkDeviceMemory vertexBufferMemory;
	VkBuffer indexBuffer;
	VkDeviceMemory indexBufferMemory;
	tinystl::vector<VkBuffer> uniformBuffers;
	tinystl::vector<VkDeviceMemory> uniformBuffersMemory;

	VkImage textureImage;
	VkDeviceMemory textureImageMemory;
	VkImageView textureImageView;
	VkSampler textureSampler;

	VkImage depthImage;
	VkDeviceMemory depthImageMemory;
	VkImageView depthImageView;

	VkDescriptorPool descriptorPool;
	tinystl::vector<VkDescriptorSet> descriptorSets;

	tinystl::vector<VkSemaphore> imageAvailableSemaphores;
	tinystl::vector<VkSemaphore> renderFinishedSemaphores;

	tinystl::vector<const char*> validationLayers;
	VkDebugUtilsMessengerEXT debugMessenger;
};