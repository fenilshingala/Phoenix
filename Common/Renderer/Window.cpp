#include "Window.h"

#include <assert.h>

#include "SDL_stdinc.h"
#define SDL_MAIN_HANDLED
#include <SDL.h>
#include <SDL_keyboard.h>
#include <SDL_vulkan.h>

#include <imgui/imgui_impl_sdl.h>
#include <imgui/imgui_impl_opengl3.h>

Window::Window() :
pWindow(nullptr), SCR_WIDTH(0), SCR_HEIGHT(0), suitableWidth(0), suitableHeight(0), exitProgram(false), isResized(false), isMinimized(false),
lastMouseX(0), lastMouseY(0), mMouseX(0), mMouseY(0), ScrollX(0), ScrollY(0), mTickStart(0), mTickEnd(0), mNeededTicksPerFrame(16.67f),
mFrameTime(0.0f), mActualFrameTime(0.0f), windowTitle("My Cool Framework!")
{
}

Window::~Window()
{
}

void Window::initWindow()
{
	SDL_DisplayMode mode = { 0 };

	if (SDL_Init(SDL_INIT_VIDEO) < 0)
	{
		assert(0);
	}

	SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 3);

	SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
	SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);

	// Get current display mode of all displays.
	for (int i = 0; i < SDL_GetNumVideoDisplays(); ++i)
	{
		SDL_DisplayMode current;

		int should_be_zero = SDL_GetCurrentDisplayMode(i, &current);

		if (should_be_zero != 0)
		{// In case of error...
			SDL_Log("Could not get display mode for video display #%d: %s", i, SDL_GetError());
		}
		else
		{
			if (mode.refresh_rate < current.refresh_rate)
				mode = current;
			// On success, print the current display mode.
			SDL_Log("Display #%d: display mode is %dx%dpx @ %dhz.", i, current.w, current.h, current.refresh_rate);
		}
	}

	if (SCR_WIDTH == 0 || SCR_HEIGHT == 0)
	{
		float enforcedRatio = 16.0f / 9.0f;
		SCR_WIDTH = (int)(mode.w * 0.9);
		SCR_HEIGHT = (int)(mode.h * 0.9);
		if (SCR_WIDTH / SCR_HEIGHT < enforcedRatio)
		{
			if (SCR_HEIGHT < SCR_WIDTH)
				SCR_HEIGHT = (int)(SCR_WIDTH / enforcedRatio);
			else
				SCR_WIDTH = (int)(SCR_HEIGHT * enforcedRatio);
		}
	}
	
	suitableWidth  = SCR_WIDTH;
	suitableHeight = SCR_HEIGHT;

	uint32_t flags = SDL_WINDOW_VULKAN | SDL_WINDOW_RESIZABLE;
#ifndef _DEBUG
	flags |= SDL_WINDOW_FULLSCREEN_DESKTOP;
#endif

	pWindow = SDL_CreateWindow(windowTitle.c_str(),
		SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
		SCR_WIDTH, SCR_HEIGHT,
		flags);

	SDL_GetWindowSize((SDL_Window*)pWindow, &SCR_WIDTH, &SCR_HEIGHT);

	if (pWindow == NULL)
	{
		assert(0);
	}
}

void Window::exitWindow()
{
	SDL_DestroyWindow((SDL_Window*)pWindow);
	SDL_Quit();
}

bool Window::waitForEvents()
{
	SDL_Event e;
	while (SDL_WaitEvent(&e) != 0)
	{
		return true;
	}
	return false;
}


void Window::update()
{
	isResized = false;

	// mouse coordinates
	lastMouseX = (int32_t)mMouseX;
	lastMouseY = (int32_t)mMouseY;

	SDL_Event e;
	while (SDL_PollEvent(&e) != 0)
	{
		//	ImGui_ImplSDL2_ProcessEvent(&e);
		switch (e.type)
		{
		case SDL_QUIT:
			exitProgram = true;
			break;

		case SDL_WINDOWEVENT:
			switch (e.window.event)
			{
			case SDL_WINDOWEVENT_FOCUS_GAINED:
				//if (focusCallback)
				//	focusCallback(true);
				break;
			case SDL_WINDOWEVENT_RESIZED:
				isResized = true;
				break;
			case SDL_WINDOWEVENT_MINIMIZED:
				isMinimized = true;
				break;
			case SDL_WINDOWEVENT_MAXIMIZED:
				isMinimized = false;
			}

			break;

		case SDL_MOUSEBUTTONUP:
			pCurrentMouseStates[e.button.button - 1] = 0;
			break;

		case SDL_MOUSEBUTTONDOWN:
			pCurrentMouseStates[e.button.button - 1] = 1;
			break;

		case SDL_MOUSEMOTION:
			mMouseX = e.motion.x;
			mMouseY = e.motion.y;
			break;

		case SDL_MOUSEWHEEL:
			ScrollX = e.wheel.x;
			ScrollY = e.wheel.y;
			break;
		}
	}

	// MOUSE STATES
	SDL_memcpy(mMousePreviousState, mMouseCurrentState, 3 * sizeof(uint8_t));
	SDL_memcpy(mMouseCurrentState, pCurrentMouseStates, 3 * sizeof(uint8_t));

	// KEYBOARD STATES
	int numberOfFetchedkeys = 0;
	const uint8_t* pCurrentKeyStates = SDL_GetKeyboardState(&numberOfFetchedkeys);

	if (numberOfFetchedkeys > 512)
		numberOfFetchedkeys = 512;

	SDL_memcpy(mPreviousState, mCurrentState, 512 * sizeof(uint8_t));
	SDL_memcpy(mCurrentState, pCurrentKeyStates, numberOfFetchedkeys * sizeof(uint8_t));
}



