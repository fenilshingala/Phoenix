#include "VulkanRenderer.h"

#include <stdexcept>
#include <functional>
#include <fstream>
#include <algorithm> // max min
#include <thread>
#include <vector>
#include <iostream>
#include <iomanip>

#include <unordered_map>
#include <unordered_set>

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

////////////////////////////////////////////////////
////////////////////// VULKAN //////////////////////

const char* deviceExtensions[] = {
	VK_KHR_SWAPCHAIN_EXTENSION_NAME,
	VK_KHR_GET_MEMORY_REQUIREMENTS_2_EXTENSION_NAME,
	VK_NV_RAY_TRACING_EXTENSION_NAME
};

VulkanRenderer::VulkanRenderer() :
pWindow(NULL), instance(VK_NULL_HANDLE), physicalDevice(VK_NULL_HANDLE), device(VK_NULL_HANDLE), graphicsQueue(VK_NULL_HANDLE), presentQueue(VK_NULL_HANDLE), surface(VK_NULL_HANDLE),
swapChain(VK_NULL_HANDLE), swapChainImages(), swapChainImageViews(), swapChainImageFormat(VK_FORMAT_UNDEFINED), swapChainExtent({ 800, 600 }),
renderPass(VK_NULL_HANDLE), swapChainFramebuffers(), commandPool(VK_NULL_HANDLE), commandBuffers(),
depthImage(VK_NULL_HANDLE), depthImageMemory(VK_NULL_HANDLE), depthImageView(VK_NULL_HANDLE),
imageAvailableSemaphores(), renderFinishedSemaphores(), validationLayers(), debugMessenger(VK_NULL_HANDLE),
MAX_FRAMES_IN_FLIGHT(2), currentFrame(0), inFlightFences(VK_NULL_HANDLE), imageIndex(0)
{
	validationLayers.emplace_back("VK_LAYER_KHRONOS_validation");
	//validationLayers.emplace_back("VK_LAYER_LUNARG_standard_validation");
	//validationLayers.emplace_back("VK_LAYER_RENDERDOC_Capture");
}

VulkanRenderer::~VulkanRenderer()
{
	validationLayers.clear();
}

void VulkanRenderer::initRenderer(Settings settings)
{
	MAX_FRAMES_IN_FLIGHT = settings.maxInFlightFrames;
	app_name = settings.app_name;

	pWindow = new Window();
	if (!app_name.empty())
	{
		pWindow->setWindowTitle(app_name.c_str());
	}
	if (settings.windowWidth != 0 && settings.windowHeight != 0)
	{
		pWindow->setWindowWidth(settings.windowWidth);
		pWindow->setWindowHeight(settings.windowHeight);
	}
	pWindow->initWindow();
	initVulkan();
}

void VulkanRenderer::exitRenderer()
{
	cleanupVulkan();
	pWindow->exitWindow();
}

void VulkanRenderer::initVulkan()
{
	createInstance();
	pWindow->CreateVulkanSurface(instance, &surface);
	pickPhysicalDevice();		// implicitly destroyed with instance
	createLogicalDevice();
	createSwapChain();
	createImageViews();			// swapchain image views
	createRenderPass();
	createCommandPool();
	createDepthResources();
	createFramebuffers();
	createSyncObjects();
	createCommandBuffers();
}

void VulkanRenderer::cleanupVulkan()
{
	for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
	{
		vkDestroySemaphore(device, renderFinishedSemaphores[i], nullptr);
		vkDestroySemaphore(device, imageAvailableSemaphores[i], nullptr);
		vkDestroyFence(device, inFlightFences[i], nullptr);
	}
	for (VkFramebuffer framebuffer : swapChainFramebuffers)
	{
		vkDestroyFramebuffer(device, framebuffer, nullptr);
	}
	vkDestroyImageView(device, depthImageView, nullptr);
	vkDestroyImage(device, depthImage, nullptr);
	vkFreeMemory(device, depthImageMemory, nullptr);
	vkFreeCommandBuffers(device, commandPool, static_cast<uint32_t>(commandBuffers.size()), commandBuffers.data());
	vkDestroyCommandPool(device, commandPool, nullptr);
	vkDestroyRenderPass(device, renderPass, nullptr);
	for (VkImageView imageView : swapChainImageViews)
	{
		vkDestroyImageView(device, imageView, nullptr);
	}
	vkDestroySwapchainKHR(device, swapChain, nullptr);
	vkDestroyDevice(device, nullptr);
	vkDestroySurfaceKHR(instance, surface, nullptr);
	destroyInstance();
}


void VulkanRenderer::recreateSwapChain()
{
	while (!pWindow->waitForEvents())
	{
		std::this_thread::sleep_for(std::chrono::milliseconds(1000));
	}
	
	vkDeviceWaitIdle(device);
	cleanupSwapChain();

	// Recreate
	createSwapChain();
	createImageViews();
	createRenderPass();
	createDepthResources();
	createFramebuffers();
	createCommandBuffers();

	Load();		// App override
}

void VulkanRenderer::cleanupSwapChain()
{
	UnLoad();	// App override

	vkFreeCommandBuffers(device, commandPool, static_cast<uint32_t>(commandBuffers.size()), commandBuffers.data());
	for (size_t i = 0; i < swapChainFramebuffers.size(); i++)
	{
		vkDestroyFramebuffer(device, swapChainFramebuffers[i], nullptr);
	}
	vkDestroyImageView(device, depthImageView, nullptr);
	vkDestroyImage(device, depthImage, nullptr);
	vkFreeMemory(device, depthImageMemory, nullptr);
	vkDestroyRenderPass(device, renderPass, nullptr);
	for (size_t i = 0; i < swapChainImageViews.size(); i++)
	{
		vkDestroyImageView(device, swapChainImageViews[i], nullptr);
	}
	vkDestroySwapchainKHR(device, swapChain, nullptr);
}

bool VulkanRenderer::checkValidationLayerSupport()
{
	uint32_t layerCount;
	vkEnumerateInstanceLayerProperties(&layerCount, nullptr);

	std::vector<VkLayerProperties> availableLayers(layerCount);
	vkEnumerateInstanceLayerProperties(&layerCount, availableLayers.data());

	for (const char* layerName : validationLayers)
	{
		bool layerFound = false;

		for (const VkLayerProperties& layerProperties : availableLayers)
		{
			if (strcmp(layerName, layerProperties.layerName) == 0)
			{
				layerFound = true;
				break;
			}
		}

		if (!layerFound)
		{
			return false;
		}
	}

	return true;
}


static VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(
	VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
	VkDebugUtilsMessageTypeFlagsEXT messageType,
	const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
	void* pUserData
	)
{
	if (messageSeverity >= VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT)
	{
		std::cerr << "validation layer: " << pCallbackData->pMessage << std::endl;
	}

	return VK_FALSE;
}

void VulkanRenderer::populateDebugMessengerCreateInfo(VkDebugUtilsMessengerCreateInfoEXT& debugCreateInfo)
{
	debugCreateInfo = {};
	debugCreateInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
	debugCreateInfo.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT; //VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT
	debugCreateInfo.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
	debugCreateInfo.pfnUserCallback = debugCallback;
	debugCreateInfo.pUserData = nullptr; // Optional
}

