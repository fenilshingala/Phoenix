#include "VulkanRenderer.h"

class Application
{
public:
	void run() {
		renderer.initWindow();
		renderer.enableDepth();
		renderer.initVulkan();

		while (!renderer.windowShouldClose() && !exit)
		{
			renderer.updateInputs();

			mainLoop();

			renderer.pollEvents();
		}

		renderer.waitDeviceIdle();
		renderer.cleanupVulkan();
		renderer.destroyWindow();
	}

private:

	void mainLoop()
	{
		renderer.drawFrame();

		if (renderer.isKeyTriggered(PH_KEY_ESCAPE))
		{
			exit = true;
		}
	}

	bool exit = false;
	VulkanRenderer renderer;
};

int main()
{
	Application app;
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