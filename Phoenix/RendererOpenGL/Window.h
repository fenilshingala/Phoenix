#pragma once

#include "../../Common/Renderer/keyBindings.h"
#include <imgui/imgui.h>
#include <stdint.h>

class Window
{
public:
	Window();
	~Window();

	void initWindow();
	void exitWindow();

	void update();

	int windowWidth();
	int windowHeight();
	
	bool windowShouldClose();
	void swapWindow();

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
	void initGui();
	void exitGui();
	void beginGuiFrame();
	void endGuiFrame();

	// FRAME RATE
	void startFrame();
	void endFrame();
	
	float frameRate();
	float frameTime();
	
	float actualFrameRate();
	float actualFrameTime();
};