void VulkanRenderer::createInstance()
{
#ifdef _DEBUG
	if (!checkValidationLayerSupport())
	{
		throw std::runtime_error("validation layers requested, but not available!");
	}
#endif

	VkApplicationInfo appInfo = {};
	appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
	appInfo.pApplicationName = app_name.c_str();
	appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
	appInfo.pEngineName = "No Engine";
	appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
	appInfo.apiVersion = VK_API_VERSION_1_0;

	VkInstanceCreateInfo createInfo = {};
	createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
	createInfo.pApplicationInfo = &appInfo;
	

	// required extensions
	uint32_t extensionCount = 0;
	const char** extensions = NULL;
	pWindow->GetVulkanExtensions(&extensionCount, &extensions);

	std::vector<const char*> requiredExtensions(extensions, extensions + extensionCount);

#ifdef _DEBUG
	requiredExtensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
#endif

	// RAY TRACING
	requiredExtensions.push_back(VK_KHR_GET_PHYSICAL_DEVICE_PROPERTIES_2_EXTENSION_NAME);

	createInfo.enabledExtensionCount = static_cast<uint32_t>(requiredExtensions.size());
	createInfo.ppEnabledExtensionNames = requiredExtensions.data();

	// check for available extensions
	uint32_t availableExtensionCount = 0;
	vkEnumerateInstanceExtensionProperties(nullptr, &availableExtensionCount, nullptr);
	std::vector<VkExtensionProperties> availableExtensions(availableExtensionCount);
	vkEnumerateInstanceExtensionProperties(nullptr, &availableExtensionCount, availableExtensions.data());
	// end


#ifdef _DEBUG
	// validation layers
	createInfo.enabledLayerCount = static_cast<uint32_t>(validationLayers.size());
	createInfo.ppEnabledLayerNames = validationLayers.data();

	// debug messenger
	//VkDebugUtilsMessengerCreateInfoEXT preDebugCreateInfo;
	//populateDebugMessengerCreateInfo(preDebugCreateInfo);
	//createInfo.pNext = &preDebugCreateInfo;
#else
		createInfo.enabledLayerCount = 0;
		createInfo.pNext = nullptr;
#endif

	VkResult result = vkCreateInstance(&createInfo, nullptr, &instance);
	if (vkCreateInstance(&createInfo, nullptr, &instance) != VK_SUCCESS)
	{
		throw std::runtime_error("failed to create instance!");
	}


#ifdef _DEBUG
	VkDebugUtilsMessengerCreateInfoEXT debugCreateInfo;
	populateDebugMessengerCreateInfo(debugCreateInfo);
	PFN_vkCreateDebugUtilsMessengerEXT func = (PFN_vkCreateDebugUtilsMessengerEXT)vkGetInstanceProcAddr(instance, "vkCreateDebugUtilsMessengerEXT");
	if (func != nullptr)
	{
		VkResult result = func(instance, &debugCreateInfo, nullptr, &debugMessenger);
		if (result != VK_SUCCESS)
		{
			throw std::runtime_error("failed to set up debug messenger!");
		}
	}
	else
	{
		throw std::runtime_error("failed to set up debug messenger!");
	}

	createInfo.pNext = (VkDebugUtilsMessengerCreateInfoEXT*)&debugCreateInfo;
#endif
}

void VulkanRenderer::destroyInstance()
{
#ifdef _DEBUG
	PFN_vkDestroyDebugUtilsMessengerEXT func = (PFN_vkDestroyDebugUtilsMessengerEXT)vkGetInstanceProcAddr(instance, "vkDestroyDebugUtilsMessengerEXT");
	if (func != nullptr)
	{
		func(instance, debugMessenger, nullptr);
	}
#endif

	vkDestroyInstance(instance, nullptr);
}


static int max_device_score = 0;

struct QueueFamilyIndices
{
	uint32_t graphicsFamily;
	uint32_t presentFamily;
};
static struct QueueFamilyIndices familyIndices = { -1, -1 };

struct SwapChainSupportDetails {
	VkSurfaceCapabilitiesKHR capabilities;
	std::vector<VkSurfaceFormatKHR> formats;
	std::vector<VkPresentModeKHR> presentModes;
};

SwapChainSupportDetails querySwapChainSupport(const VkPhysicalDevice& device, const VkSurfaceKHR& surface)
{
	SwapChainSupportDetails details;

	vkGetPhysicalDeviceSurfaceCapabilitiesKHR(device, surface, &details.capabilities);
	
	uint32_t formatCount;
	vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &formatCount, nullptr);

	if (formatCount != 0) {
		details.formats.resize(formatCount);
		vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &formatCount, details.formats.data());
	}

	uint32_t presentModeCount;
	vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface, &presentModeCount, nullptr);

	if (presentModeCount != 0) {
		details.presentModes.resize(presentModeCount);
		vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface, &presentModeCount, details.presentModes.data());
	}

	return details;
}

int rateDeviceSuitability(const VkPhysicalDevice& device, const VkSurfaceKHR& surface)
{
	VkPhysicalDeviceProperties deviceProperties;
	VkPhysicalDeviceFeatures deviceFeatures;
	vkGetPhysicalDeviceProperties(device, &deviceProperties);
	vkGetPhysicalDeviceFeatures(device, &deviceFeatures);

	// Application can't function without geometry shaders
	if (!deviceFeatures.geometryShader || !deviceFeatures.samplerAnisotropy)
	{
		return 0;
	}


	int score = 0;

	// Discrete GPUs have a significant performance advantage
	if (deviceProperties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU)
	{
		score += 1000;
	}

	// Maximum possible size of textures affects graphics quality
	score += deviceProperties.limits.maxImageDimension2D;


	// QUEUE FAMILIES CHECK
	uint32_t queueFamilyCount = 0;
	vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, nullptr);

	std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
	vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, queueFamilies.data());

	int i = 0;
	for (const VkQueueFamilyProperties& queueFamily : queueFamilies)
	{
		// GRAPHICS FAMILY
		if (queueFamily.queueCount > 0 && queueFamily.queueFlags & VK_QUEUE_GRAPHICS_BIT)
		{
			familyIndices.graphicsFamily = i;
		}
		
		// PRESENTATION FAMILY
		VkBool32 presentSupport = false;
		vkGetPhysicalDeviceSurfaceSupportKHR(device, i, surface, &presentSupport);
		if (queueFamily.queueCount > 0 && presentSupport)
		{
			familyIndices.presentFamily = i;
		}
		break;

		++i;
	}
	// END QUEUE FAMILIES CHECK


	// device extension support
	uint32_t extensionCount;
	vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, nullptr);

	std::vector<VkExtensionProperties> availableExtensions(extensionCount);
	vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, availableExtensions.data());

	std::unordered_set<std::string> requiredExtensions;
	int size = sizeof(deviceExtensions) / sizeof(const char*);
	for (int i=0; i < size; ++i)
	{
		requiredExtensions.insert(deviceExtensions[i]);
	}

	for (const auto& extension : availableExtensions) {
		requiredExtensions.erase(extension.extensionName);
	}
	// END device extension support


	// swapchain support
	bool swapChainAdequate = false;
	if (requiredExtensions.empty())
	{
		SwapChainSupportDetails swapChainSupport = querySwapChainSupport(device, surface);
		swapChainAdequate = !swapChainSupport.formats.empty() && !swapChainSupport.presentModes.empty();
	}
	// END swapchain support


	if (-1 == familyIndices.graphicsFamily || -1 == familyIndices.presentFamily || !requiredExtensions.empty() || !swapChainAdequate)
		return 0;

	std::cout << "Device: " << std::setw(20) << deviceProperties.deviceName << "\t" << "Score: " << score << "\n";

	if(max_device_score < score)
		max_device_score = score;

	return score;
}