///////////////////////////////////////////
//// MOUSE
bool Window::isButtonPressed(unsigned int keyScanCode)
{
	if (keyScanCode - 1 >= 3)
		return false;
	if (mMouseCurrentState[keyScanCode - 1])
		return true;

	return false;
}

bool Window::isButtonTriggerred(unsigned int keyScanCode)
{
	if (keyScanCode - 1 >= 3)
		return false;
	if (!mMousePreviousState[keyScanCode - 1] && mMouseCurrentState[keyScanCode - 1])
		return true;

	return false;
}

bool Window::isButtonReleased(unsigned int keyScanCode)
{
	if (keyScanCode - 1 >= 3)
		return false;
	if (mMousePreviousState[keyScanCode - 1] && !mMouseCurrentState[keyScanCode - 1])
		return true;

	return false;
}

bool Window::isLeftClicked()
{
	return isButtonPressed(SDL_BUTTON_LEFT);
}

bool Window::isRightClicked()
{
	return isButtonPressed(SDL_BUTTON_RIGHT);
}

int32_t Window::mouseX()
{
	return mMouseX;
}

int32_t Window::mouseY()
{
	return mMouseY;
}

int32_t Window::scrollX()
{
	return ScrollX;
}

int32_t Window::scrollY()
{
	return ScrollY;
}


///////////////////////////////////////////
//// KEYBOARD
bool Window::isKeyPressed(uint32_t keyScanCode)
{
	if (keyScanCode >= 512)
		return false;
	if (mCurrentState[keyScanCode])
		return true;

	return false;
}

bool Window::isKeyTriggered(uint32_t keyScanCode)
{
	if (keyScanCode >= 512)
		return false;
	if (mCurrentState[keyScanCode] && !mPreviousState[keyScanCode])
		return true;

	return false;
}

bool Window::isKeyReleased(uint32_t keyScanCode)
{
	if (keyScanCode >= 512)
		return false;
	if (!mCurrentState[keyScanCode] && mPreviousState[keyScanCode])
		return true;

	return false;
}


///////////////////////////////////////////
//// GUI
/*
void Window::initGui()
{
	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	ImGuiIO& io = ImGui::GetIO(); (void)io;
	io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;

	ImGui::StyleColorsDark();

	//ImGui_ImplSDL2_InitForOpenGL((SDL_Window*)pWindow, gl_context);	TO DO for Vulkan
	ImGui_ImplOpenGL3_Init("#version 130");
}

void Window::exitGui()
{
	ImGui_ImplOpenGL3_Shutdown();
	ImGui_ImplSDL2_Shutdown();
	ImGui::DestroyContext();
}

void Window::beginGuiFrame()
{
	ImGui_ImplOpenGL3_NewFrame();
	//ImGui_ImplSDL2_NewFrame(gpWindow);		TO DO for Vulkan
	ImGui::NewFrame();
}

void Window::endGuiFrame()
{
	ImGui::Render();
	ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
	ImGui::EndFrame();
}
*/


///////////////////////////////////////////
//// FRAME RATE CONTROLLER

void Window::startFrame()
{
	mTickStart = SDL_GetTicks();
}

void Window::endFrame()
{
	mTickEnd = SDL_GetTicks();

	mActualFrameTime = (float)(mTickEnd - mTickStart);

	while (mTickEnd - mTickStart < mNeededTicksPerFrame)
	{
		mTickEnd = SDL_GetTicks();
	}

	mFrameTime = (float)(mTickEnd - mTickStart);
}

float Window::frameRate()
{
	return (float)mFrameTime <= 0.0f ? 0.0f : 1000.0f / mFrameTime;
}

float Window::frameTime()
{
	return mFrameTime;
}

float Window::actualFrameRate()
{
	return (float)mActualFrameTime <= 0.0f ? 0.0f : 1000.0f / mActualFrameTime;
}

float Window::actualFrameTime()
{
	return mActualFrameTime;
}


///////////////////////////////////////////
//// VULKAN

void Window::GetVulkanDrawableSize(int* width, int* height)
{
	SDL_Vulkan_GetDrawableSize((SDL_Window*)pWindow, width, height);
}

void Window::GetVulkanExtensions(uint32_t* _extensionCount, const char*** pExtensionNames)
{
	if (!SDL_Vulkan_GetInstanceExtensions(NULL, _extensionCount, NULL))
		assert(0);
	*pExtensionNames = (const char**)malloc(sizeof(const char*) * (*_extensionCount));
	if (!SDL_Vulkan_GetInstanceExtensions((SDL_Window*)pWindow, _extensionCount, *pExtensionNames))
		assert(0);
}

void Window::CreateVulkanSurface(void* instance, void* surface)
{
	if (SDL_Vulkan_CreateSurface((SDL_Window*)pWindow, (VkInstance_T*)instance, (VkSurfaceKHR_T**)surface) != SDL_TRUE)
	{
		assert(0);
	}
}