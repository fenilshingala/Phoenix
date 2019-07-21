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

struct PH_BufferUpdateInfo
{
	VkDescriptorBufferInfo bufferInfo;
};

struct PH_BufferDesc
{
	uint64_t			  mBufferSize;
	VkBufferUsageFlags	  mUsage;
	VkMemoryPropertyFlags mProperties;
};

struct PH_Buffer
{
	friend class VulkanRenderer;
	tinystl::vector<VkBuffer>		mBuffers;
	tinystl::vector<VkDeviceMemory> mBuffersMemory;
	void*  data;
	size_t size;
	PH_BufferDesc bufferDesc;
};

struct PH_TextureUpdateInfo
{
	VkDescriptorImageInfo imageInfo;
};

struct PH_Texture
{
	friend class VulkanRenderer;
	const char*	   filename;
	VkImage		   textureImage;
	VkDeviceMemory textureImageMemory;
	VkImageView	   textureImageView;
};

struct PH_SwapChain
{
	friend class VulkanRenderer;

	VkSwapchainKHR			 swapChain;
	VkExtent2D				 swapChainExtent;
	tinystl::vector<VkImage> swapChainImages;

	PH_BufferUpdateInfo	 bufferUpdateInfo;
	PH_TextureUpdateInfo textureUpdateInfo;

private:
	tinystl::vector<PH_Buffer*>  buffers;
	tinystl::vector<PH_Texture*> textures;
	tinystl::vector<VkImageView> swapChainImageViews;
	VkFormat swapChainImageFormat;
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
	void addSwapChain(PH_SwapChain* swapchain);
	void createGraphicsPipeline(PH_SwapChain* pSwapChain, PH_Pipeline&, bool recreate=false);//
	void initVulkan2(PH_SwapChain* swapchain);//
	void createCommandBuffers(PH_SwapChain&, PH_Pipeline&);//
	void cleanupVulkan();
	void waitDeviceIdle();

	uint32_t acquireNextImage(PH_SwapChain* pSwapChain);
	void drawFrame(PH_SwapChain*, uint32_t);

	void createUniformBuffers(PH_SwapChain*, PH_Buffer*, bool recreate = false);
	void updateUniformBuffer(uint32_t currentImage, PH_Buffer&);

	void createTextureImage(PH_Texture*);
	void createTextureSampler(VkSampler* textureSampler);
	void destroySampler(VkSampler* sampler);

	void createDescriptorSets(PH_SwapChain*);

private:
	int WIDTH;
	int HEIGHT;

	// WINDOW
	const char** getExtensions(uint32_t& _extensionCount);
	void createSurface();
	//void getFramebufferSize(GLFWwindow* window, int* _width, int* _height);
	

	GLFWwindow* window;

	// VULKAN
	void cleanupSwapChain(PH_SwapChain*);
	void recreateSwapChain(PH_SwapChain*);

	void createInstance();
	void pickPhysicalDevice();
	void createLogicalDevice();

	void createSwapChain(PH_SwapChain* swapChain);
	void createImageViews(PH_SwapChain* swapChain);

	VkShaderModule createShaderModule(const tinystl::vector<char>& code);
	void createRenderPass(PH_Pipeline&);//
	void createDescriptorSetLayout();
	void createCommandPool();
	//void createCommandBuffers(PipelineInfo&);
	void createSyncObjects();

	VkCommandBuffer beginSingleTimeCommands();
	void endSingleTimeCommands(VkCommandBuffer commandBuffer);

	void createDepthResources(PH_SwapChain*, PH_Pipeline&);//
	void createFramebuffers(PH_SwapChain*, PH_Pipeline&);//

	void createBuffer(VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties, VkBuffer& buffer, VkDeviceMemory& bufferMemory);
	void copyBuffer(VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size);
	void copyBufferToImage(VkBuffer buffer, VkImage image, uint32_t width, uint32_t height);

	void createVertexBuffer(PH_Pipeline&);
	void createIndexBuffer(PH_Pipeline&);
	/*void createUniformBuffers(PH_SwapChain*, PH_BufferDesc&, PH_Buffer&);
	void updateUniformBuffer(uint32_t currentImage, PH_Buffer&);*/
	
	void createImage(uint32_t width, uint32_t height, VkFormat format, VkImageTiling tiling, VkImageUsageFlags usage,
		VkMemoryPropertyFlags properties, VkImage& image, VkDeviceMemory& imageMemory);
	VkImageView createImageView(VkImage image, VkFormat format, VkImageAspectFlags aspectFlags);
	void transitionImageLayout(VkImage image, VkFormat format, VkImageLayout oldLayout, VkImageLayout newLayout);

	void createTextureImageView(PH_Texture*);

	void createDescriptorPool(PH_SwapChain*);

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
	
	
	tinystl::vector<PH_SwapChain*> swapChains;
	//VkSwapchainKHR swapChain;
	//tinystl::vector<VkImage> swapChainImages;
	//tinystl::vector<VkImageView> swapChainImageViews;
	VkFormat swapChainImageFormat;
	//VkExtent2D swapChainExtent;
	tinystl::vector<VkShaderModule> shaderModules;

	VkDescriptorSetLayout descriptorSetLayout;

	tinystl::vector<PH_Pipeline> mPipelines;//

	VkCommandPool commandPool;
	tinystl::vector<VkCommandBuffer> commandBuffers;

	//tinystl::vector<VkBuffer> uniformBuffers;
	//tinystl::vector<VkDeviceMemory> uniformBuffersMemory;

	/*VkImage textureImage;
	VkDeviceMemory textureImageMemory;
	VkImageView textureImageView;*/
	//VkSampler textureSampler;

	VkDescriptorPool descriptorPool;
	tinystl::vector<VkDescriptorSet> descriptorSets;

	tinystl::vector<VkSemaphore> imageAvailableSemaphores;
	tinystl::vector<VkSemaphore> renderFinishedSemaphores;

	tinystl::vector<const char*> validationLayers;
	VkDebugUtilsMessengerEXT debugMessenger;
};