void VulkanRenderer::pickPhysicalDevice()
{
	uint32_t deviceCount = 0;
	vkEnumeratePhysicalDevices(instance, &deviceCount, nullptr);
	if (deviceCount == 0)
	{
		throw std::runtime_error("failed to find GPUs with Vulkan support!");
	}
	
	std::vector<VkPhysicalDevice> devices(deviceCount);
	vkEnumeratePhysicalDevices(instance, &deviceCount, devices.data());

	std::unordered_map<int, VkPhysicalDevice> candidates;

	for (const VkPhysicalDevice& device : devices)
	{
		int score = rateDeviceSuitability(device, surface);
		candidates.insert(std::pair<int, VkPhysicalDevice>(score, device));
	}
	
	// Check if the best candidate is suitable at all
	if (max_device_score > 0)
	{
		physicalDevice = candidates[max_device_score];
		if (physicalDevice == VK_NULL_HANDLE)
		{
			throw std::runtime_error("failed to find a suitable GPU!");
		}

	}
	else {
		throw std::runtime_error("failed to find a suitable GPU!");
	}

	VkPhysicalDeviceProperties deviceProperties;
	vkGetPhysicalDeviceProperties(physicalDevice, &deviceProperties);
}

void VulkanRenderer::createLogicalDevice()
{
	std::vector<VkDeviceQueueCreateInfo> queueCreateInfos;
	std::unordered_set<uint32_t> uniqueQueueFamilies; 
	uniqueQueueFamilies.insert(familyIndices.graphicsFamily);
	uniqueQueueFamilies.insert(familyIndices.presentFamily);

	float queuePriority = 1.0f;
	for (uint32_t queueFamily : uniqueQueueFamilies)
	{
		VkDeviceQueueCreateInfo queueCreateInfo = {};
		queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
		queueCreateInfo.queueFamilyIndex = queueFamily;
		queueCreateInfo.queueCount = 1;
		queueCreateInfo.pQueuePriorities = &queuePriority;
		queueCreateInfos.push_back(queueCreateInfo);
	}

	// queues
	VkDeviceCreateInfo createInfo = {};
	createInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
	createInfo.queueCreateInfoCount = static_cast<uint32_t>(queueCreateInfos.size());
	createInfo.pQueueCreateInfos = queueCreateInfos.data();

	// features
	VkPhysicalDeviceFeatures deviceFeatures = {};
	deviceFeatures.samplerAnisotropy = VK_TRUE;
	createInfo.pEnabledFeatures = &deviceFeatures;
	
	// extensions
	createInfo.enabledExtensionCount = static_cast<uint32_t>(sizeof(deviceExtensions) / sizeof(const char*));
	createInfo.ppEnabledExtensionNames = deviceExtensions;

	// validation layers
#ifdef _DEBUG
	createInfo.enabledLayerCount = static_cast<uint32_t>(validationLayers.size());
	createInfo.ppEnabledLayerNames = validationLayers.data();
#else
	createInfo.enabledLayerCount = 0;
#endif

	if (vkCreateDevice(physicalDevice, &createInfo, nullptr, &device) != VK_SUCCESS)
	{
		throw std::runtime_error("failed to create logical device!");
	}

	vkGetDeviceQueue(device, familyIndices.graphicsFamily, 0, &graphicsQueue);
	vkGetDeviceQueue(device, familyIndices.presentFamily, 0, &presentQueue);
}

VkSurfaceFormatKHR chooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& availableFormats)
{
	for (const VkSurfaceFormatKHR& availableFormat : availableFormats)
	{
		if (availableFormat.format == VK_FORMAT_B8G8R8A8_UNORM && availableFormat.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR)
		{
			return availableFormat;
		}
	}

	return availableFormats[0];
}

VkPresentModeKHR chooseSwapPresentMode(const std::vector<VkPresentModeKHR>& availablePresentModes)
{
	VkPresentModeKHR bestMode = VK_PRESENT_MODE_FIFO_KHR;

	for (const auto& availablePresentMode : availablePresentModes)
	{
		if (availablePresentMode == VK_PRESENT_MODE_MAILBOX_KHR)
		{
			return availablePresentMode;
		}
		else if (availablePresentMode == VK_PRESENT_MODE_IMMEDIATE_KHR)
		{
			bestMode = availablePresentMode;
		}
	}

	return bestMode;
}

VkExtent2D VulkanRenderer::chooseSwapExtent(const VkSurfaceCapabilitiesKHR& capabilities, int& _WIDTH, int& _HEIGHT)
{
	if (capabilities.currentExtent.width != std::numeric_limits<uint32_t>::max())
	{
		return capabilities.currentExtent;
	}
	else
	{
		pWindow->GetVulkanDrawableSize(&_WIDTH, &_HEIGHT);

		VkExtent2D actualExtent = {
			static_cast<uint32_t>(_WIDTH),
			static_cast<uint32_t>(_HEIGHT)
		};

		actualExtent.width  = std::max(capabilities.minImageExtent.width,  std::min(capabilities.maxImageExtent.width, actualExtent.width));
		actualExtent.height = std::max(capabilities.minImageExtent.height, std::min(capabilities.maxImageExtent.height, actualExtent.height));

		return actualExtent;
	}
}

void VulkanRenderer::createSwapChain()
{
	int WIDTH, HEIGHT;
	SwapChainSupportDetails swapChainSupport = querySwapChainSupport(physicalDevice, surface);

	VkSurfaceFormatKHR surfaceFormat = chooseSwapSurfaceFormat(swapChainSupport.formats);
	VkPresentModeKHR presentMode	 = chooseSwapPresentMode(swapChainSupport.presentModes);
	VkExtent2D extent				 = chooseSwapExtent(swapChainSupport.capabilities, WIDTH, HEIGHT);

	swapChainImageFormat = surfaceFormat.format;
	swapChainExtent = extent;

	uint32_t imageCount = swapChainSupport.capabilities.minImageCount + 1;
	if (swapChainSupport.capabilities.maxImageCount > 0 && imageCount > swapChainSupport.capabilities.maxImageCount)
	{
		imageCount = swapChainSupport.capabilities.maxImageCount;
	}

	VkSwapchainCreateInfoKHR createInfo = {};
	createInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
	createInfo.surface = surface;
	createInfo.minImageCount = imageCount;
	createInfo.imageFormat = surfaceFormat.format;
	createInfo.imageColorSpace = surfaceFormat.colorSpace;
	createInfo.imageExtent = extent;
	createInfo.imageArrayLayers = 1;
	createInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT;

	uint32_t queueFamilyIndices[] = { familyIndices.graphicsFamily, familyIndices.presentFamily };
	if (familyIndices.graphicsFamily != familyIndices.presentFamily)
	{
		createInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
		createInfo.queueFamilyIndexCount = static_cast<uint32_t>(sizeof(queueFamilyIndices) / sizeof(uint32_t));
		createInfo.pQueueFamilyIndices = queueFamilyIndices;
	}
	else
	{
		createInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
		createInfo.queueFamilyIndexCount = 0; // Optional
		createInfo.pQueueFamilyIndices = nullptr; // Optional
	}

	createInfo.preTransform = swapChainSupport.capabilities.currentTransform;
	createInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
	createInfo.presentMode = presentMode;
	createInfo.clipped = VK_TRUE;
	createInfo.oldSwapchain = VK_NULL_HANDLE;

	if (vkCreateSwapchainKHR(device, &createInfo, nullptr, &swapChain) != VK_SUCCESS)
	{
		throw std::runtime_error("failed to create swap chain!");
	}

	vkGetSwapchainImagesKHR(device, swapChain, &imageCount, nullptr);
	swapChainImages.resize(imageCount);
	vkGetSwapchainImagesKHR(device, swapChain, &imageCount, swapChainImages.data());
}

