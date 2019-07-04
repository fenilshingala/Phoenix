#include "VulkanRenderer.h"

class HelloTriangleApplication
{
public:
	void run() {
		renderer.initWindow(1440, 900);
		renderer.initVulkan();

		while (!renderer.windowShouldClose()) {
			mainLoop();

			renderer.pollEvents();
		}

		renderer.cleanupVulkan();
		renderer.destroyWindow();
	}

private:

	void mainLoop()
	{
		// api sepcific updates
	}

	VulkanRenderer renderer;
};

int main()
{
	HelloTriangleApplication app;
	try {
		app.run();
	}
	catch (const std::exception& e) {
		std::cerr << e.what() << std::endl;
		return EXIT_FAILURE;
	}

	return EXIT_SUCCESS;

	return 0;
}