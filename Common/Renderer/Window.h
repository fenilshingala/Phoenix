#pragma once

#include "keyBindings.h"
#include <imgui/imgui.h>
#include <stdint.h>
#include <string>

class Window
{
public:
	Window();
	~Window();

	void initWindow();
	void exitWindow();
	bool waitForEvents();

	void update();

	inline int getWindowWidth();
	inline int getWindowHeight();
	inline void setWindowWidth(int val);
	inline void setWindowHeight(int val);

	inline bool windowShouldClose();
	inline bool isWindowResized();
	inline bool isWindowMinimized();

	inline void setWindowTitle(const char* name);

	// MOUSE
	bool isLeftClicked();
	bool isRightClicked();

	int32_t mouseX();
	int32_t mouseY();
	int32_t scrollX();
	int32_t scrollY();

	// KEYBOARD
	bool isKeyPressed(uint32_t keyScanCode);
	bool isKeyTriggered(uint32_t keyScanCode);
	bool isKeyReleased(uint32_t keyScanCode);

	// GUI
	//void initGui();
	//void exitGui();
	//void beginGuiFrame();
	//void endGuiFrame();

	// FRAME RATE
	void startFrame();
	void endFrame();

	float frameRate();
	float frameTime();

	float actualFrameRate();
	float actualFrameTime();

	// VULKAN
	void CreateVulkanSurface(void* instance, void* surface);
	void GetVulkanDrawableSize(int* width, int* height);
	void GetVulkanExtensions(uint32_t* _extensionCount, const char*** pExtensionNames);

private:
	bool isButtonPressed(unsigned int keyScanCode);
	bool isButtonTriggerred(unsigned int keyScanCode);
	bool isButtonReleased(unsigned int keyScanCode);

	void* pWindow;		// Window Handle
	int SCR_WIDTH;
	int SCR_HEIGHT;
	int suitableWidth;
	int suitableHeight;

	bool exitProgram;
	bool isResized;
	bool isMinimized;

	// MOUSE
	uint8_t pCurrentMouseStates[3] = { 0 };
	uint8_t mMouseCurrentState[3]  = { 0 };
	uint8_t mMousePreviousState[3] = { 0 };

	int32_t lastMouseX;
	int32_t	lastMouseY;
	int32_t mMouseX;
	int32_t mMouseY;

	int32_t ScrollX;
	int32_t ScrollY;

	// KEYBOARD
	uint8_t mCurrentState[PH_MAX_KEYS]  = { 0 };
	uint8_t mPreviousState[PH_MAX_KEYS] = { 0 };

	// FRAME RATE CONTROLLER
	uint32_t mTickStart;
	uint32_t mTickEnd;
	float mNeededTicksPerFrame;
	float mFrameTime;
	float mActualFrameTime;

	std::string windowTitle;
};

inline int Window::getWindowWidth()
{
	return SCR_WIDTH;
}

inline int Window::getWindowHeight()
{
	return SCR_HEIGHT;
}

inline void Window::setWindowWidth(int val)
{
	SCR_WIDTH = val;
}

inline void Window::setWindowHeight(int val)
{
	SCR_HEIGHT = val;
}

inline bool Window::windowShouldClose()
{
	return exitProgram;
}

inline bool Window::isWindowResized()
{
	return isResized;
}

inline bool Window::isWindowMinimized()
{
	return isMinimized;
}

inline void Window::setWindowTitle(const char* name)
{
	windowTitle = name;
}