void VulkanRenderer::createImageViews()
{
	swapChainImageViews.resize(swapChainImages.size());
	for (size_t i = 0; i < swapChainImages.size(); i++)
	{
		swapChainImageViews[i] = createImageView(swapChainImages[i], swapChainImageFormat, VK_IMAGE_ASPECT_COLOR_BIT);
	}
}


static std::vector<char> readFile(const char* filename)
{
	std::ifstream file(filename, std::ios::ate | std::ios::binary);

	if (!file.is_open())
	{
		throw std::runtime_error("failed to open file!");
	}

	size_t fileSize = (size_t)file.tellg();
	std::vector<char> buffer(fileSize);
	file.seekg(0);
	file.read(buffer.data(), fileSize);
	file.close();

	return buffer;
}

void VulkanRenderer::PH_CreateShaderModule(const char* path, VkShaderModule* shaderModule)
{
	std::vector<char> code = readFile(path);

	VkShaderModuleCreateInfo createInfo = {};
	createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
	createInfo.codeSize = code.size();
	createInfo.pCode = reinterpret_cast<const uint32_t*>(code.data());
	
	if (vkCreateShaderModule(device, &createInfo, nullptr, shaderModule) != VK_SUCCESS)
	{
		throw std::runtime_error("failed to create shader module!");
	}
}

void VulkanRenderer::PH_DestroyShaderModule(VkShaderModule* shaderModule)
{
	vkDestroyShaderModule(device, *shaderModule, nullptr);
}

void VulkanRenderer::PH_LoadModel(const std::string& filename, VertexLayout layout, PH_Model *ph_model)
{
	Assimp::Importer Importer;
	const aiScene* pScene;

	pScene = Importer.ReadFile(filename.c_str(), PH_Model::defaultFlags);
	if (!pScene)
	{
		std::string error = Importer.GetErrorString();
		std::cerr << error;
	}

	if (pScene)
	{
		ph_model->parts.clear();
		ph_model->parts.resize(pScene->mNumMeshes);

		glm::vec3 scale(0.25f);
		glm::vec2 uvscale(0.25f);
		glm::vec3 center(0.0f);
		std::vector<float> vertexBuffer;
		std::vector<uint32_t> indexBuffer;

		ph_model->vertexCount = 0;
		ph_model->indexCount = 0;

		// Load meshes
		for (unsigned int i = 0; i < pScene->mNumMeshes; i++)
		{
			const aiMesh* paiMesh = pScene->mMeshes[i];

			ph_model->parts[i] = {};
			ph_model->parts[i].vertexBase = ph_model->vertexCount;
			ph_model->parts[i].indexBase  = ph_model->indexCount;

			ph_model->vertexCount += pScene->mMeshes[i]->mNumVertices;

			aiColor3D pColor(0.f, 0.f, 0.f);
			pScene->mMaterials[paiMesh->mMaterialIndex]->Get(AI_MATKEY_COLOR_DIFFUSE, pColor);

			const aiVector3D Zero3D(0.0f, 0.0f, 0.0f);

			for (unsigned int j = 0; j < paiMesh->mNumVertices; j++)
			{
				const aiVector3D* pPos = &(paiMesh->mVertices[j]);
				const aiVector3D* pNormal = &(paiMesh->mNormals[j]);
				const aiVector3D* pTexCoord = (paiMesh->HasTextureCoords(0)) ? &(paiMesh->mTextureCoords[0][j]) : &Zero3D;
				const aiVector3D* pTangent = (paiMesh->HasTangentsAndBitangents()) ? &(paiMesh->mTangents[j]) : &Zero3D;
				const aiVector3D* pBiTangent = (paiMesh->HasTangentsAndBitangents()) ? &(paiMesh->mBitangents[j]) : &Zero3D;

				for (auto& component : layout.components)
				{
					switch (component) {
					case VERTEX_COMPONENT_POSITION:
						vertexBuffer.push_back(pPos->x * scale.x + center.x);
						vertexBuffer.push_back(-pPos->y * scale.y + center.y);
						vertexBuffer.push_back(pPos->z * scale.z + center.z);
						break;
					case VERTEX_COMPONENT_NORMAL:
						vertexBuffer.push_back(pNormal->x);
						vertexBuffer.push_back(-pNormal->y);
						vertexBuffer.push_back(pNormal->z);
						break;
					case VERTEX_COMPONENT_UV:
						vertexBuffer.push_back(pTexCoord->x * uvscale.s);
						vertexBuffer.push_back(pTexCoord->y * uvscale.t);
						break;
					case VERTEX_COMPONENT_COLOR:
						vertexBuffer.push_back(pColor.r);
						vertexBuffer.push_back(pColor.g);
						vertexBuffer.push_back(pColor.b);
						break;
					case VERTEX_COMPONENT_TANGENT:
						vertexBuffer.push_back(pTangent->x);
						vertexBuffer.push_back(pTangent->y);
						vertexBuffer.push_back(pTangent->z);
						break;
					case VERTEX_COMPONENT_BITANGENT:
						vertexBuffer.push_back(pBiTangent->x);
						vertexBuffer.push_back(pBiTangent->y);
						vertexBuffer.push_back(pBiTangent->z);
						break;
						// Dummy components for padding
					case VERTEX_COMPONENT_DUMMY_FLOAT:
						vertexBuffer.push_back(0.0f);
						break;
					case VERTEX_COMPONENT_DUMMY_VEC4:
						vertexBuffer.push_back(0.0f);
						vertexBuffer.push_back(0.0f);
						vertexBuffer.push_back(0.0f);
						vertexBuffer.push_back(0.0f);
						break;
					};
				}

				ph_model->dim.max.x = fmax(pPos->x, ph_model->dim.max.x);
				ph_model->dim.max.y = fmax(pPos->y, ph_model->dim.max.y);
				ph_model->dim.max.z = fmax(pPos->z, ph_model->dim.max.z);

				ph_model->dim.min.x = fmin(pPos->x, ph_model->dim.min.x);
				ph_model->dim.min.y = fmin(pPos->y, ph_model->dim.min.y);
				ph_model->dim.min.z = fmin(pPos->z, ph_model->dim.min.z);
			}

			ph_model->dim.size = ph_model->dim.max - ph_model->dim.min;

			ph_model->parts[i].vertexCount = paiMesh->mNumVertices;

			uint32_t indexBase = static_cast<uint32_t>(indexBuffer.size());
			for (unsigned int j = 0; j < paiMesh->mNumFaces; j++)
			{
				const aiFace& Face = paiMesh->mFaces[j];
				if (Face.mNumIndices != 3)
					continue;
				indexBuffer.push_back(indexBase + Face.mIndices[0]);
				indexBuffer.push_back(indexBase + Face.mIndices[1]);
				indexBuffer.push_back(indexBase + Face.mIndices[2]);
				ph_model->parts[i].indexCount += 3;
				ph_model->indexCount += 3;
			}
		}


		uint32_t vBufferSize = static_cast<uint32_t>(vertexBuffer.size()) * sizeof(float);
		uint32_t iBufferSize = static_cast<uint32_t>(indexBuffer.size()) * sizeof(uint32_t);

		// VERTEX BUFFER
		{
			PH_BufferCreateInfo vBufferInfo;
			vBufferInfo.bufferSize = vBufferSize;
			vBufferInfo.bufferUsageFlags = VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT;
			vBufferInfo.memoryPropertyFlags = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
			vBufferInfo.data = vertexBuffer.data();
			PH_CreateBuffer(vBufferInfo, &(ph_model->vertices));
		}

		// INDEX BUFFER
		{
			PH_BufferCreateInfo iBufferInfo;
			iBufferInfo.bufferSize = iBufferSize;
			iBufferInfo.bufferUsageFlags = VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT;
			iBufferInfo.memoryPropertyFlags = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
			iBufferInfo.data = indexBuffer.data();
			PH_CreateBuffer(iBufferInfo, &(ph_model->indices));
		}
	}
	else
	{
		assert(0);
	}
};

