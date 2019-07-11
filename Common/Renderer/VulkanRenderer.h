#pragma once

#include <vulkan/vulkan.h>
#include "keyBindings.h"

#include <cstdlib> // EXIT_SUCCESS, EXIT_FAILURE
#include <iostream>

#include <TINYSTL/vector.h>
#include <TINYSTL/string.h>

struct GLFWwindow;
typedef void(keyCallBackSignature)(GLFWwindow*, int, int, int, int);
typedef void(resizeCallBackSignature)(GLFWwindow*, int, int);

class VulkanRenderer
{
public:
	VulkanRenderer() : WIDTH(800), HEIGHT(600), window(nullptr), validationLayers()
	{
		validationLayers.emplace_back("VK_LAYER_LUNARG_standard_validation");
	}
	~VulkanRenderer()
	{
		validationLayers.clear();
	}

	// WINDOW
	void initWindow(int _WIDTH = 800, int _HEIGHT = 600);
	void setFramebufferSizeCallback(resizeCallBackSignature&);
	void setKeyCallback(keyCallBackSignature&);
	void pollEvents();
	void waitEvents();
	void updateInputs();
	bool isKeyPressed(uint32_t _key);
	bool isKeyTriggered(uint32_t _key);

	int windowShouldClose();
	void destroyWindow();

	// VULKAN
	void initVulkan();
	void cleanupVulkan();
	void drawFrame();
	void waitDeviceIdle();

private:
	int WIDTH;
	int HEIGHT;

	// WINDOW
	const char** getExtensions(uint32_t& _extensionCount);
	void createSurface();
	//void getFramebufferSize(GLFWwindow* window, int* _width, int* _height);
	

	GLFWwindow* window;

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
	void createFramebuffers();
	void createCommandPool();
	void createCommandBuffers();
	void createSyncObjects();

	void createBuffer(VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties, VkBuffer& buffer, VkDeviceMemory& bufferMemory);
	void copyBuffer(VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size);
	void createVertexBuffer();
	void createIndexBuffer();
	void createUniformBuffers();
	void updateUniformBuffer(uint32_t currentImage);
	
	void createDescriptorPool();
	void createDescriptorSets();

	void destroyInstance();

	bool checkValidationLayerSupport();
	void populateDebugMessengerCreateInfo(VkDebugUtilsMessengerCreateInfoEXT& debugCreateInfo);

	VkInstance instance;

	VkPhysicalDevice physicalDevice = VK_NULL_HANDLE;
	VkDevice device; // logical
	
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

	VkDescriptorPool descriptorPool;
	tinystl::vector<VkDescriptorSet> descriptorSets;

	tinystl::vector<VkSemaphore> imageAvailableSemaphores;
	tinystl::vector<VkSemaphore> renderFinishedSemaphores;

	tinystl::vector<const char*> validationLayers;
	VkDebugUtilsMessengerEXT debugMessenger;
};