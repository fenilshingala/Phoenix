#pragma once

#include <vulkan/vulkan.h>

#include <cstdlib> // EXIT_SUCCESS, EXIT_FAILURE
#include <iostream>
#include <stdexcept>
#include <functional>
#include "../../Common/Thirdparty/stl/TINYSTL/vector.h"
#include "../../Common/Thirdparty/stl/TINYSTL/string.h"

struct GLFWwindow;

class VulkanRenderer
{
public:
	VulkanRenderer() : window(nullptr), WIDTH(800), HEIGHT(600), validationLayers()
	{
		validationLayers.emplace_back("VK_LAYER_LUNARG_standard_validation");
	}
	~VulkanRenderer() {}

	// WINDOW
	void initWindow(int _WIDTH = 800, int _HEIGHT = 600);
	void pollEvents();
	int windowShouldClose();
	void destroyWindow();

	// VULKAN
	void initVulkan();
	void cleanupVulkan();

private:
	// WINDOW
	const char** getExtensions(uint32_t& _extensionCount);
	GLFWwindow* window;

	int WIDTH;
	int HEIGHT;

	// VULKAN
	void createInstance();
	void destroyInstance();
	bool checkValidationLayerSupport();
	void populateDebugMessengerCreateInfo(VkDebugUtilsMessengerCreateInfoEXT& debugCreateInfo);

	VkInstance instance;
	tinystl::vector<const char*> validationLayers;
	VkDebugUtilsMessengerEXT debugMessenger;
};