void VulkanRenderer::PH_DeleteModel(PH_Model* ph_model)
{
	PH_DeleteBuffer(&ph_model->vertices);

	if (ph_model->indices.buffer != VK_NULL_HANDLE)
	{
		PH_DeleteBuffer(&ph_model->indices);
	}
}

VkFormat findSupportedFormat(VkPhysicalDevice& physicalDevice, const std::vector<VkFormat>& candidates, VkImageTiling tiling, VkFormatFeatureFlags features)
{
	for (VkFormat format : candidates)
	{
		VkFormatProperties props;
		vkGetPhysicalDeviceFormatProperties(physicalDevice, format, &props);

		if (tiling == VK_IMAGE_TILING_LINEAR && (props.linearTilingFeatures & features) == features)
		{
			return format;
		}
		else if (tiling == VK_IMAGE_TILING_OPTIMAL && (props.optimalTilingFeatures & features) == features)
		{
			return format;
		}
	}
	throw std::runtime_error("failed to find supported format!");
}

VkFormat findDepthFormat(VkPhysicalDevice& physicalDevice)
{
	std::vector<VkFormat> candidates;
	candidates.emplace_back(VK_FORMAT_D32_SFLOAT);
	candidates.emplace_back(VK_FORMAT_D32_SFLOAT_S8_UINT);
	candidates.emplace_back(VK_FORMAT_D24_UNORM_S8_UINT);

	return findSupportedFormat(physicalDevice,
		candidates,
		VK_IMAGE_TILING_OPTIMAL,
		VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT
	);
}

void VulkanRenderer::createRenderPass()
{
	VkSubpassDescription subpass = {};
	subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
	subpass.colorAttachmentCount = 1;
	
	std::vector<VkAttachmentDescription> attachments;
	{
		VkAttachmentReference colorAttachmentRef = {};
		colorAttachmentRef.attachment = 0;
		colorAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

		subpass.pColorAttachments = &colorAttachmentRef;

		VkAttachmentDescription colorAttachment = {};
		colorAttachment.format = swapChainImageFormat;
		colorAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
		colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
		colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
		colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
		colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
		colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		colorAttachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

		attachments.emplace_back(colorAttachment);
	}

	{
		VkAttachmentDescription depthAttachment = {};
		depthAttachment.format = findDepthFormat(physicalDevice);
		depthAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
		depthAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
		depthAttachment.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
		depthAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
		depthAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
		depthAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		depthAttachment.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

		VkAttachmentReference depthAttachmentRef = {};
		depthAttachmentRef.attachment = 1;
		depthAttachmentRef.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
	
		subpass.pDepthStencilAttachment = &depthAttachmentRef;

		attachments.emplace_back(depthAttachment);
	}
	
	VkRenderPassCreateInfo renderPassInfo = {};
	renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
	renderPassInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
	renderPassInfo.pAttachments = attachments.data();
	renderPassInfo.subpassCount = 1;
	renderPassInfo.pSubpasses = &subpass;

	VkSubpassDependency dependency[2] = {};
	dependency[0].srcSubpass = VK_SUBPASS_EXTERNAL;
	dependency[0].dstSubpass = 0;
	dependency[0].srcStageMask = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
	dependency[0].dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	dependency[0].srcAccessMask = VK_ACCESS_MEMORY_READ_BIT;
	dependency[0].dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
	dependency[0].dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;

	dependency[1].srcSubpass = 0;
	dependency[1].dstSubpass = VK_SUBPASS_EXTERNAL;
	dependency[1].srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	dependency[1].dstStageMask = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
	dependency[1].srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
	dependency[1].dstAccessMask = VK_ACCESS_MEMORY_READ_BIT;
	dependency[1].dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;

	renderPassInfo.dependencyCount = (uint32_t)(sizeof(dependency) / sizeof(dependency[0]));
	renderPassInfo.pDependencies = dependency;

	if (vkCreateRenderPass(device, &renderPassInfo, nullptr, &renderPass) != VK_SUCCESS)
	{
		throw std::runtime_error("failed to create render pass!");
	}
}

void VulkanRenderer::PH_CreateDescriptorSetLayout(VkDescriptorSetLayoutCreateInfo layoutInfo, VkDescriptorSetLayout* descriptorSetLayout)
{
	if (vkCreateDescriptorSetLayout(device, &layoutInfo, nullptr, descriptorSetLayout) != VK_SUCCESS)
	{
		throw std::runtime_error("failed to create descriptor set layout!");
	}
}

void VulkanRenderer::PH_DeleteDescriptorSetLayout(VkDescriptorSetLayout* descriptorSetLayout)
{
	vkDestroyDescriptorSetLayout(device, *descriptorSetLayout, nullptr);
}

void VulkanRenderer::PH_CreatePipelineLayout(VkPipelineLayoutCreateInfo pipelineLayoutInfo, VkPipelineLayout* pipelineLayout)
{
	if (vkCreatePipelineLayout(device, &pipelineLayoutInfo, nullptr, pipelineLayout) != VK_SUCCESS)
	{
		throw std::runtime_error("failed to create pipeline layout!");
	}
}

void VulkanRenderer::PH_DeletePipelineLayout(VkPipelineLayout* pipelineLayout)
{
	vkDestroyPipelineLayout(device, *pipelineLayout, nullptr);
}


void VulkanRenderer::PH_CreateGraphicsPipeline(VkGraphicsPipelineCreateInfo pipelineInfo, VkPipeline* graphicsPipeline)
{
	if (vkCreateGraphicsPipelines(device, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, graphicsPipeline) != VK_SUCCESS)
	{
		throw std::runtime_error("failed to create graphics pipeline!");
	}
}

void VulkanRenderer::PH_DeleteGraphicsPipeline(VkPipeline* graphicsPipeline)
{
	vkDestroyPipeline(device, *graphicsPipeline, nullptr);
}

void VulkanRenderer::createFramebuffers()
{
	swapChainFramebuffers.resize(swapChainImageViews.size());
	for (size_t i = 0; i < swapChainImageViews.size(); i++)
	{
		std::vector<VkImageView> attachments;
		attachments.push_back(swapChainImageViews[i]);
		attachments.push_back(depthImageView);
		
		VkFramebufferCreateInfo framebufferInfo = {};
		framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
		framebufferInfo.renderPass = renderPass;
		framebufferInfo.attachmentCount = (uint32_t)attachments.size();
		framebufferInfo.pAttachments = attachments.data();
		framebufferInfo.width = swapChainExtent.width;
		framebufferInfo.height = swapChainExtent.height;
		framebufferInfo.layers = 1;

		if (vkCreateFramebuffer(device, &framebufferInfo, nullptr, &swapChainFramebuffers[i]) != VK_SUCCESS)
		{
			throw std::runtime_error("failed to create framebuffer!");
		}
	}
}

