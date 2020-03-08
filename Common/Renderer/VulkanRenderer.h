#pragma once

#include <vulkan/vulkan.h>

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <assimp/Importer.hpp> 
#include <assimp/scene.h>     
#include <assimp/postprocess.h>
#include <assimp/cimport.h>

#include <vector>
#include <unordered_map>
#include <string>

#include "Window.h"
#include "Camera.hpp"

struct Settings
{
	int				windowWidth	 = 0;
	int				windowHeight = 0;
	std::string		app_name;
	uint32_t		maxInFlightFrames = 2;
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

	VkSampler			sampler; // for combined image sampler
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


typedef enum Vertex_Component {
	VERTEX_COMPONENT_POSITION = 0x0,
	VERTEX_COMPONENT_NORMAL = 0x1,
	VERTEX_COMPONENT_COLOR = 0x2,
	VERTEX_COMPONENT_UV = 0x3,
	VERTEX_COMPONENT_TANGENT = 0x4,
	VERTEX_COMPONENT_BITANGENT = 0x5,
	VERTEX_COMPONENT_DUMMY_FLOAT = 0x6,
	VERTEX_COMPONENT_DUMMY_VEC4 = 0x7
} Vertex_Component;

struct VertexLayout {
public:
	/** @brief Components used to generate vertices from */
	std::vector<Vertex_Component> components;

	VertexLayout(std::vector<Vertex_Component> components)
	{
		this->components = std::move(components);
	}

	uint32_t stride()
	{
		uint32_t res = 0;
		for (auto& component : components)
		{
			switch (component)
			{
			case VERTEX_COMPONENT_UV:
				res += 2 * sizeof(float);
				break;
			case VERTEX_COMPONENT_DUMMY_FLOAT:
				res += sizeof(float);
				break;
			case VERTEX_COMPONENT_DUMMY_VEC4:
				res += 4 * sizeof(float);
				break;
			default:
				// All components except the ones listed above are made up of 3 floats
				res += 3 * sizeof(float);
			}
		}
		return res;
	}
};


struct PH_Model
{
public:
	PH_Buffer vertices;
	PH_Buffer indices;
	uint32_t indexCount = 0;
	uint32_t vertexCount = 0;

	struct ModelPart
	{
		uint32_t vertexBase;
		uint32_t vertexCount;
		uint32_t indexBase;
		uint32_t indexCount;
		int		 materialIndex;
	};
	std::vector<ModelPart> parts;
	std::unordered_map<uint32_t, std::vector<PH_Image>> mMeshTexturesMap;
	std::vector<PH_Image> textures_loaded;
	std::string directory;

	static const int defaultFlags = aiProcess_FlipWindingOrder | aiProcess_Triangulate | aiProcess_PreTransformVertices | aiProcess_CalcTangentSpace | aiProcess_GenSmoothNormals;

	struct Dimension
	{
		glm::vec3 min = glm::vec3(FLT_MAX);
		glm::vec3 max = glm::vec3(-FLT_MAX);
		glm::vec3 size;
	} dim;
};

class VulkanRenderer
{
public:
	VulkanRenderer();
	virtual ~VulkanRenderer();

	void initRenderer(Settings settings);
	void exitRenderer();

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

	// MODELS
	void PH_LoadModel(const std::string& filename, VertexLayout layout, PH_Model* ph_model);
	void PH_DeleteModel(PH_Model* ph_model);
	std::vector<PH_Image> InitMaterials(const aiMaterial* material, aiTextureType type, std::string typeName, PH_Model* ph_model);

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
	virtual void createDescriptorPool() = 0;
	virtual void createDescriptorSets() = 0;
	virtual void RecordCommandBuffers() = 0;
	virtual void DrawFrame() = 0;
	virtual void Init() = 0;
	virtual void Exit() = 0;
	virtual void Load() = 0;
	virtual void UnLoad() = 0;

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

	std::string app_name;
};