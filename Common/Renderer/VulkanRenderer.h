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

struct VertexData
{
	uint64_t size;
	void*	 data;
	uint32_t count;
};

struct IndexData
{
	uint64_t size;
	void*	 data;
	uint32_t count;
};

struct PH_Pipeline
{
	friend class VulkanRenderer;
	VkAttachmentDescription mColorAttachment;
	VkAttachmentDescription mDepthAttachment;
	VkPipelineVertexInputStateCreateInfo mVertexInputState;
	VertexData mVertex;
	IndexData  mIndex;

private:
	VkRenderPass renderPass;
	VkPipelineLayout pipelineLayout;
	VkPipeline mPipeline;
	tinystl::vector<VkFramebuffer> mFramebuffers;

	VkBuffer vertexBuffer;
	VkDeviceMemory vertexBufferMemory;
	VkBuffer indexBuffer;
	VkDeviceMemory indexBufferMemory;
	VkImage depthImage;
	VkDeviceMemory depthImageMemory;
	VkImageView depthImageView;
};

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
	void enableDepth();
	void initVulkan();
	void createGraphicsPipeline(PH_Pipeline&, bool recreate=false);//
	void initVulkan2();//
	void createCommandBuffers(PH_Pipeline&);//
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
	void createRenderPass(PH_Pipeline&);//
	void createDescriptorSetLayout();
	void createCommandPool();
	//void createCommandBuffers(PipelineInfo&);
	void createSyncObjects();

	VkCommandBuffer beginSingleTimeCommands();
	void endSingleTimeCommands(VkCommandBuffer commandBuffer);

	void createDepthResources(PH_Pipeline&);//
	void createFramebuffers(PH_Pipeline&);//

	void createBuffer(VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties, VkBuffer& buffer, VkDeviceMemory& bufferMemory);
	void copyBuffer(VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size);
	void copyBufferToImage(VkBuffer buffer, VkImage image, uint32_t width, uint32_t height);

	void createVertexBuffer(PH_Pipeline&);
	void createIndexBuffer(PH_Pipeline&);
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

	void destroyGraphicsPipeline(PH_Pipeline& mPipelineInfo);
	void destroyInstance();

	bool checkValidationLayerSupport();
	void populateDebugMessengerCreateInfo(VkDebugUtilsMessengerCreateInfoEXT& debugCreateInfo);

	VkInstance instance;

	VkPhysicalDevice physicalDevice = VK_NULL_HANDLE;
	VkDevice device; // logical
	
	bool depthEnabled = false;

	VkQueue graphicsQueue;
	VkQueue presentQueue;
	
	VkSurfaceKHR surface;
	
	VkSwapchainKHR swapChain;
	tinystl::vector<VkImage> swapChainImages;
	tinystl::vector<VkImageView> swapChainImageViews;
	VkFormat swapChainImageFormat;
	VkExtent2D swapChainExtent;
	tinystl::vector<VkShaderModule> shaderModules;
	//VkRenderPass renderPass;

	VkDescriptorSetLayout descriptorSetLayout;
	//VkPipelineLayout pipelineLayout;
	//VkPipeline graphicsPipeline;

	//PH_Pipeline graphicsPipeline; //
	tinystl::vector<PH_Pipeline> mPipelines;//

	//tinystl::vector<VkFramebuffer> swapChainFramebuffers;
	VkCommandPool commandPool;
	tinystl::vector<VkCommandBuffer> commandBuffers;

	//VkBuffer vertexBuffer;
	//VkDeviceMemory vertexBufferMemory;
	//VkBuffer indexBuffer;
	//VkDeviceMemory indexBufferMemory;
	tinystl::vector<VkBuffer> uniformBuffers;
	tinystl::vector<VkDeviceMemory> uniformBuffersMemory;

	VkImage textureImage;
	VkDeviceMemory textureImageMemory;
	VkImageView textureImageView;
	VkSampler textureSampler;

	/*VkImage depthImage;
	VkDeviceMemory depthImageMemory;
	VkImageView depthImageView;*/

	VkDescriptorPool descriptorPool;
	tinystl::vector<VkDescriptorSet> descriptorSets;

	tinystl::vector<VkSemaphore> imageAvailableSemaphores;
	tinystl::vector<VkSemaphore> renderFinishedSemaphores;

	tinystl::vector<const char*> validationLayers;
	VkDebugUtilsMessengerEXT debugMessenger;
};