void VulkanRenderer::createCommandPool()
{
	VkCommandPoolCreateInfo poolInfo = {};
	poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
	poolInfo.queueFamilyIndex = familyIndices.graphicsFamily;
	poolInfo.flags = 0; // Optional
	if (vkCreateCommandPool(device, &poolInfo, nullptr, &commandPool) != VK_SUCCESS)
	{
		throw std::runtime_error("failed to create command pool!");
	}
}

void VulkanRenderer::createCommandBuffers()
{
	commandBuffers.resize(swapChainFramebuffers.size());

	VkCommandBufferAllocateInfo allocInfo = {};
	allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	allocInfo.commandPool = commandPool;
	allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
	allocInfo.commandBufferCount = (uint32_t)commandBuffers.size();

	if (vkAllocateCommandBuffers(device, &allocInfo, commandBuffers.data()) != VK_SUCCESS)
	{
		throw std::runtime_error("failed to allocate command buffers!");
	}
}

void VulkanRenderer::createSyncObjects()
{
	imageAvailableSemaphores.resize(MAX_FRAMES_IN_FLIGHT);
	renderFinishedSemaphores.resize(MAX_FRAMES_IN_FLIGHT);
	inFlightFences.resize(MAX_FRAMES_IN_FLIGHT);

	VkSemaphoreCreateInfo semaphoreInfo = {};
	semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

	VkFenceCreateInfo fenceInfo = {};
	fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
	fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

	for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
	{
		if (vkCreateSemaphore(device, &semaphoreInfo, nullptr, &imageAvailableSemaphores[i]) != VK_SUCCESS ||
			vkCreateSemaphore(device, &semaphoreInfo, nullptr, &renderFinishedSemaphores[i]) != VK_SUCCESS ||
			vkCreateFence(device, &fenceInfo, nullptr, &inFlightFences[i]) != VK_SUCCESS)
		{
			throw std::runtime_error("failed to create semaphores!");
		}
	}
}

const uint32_t VulkanRenderer::PH_PrepareNextFrame()
{
	vkWaitForFences(device, 1, &inFlightFences[currentFrame], VK_TRUE, std::numeric_limits<uint64_t>::max());

	VkResult result = vkAcquireNextImageKHR(device, swapChain, std::numeric_limits<uint64_t>::max(), imageAvailableSemaphores[currentFrame], VK_NULL_HANDLE, &imageIndex);

	if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR)
	{
		recreateSwapChain();
		return -1;
	}
	else if (result != VK_SUCCESS)
	{
		throw std::runtime_error("failed to acquire swap chain image!");
	}

	return imageIndex;
}

void VulkanRenderer::PH_SubmitFrame()
{
	VkSubmitInfo submitInfo = {};
	submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

	VkSemaphore waitSemaphores[] = { imageAvailableSemaphores[currentFrame] };
	VkPipelineStageFlags waitStages[] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };
	submitInfo.waitSemaphoreCount = 1;
	submitInfo.pWaitSemaphores = waitSemaphores;
	submitInfo.pWaitDstStageMask = waitStages;
	submitInfo.commandBufferCount = 1;
	submitInfo.pCommandBuffers = &commandBuffers[imageIndex];

	VkSemaphore signalSemaphores[] = { renderFinishedSemaphores[currentFrame] };
	submitInfo.signalSemaphoreCount = 1;
	submitInfo.pSignalSemaphores = signalSemaphores;

	vkResetFences(device, 1, &inFlightFences[currentFrame]);

	if (vkQueueSubmit(graphicsQueue, 1, &submitInfo, inFlightFences[currentFrame]) != VK_SUCCESS)
	{
		throw std::runtime_error("failed to submit draw command buffer!");
	}

	VkPresentInfoKHR presentInfo = {};
	presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;

	presentInfo.waitSemaphoreCount = 1;
	presentInfo.pWaitSemaphores = signalSemaphores;

	VkSwapchainKHR swapChains[] = { swapChain };
	presentInfo.swapchainCount = 1;
	presentInfo.pSwapchains = swapChains;
	presentInfo.pImageIndices = &imageIndex;
	presentInfo.pResults = nullptr; // Optional

	VkResult result = vkQueuePresentKHR(presentQueue, &presentInfo);
	if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR || pWindow->isWindowResized() || pWindow->isWindowMinimized())
	{
		recreateSwapChain();
	}
	else if (result != VK_SUCCESS)
	{
		throw std::runtime_error("failed to present swap chain image!");
	}

	currentFrame = (currentFrame + 1) % MAX_FRAMES_IN_FLIGHT;
}

void VulkanRenderer::waitDeviceIdle()
{
	vkDeviceWaitIdle(device);
}

uint32_t VulkanRenderer::findMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties)
{
	VkPhysicalDeviceMemoryProperties memProperties;
	vkGetPhysicalDeviceMemoryProperties(physicalDevice, &memProperties);

	for (uint32_t i = 0; i < memProperties.memoryTypeCount; i++)
	{
		if ((typeFilter & (1 << i)) && (memProperties.memoryTypes[i].propertyFlags & properties) == properties)
		{
			return i;
		}
	}

	throw std::runtime_error("failed to find suitable memory type!");
}

VkCommandBuffer VulkanRenderer::beginSingleTimeCommands()
{
	VkCommandBufferAllocateInfo allocInfo = {};
	allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
	allocInfo.commandPool = commandPool;
	allocInfo.commandBufferCount = 1;

	VkCommandBuffer commandBuffer;
	vkAllocateCommandBuffers(device, &allocInfo, &commandBuffer);

	VkCommandBufferBeginInfo beginInfo = {};
	beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

	vkBeginCommandBuffer(commandBuffer, &beginInfo);

	return commandBuffer;
}

void VulkanRenderer::endSingleTimeCommands(VkCommandBuffer commandBuffer)
{
	vkEndCommandBuffer(commandBuffer);

	VkSubmitInfo submitInfo = {};
	submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
	submitInfo.commandBufferCount = 1;
	submitInfo.pCommandBuffers = &commandBuffer;

	vkQueueSubmit(graphicsQueue, 1, &submitInfo, VK_NULL_HANDLE);
	vkQueueWaitIdle(graphicsQueue);

	vkFreeCommandBuffers(device, commandPool, 1, &commandBuffer);
}

void VulkanRenderer::createBuffer(VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties, VkBuffer& buffer, VkDeviceMemory& bufferMemory)
{
	VkBufferCreateInfo bufferInfo = {};
	bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
	bufferInfo.size = size;
	bufferInfo.usage = usage;
	bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

	if (vkCreateBuffer(device, &bufferInfo, nullptr, &buffer) != VK_SUCCESS) {
		throw std::runtime_error("failed to create buffer!");
	}

	VkMemoryRequirements memRequirements;
	vkGetBufferMemoryRequirements(device, buffer, &memRequirements);

	VkMemoryAllocateInfo allocInfo = {};
	allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
	allocInfo.allocationSize = memRequirements.size;
	allocInfo.memoryTypeIndex = findMemoryType(memRequirements.memoryTypeBits, properties);

	if (vkAllocateMemory(device, &allocInfo, nullptr, &bufferMemory) != VK_SUCCESS) {
		throw std::runtime_error("failed to allocate buffer memory!");
	}

	vkBindBufferMemory(device, buffer, bufferMemory, 0);
}

void VulkanRenderer::copyBuffer(VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size)
{
	VkCommandBuffer commandBuffer = beginSingleTimeCommands();

	VkBufferCopy copyRegion = {};
	copyRegion.srcOffset = 0; // Optional
	copyRegion.dstOffset = 0; // Optional
	copyRegion.size = size;
	vkCmdCopyBuffer(commandBuffer, srcBuffer, dstBuffer, 1, &copyRegion);

	endSingleTimeCommands(commandBuffer);
}

void VulkanRenderer::copyBufferToImage(VkBuffer buffer, VkImage image, uint32_t width, uint32_t height)
{
	VkCommandBuffer commandBuffer = beginSingleTimeCommands();

	VkBufferImageCopy region = {};
	region.bufferOffset = 0;
	region.bufferRowLength = 0;
	region.bufferImageHeight = 0;

	region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	region.imageSubresource.mipLevel = 0;
	region.imageSubresource.baseArrayLayer = 0;
	region.imageSubresource.layerCount = 1;

	region.imageOffset = { 0, 0, 0 };
	region.imageExtent = {
		width,
		height,
		1
	};

	vkCmdCopyBufferToImage(
		commandBuffer,
		buffer,
		image,
		VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
		1,
		&region
	);

	endSingleTimeCommands(commandBuffer);
}

void VulkanRenderer::PH_CreateBuffer(PH_BufferCreateInfo info, PH_Buffer* ph_buffer)
{
	ph_buffer->bufferSize = info.bufferSize;

	if ((info.memoryPropertyFlags & VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT) && info.data != nullptr)
	{
		VkBuffer stagingBuffer;
		VkDeviceMemory stagingBufferMemory;
		createBuffer(ph_buffer->bufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, stagingBuffer, stagingBufferMemory);
		
		void* data;
		vkMapMemory(device, stagingBufferMemory, 0, ph_buffer->bufferSize, 0, &data);
		memcpy(data, info.data, (size_t)ph_buffer->bufferSize);
		vkUnmapMemory(device, stagingBufferMemory);

		createBuffer(ph_buffer->bufferSize, info.bufferUsageFlags, info.memoryPropertyFlags, ph_buffer->buffer, ph_buffer->bufferMemory);

		copyBuffer(stagingBuffer, ph_buffer->buffer, ph_buffer->bufferSize);

		vkDestroyBuffer(device, stagingBuffer, nullptr);
		vkFreeMemory(device, stagingBufferMemory, nullptr);
	}
	else// if (info.memoryPropertyFlags | VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT)
	{
		createBuffer(ph_buffer->bufferSize, info.bufferUsageFlags, info.memoryPropertyFlags, ph_buffer->buffer, ph_buffer->bufferMemory);

		if (info.data != nullptr)
		{
			void* data;
			vkMapMemory(device, ph_buffer->bufferMemory, 0, ph_buffer->bufferSize, 0, &data);
			memcpy(data, info.data, (size_t)ph_buffer->bufferSize);
			vkUnmapMemory(device, ph_buffer->bufferMemory);
		}
	}
}

void VulkanRenderer::PH_DeleteBuffer(PH_Buffer* ph_buffer)
{
	vkDestroyBuffer(device, ph_buffer->buffer, nullptr);
	vkFreeMemory(device, ph_buffer->bufferMemory, nullptr);
}

void VulkanRenderer::PH_UpdateBuffer(PH_BufferUpdateInfo info)
{
	void* data;
	vkMapMemory(device, info.buffer.bufferMemory, 0, info.dataSize, 0, &data);
		memcpy(data, info.data, info.dataSize);
	vkUnmapMemory(device, info.buffer.bufferMemory);
}

void VulkanRenderer::createImage(uint32_t width, uint32_t height, VkFormat format, VkImageTiling tiling, VkImageUsageFlags usage,
					VkMemoryPropertyFlags properties, VkImage& image, VkDeviceMemory& imageMemory)
{
	VkImageCreateInfo imageInfo = {};
	imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
	imageInfo.imageType = VK_IMAGE_TYPE_2D;
	imageInfo.extent.width = width;
	imageInfo.extent.height = height;
	imageInfo.extent.depth = 1;
	imageInfo.mipLevels = 1;
	imageInfo.arrayLayers = 1;
	imageInfo.format = format;
	imageInfo.tiling = tiling;
	imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	imageInfo.usage = usage;
	imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
	imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
	imageInfo.flags = 0; // Optional

	if (vkCreateImage(device, &imageInfo, nullptr, &image) != VK_SUCCESS)
	{
		throw std::runtime_error("failed to create image!");
	}

	VkMemoryRequirements memRequirements;
	vkGetImageMemoryRequirements(device, image, &memRequirements);

	VkMemoryAllocateInfo allocInfo = {};
	allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
	allocInfo.allocationSize = memRequirements.size;
	allocInfo.memoryTypeIndex = findMemoryType(memRequirements.memoryTypeBits, properties);

	if (vkAllocateMemory(device, &allocInfo, nullptr, &imageMemory) != VK_SUCCESS) {
		throw std::runtime_error("failed to allocate image memory!");
	}

	vkBindImageMemory(device, image, imageMemory, 0);
}

VkImageView VulkanRenderer::createImageView(VkImage image, VkFormat format, VkImageAspectFlags aspectFlags)
{
	VkImageViewCreateInfo viewInfo = {};
	viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
	viewInfo.image = image;
	viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
	viewInfo.format = format;
	viewInfo.subresourceRange.aspectMask = aspectFlags;
	viewInfo.subresourceRange.baseMipLevel = 0;
	viewInfo.subresourceRange.levelCount = 1;
	viewInfo.subresourceRange.baseArrayLayer = 0;
	viewInfo.subresourceRange.layerCount = 1;

	VkImageView imageView;
	if (vkCreateImageView(device, &viewInfo, nullptr, &imageView) != VK_SUCCESS)
	{
		throw std::runtime_error("failed to create texture image view!");
	}

	return imageView;
}

bool hasStencilComponent(VkFormat format)
{
	return format == VK_FORMAT_D32_SFLOAT_S8_UINT || format == VK_FORMAT_D24_UNORM_S8_UINT;
}

void VulkanRenderer::transitionImageLayout(VkCommandBuffer cmd, VkImage image, VkFormat format, VkImageLayout oldLayout, VkImageLayout newLayout)
{
	VkImageMemoryBarrier barrier = {};
	barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
	barrier.oldLayout = oldLayout;
	barrier.newLayout = newLayout;
	barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	barrier.image = image;

	if (newLayout == VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL)
	{
		barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;

		if (hasStencilComponent(format))
		{
			barrier.subresourceRange.aspectMask |= VK_IMAGE_ASPECT_STENCIL_BIT;
		}
	}
	else
	{
		barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	}

	barrier.subresourceRange.baseMipLevel = 0;
	barrier.subresourceRange.levelCount = 1;
	barrier.subresourceRange.baseArrayLayer = 0;
	barrier.subresourceRange.layerCount = 1;
	
	VkPipelineStageFlags sourceStage	  = VK_PIPELINE_STAGE_ALL_COMMANDS_BIT;
	VkPipelineStageFlags destinationStage = VK_PIPELINE_STAGE_ALL_COMMANDS_BIT;

	switch (oldLayout)
	{
	case VK_IMAGE_LAYOUT_UNDEFINED:
		barrier.srcAccessMask = 0;
		sourceStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
		break;

	case VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL:
		barrier.srcAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
		sourceStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
		break;

	case VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL:
		barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
		sourceStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
		break;

	case VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL:
		barrier.srcAccessMask = VK_ACCESS_SHADER_READ_BIT;
		sourceStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
		break;

	case VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL:
		barrier.srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
		break;

	case VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL:
		barrier.srcAccessMask |= VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
		break;

	case VK_IMAGE_LAYOUT_PREINITIALIZED:
		break;

	default:
		// layout not handled yet
		break;
	}

	switch (newLayout)
	{
	case VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL:
		barrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
		//destinationStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
		break;

	case VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL:
		barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
		destinationStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
		break;

	case VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL:
		barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
		destinationStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
		break;

	case VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL:
		barrier.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
		break;

	case VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL:
		barrier.dstAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
		destinationStage = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
		break;

	default:
		// layout not handled yet
		break;
	}

	vkCmdPipelineBarrier(
		cmd,
		sourceStage, destinationStage,
		0,
		0, nullptr,
		0, nullptr,
		1, &barrier
	);
}


void VulkanRenderer::createDepthResources()
{
	VkFormat depthFormat = findDepthFormat(physicalDevice);

	createImage(swapChainExtent.width, swapChainExtent.height, depthFormat,
		VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
		depthImage, depthImageMemory);
	depthImageView = createImageView(depthImage, depthFormat, VK_IMAGE_ASPECT_DEPTH_BIT);
		
	VkCommandBuffer commandBuffer = beginSingleTimeCommands();
		transitionImageLayout(commandBuffer, depthImage, depthFormat, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL);
	endSingleTimeCommands(commandBuffer);
}

void VulkanRenderer::PH_CreateTexture(PH_ImageCreateInfo info, PH_Image* ph_image)
{
	ph_image->format = info.format;
	ph_image->width = info.width;
	ph_image->height = info.height;
	ph_image->path = info.path;

	if (ph_image->path.empty())
	{
		createImage(ph_image->width, ph_image->height, ph_image->format, info.tiling, info.usageFlags, info.memoryProperty, ph_image->image, ph_image->imageMemory);
		ph_image->imageView = createImageView(ph_image->image, ph_image->format, info.aspectBits);
	}
	else
	{
		stbi_uc* pixels = stbi_load(ph_image->path.c_str(), &ph_image->width, &ph_image->height, &ph_image->nChannels, STBI_rgb_alpha);
		
		VkDeviceSize imageSize = (uint64_t)(ph_image->width * ph_image->height * 4);

		if (!pixels)
		{
			throw std::runtime_error("failed to load texture image!");
		}

		VkBuffer stagingBuffer;
		VkDeviceMemory stagingBufferMemory;

		createBuffer(imageSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, stagingBuffer, stagingBufferMemory);

		void* data;
		vkMapMemory(device, stagingBufferMemory, 0, imageSize, 0, &data);
		memcpy(data, pixels, static_cast<size_t>(imageSize));
		vkUnmapMemory(device, stagingBufferMemory);

		stbi_image_free(pixels);

		if (ph_image->format == VK_FORMAT_UNDEFINED)
		{
			/*if(ph_image->nChannels == 4)
				ph_image->format = VK_FORMAT_R8G8B8A8_UNORM;
			else if(ph_image->nChannels == 3)
				ph_image->format = VK_FORMAT_R8G8B8_UNORM;
			else*/
				ph_image->format = VK_FORMAT_R8G8B8A8_UNORM;
		}

		createImage(ph_image->width, ph_image->height, ph_image->format, info.tiling, info.usageFlags, info.memoryProperty, ph_image->image, ph_image->imageMemory);

		{
			VkCommandBuffer cmd = beginSingleTimeCommands();
				transitionImageLayout(cmd, ph_image->image, ph_image->format, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
			endSingleTimeCommands(cmd);
		}

		copyBufferToImage(stagingBuffer, ph_image->image, static_cast<uint32_t>(ph_image->width), static_cast<uint32_t>(ph_image->height));
	
		{
			VkCommandBuffer cmd = beginSingleTimeCommands();
				transitionImageLayout(cmd, ph_image->image, ph_image->format, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
			endSingleTimeCommands(cmd);
		}
		vkDestroyBuffer(device, stagingBuffer, nullptr);
		vkFreeMemory(device, stagingBufferMemory, nullptr);

		ph_image->imageView = createImageView(ph_image->image, ph_image->format, info.aspectBits);
	}
}

void VulkanRenderer::PH_DeleteTexture(PH_Image* ph_image)
{
	vkDestroyImageView(device, ph_image->imageView, nullptr);
	vkDestroyImage(device, ph_image->image, nullptr);
	vkFreeMemory(device, ph_image->imageMemory, nullptr);
}

VkSampler VulkanRenderer::PH_CreateSampler()
{
	VkSampler textureSampler;

	VkSamplerCreateInfo samplerInfo = {};
	samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
	samplerInfo.magFilter = VK_FILTER_LINEAR;
	samplerInfo.minFilter = VK_FILTER_LINEAR;
	samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
	samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
	samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
	samplerInfo.anisotropyEnable = VK_TRUE;
	samplerInfo.maxAnisotropy = 16;
	samplerInfo.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
	samplerInfo.unnormalizedCoordinates = VK_FALSE;
	samplerInfo.compareEnable = VK_FALSE;
	samplerInfo.compareOp = VK_COMPARE_OP_ALWAYS;
	samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
	samplerInfo.mipLodBias = 0.0f;
	samplerInfo.minLod = 0.0f;
	samplerInfo.maxLod = 0.0f;

	if (vkCreateSampler(device, &samplerInfo, nullptr, &textureSampler) != VK_SUCCESS)
	{
		throw std::runtime_error("failed to create texture sampler!");
	}

	return textureSampler;
}

void VulkanRenderer::PH_DeleteSampler(VkSampler* sampler)
{
	vkDestroySampler(device, *sampler, nullptr);
}

void VulkanRenderer::PH_CreateDescriptorPool(uint32_t noOfPools, VkDescriptorPoolSize* poolSizes, uint32_t maxSets, VkDescriptorPool* descriptorPool)
{
	VkDescriptorPoolCreateInfo descriptorPoolInfo{};
	descriptorPoolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
	descriptorPoolInfo.poolSizeCount = noOfPools;
	descriptorPoolInfo.pPoolSizes = poolSizes;
	descriptorPoolInfo.maxSets = maxSets;

	if (vkCreateDescriptorPool(device, &descriptorPoolInfo, nullptr, descriptorPool) != VK_SUCCESS)
	{
		throw std::runtime_error("failed to create descriptor pool!");
	}
}

void VulkanRenderer::PH_DeleteDescriptorPool(VkDescriptorPool* descriptorPool)
{
	vkDestroyDescriptorPool(device, *descriptorPool, nullptr);
}

void VulkanRenderer::PH_CreateDescriptorSets(VkDescriptorSetLayout descriptorSetLayout, uint32_t descriptorCount, VkDescriptorPool descriptorPool, VkDescriptorSet* descriptorSets)
{
	std::vector<VkDescriptorSetLayout> layouts(descriptorCount, descriptorSetLayout);

	VkDescriptorSetAllocateInfo allocInfo = {};
	allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
	allocInfo.descriptorPool = descriptorPool;
	allocInfo.descriptorSetCount = descriptorCount;
	allocInfo.pSetLayouts = layouts.data();

	if (vkAllocateDescriptorSets(device, &allocInfo, descriptorSets) != VK_SUCCESS)
	{
		throw std::runtime_error("failed to allocate descriptor sets!");
	}
}

void VulkanRenderer::PH_UpdateDeDescriptorSets(std::vector<VkWriteDescriptorSet> descriptorWrites)
{
	vkUpdateDescriptorSets(device, static_cast<uint32_t>(descriptorWrites.size()), descriptorWrites.data(), 0, nullptr);
}