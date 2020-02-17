#include "Window.h"

#include <assert.h>

#include <glad/glad.h>

#include "SDL_stdinc.h"
#define SDL_MAIN_HANDLED
#include <SDL.h>
#include <SDL_keyboard.h>

#include <imgui/imgui_impl_sdl.h>
#include <imgui/imgui_impl_opengl3.h>

static SDL_Window* gpWindow = NULL;
static int SCR_WIDTH  = 0;
static int SCR_HEIGHT = 0;
static int suitableWidth  = 0;
static int suitableHeight = 0;
static SDL_GLContext gl_context;

static bool exitProgram = false;

// MOUSE
bool _leftClicked = false;
bool _rightClicked = false;

uint8_t pCurrentMouseStates[3] = { 0 };
uint8_t mMouseCurrentState[3]  = { 0 };
uint8_t mMousePreviousState[3] = { 0 };

int32_t lastMouseX = 0, lastMouseY = 0, MouseX = 0, MouseY = 0;

bool isButtonPressed(unsigned int keyScanCode)
{
	if (keyScanCode - 1 >= 3)
		return false;
	if (mMouseCurrentState[keyScanCode - 1])
		return true;

	return false;
}

bool isButtonTriggerred(unsigned int keyScanCode) {
	if (keyScanCode - 1 >= 3)
		return false;
	if (!mMousePreviousState[keyScanCode - 1] && mMouseCurrentState[keyScanCode - 1])
		return true;

	return false;
}

bool isButtonReleased(unsigned int keyScanCode)
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
	return MouseX;
}

int32_t Window::mouseY()
{
	return MouseY;
}

int32_t ScrollX = 0, ScrollY = 0;
int32_t Window::scrollX()
{
	return ScrollX;
}

int32_t Window::scrollY()
{
	return ScrollY;
}

// KEYBOARD
uint8_t mCurrentState[PH_MAX_KEYS]  = { 0 };
uint8_t mPreviousState[PH_MAX_KEYS] = { 0 };

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

Window::Window()
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

	suitableWidth = SCR_WIDTH;
	suitableHeight = SCR_HEIGHT;

	uint32_t flags = SDL_WINDOW_OPENGL;
/*#ifndef _DEBUG
	flags |= SDL_WINDOW_FULLSCREEN_DESKTOP;
#endif*/

	gpWindow = SDL_CreateWindow("My Cool Framework!",
		SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
		SCR_WIDTH, SCR_HEIGHT,
		flags);

	SDL_GetWindowSize(gpWindow, &SCR_WIDTH, &SCR_HEIGHT);

	if (gpWindow == NULL)
	{
		assert(0);
	}

	/*
		// WINDOW ICON
		{
			int orig_format, width, height;
			int req_format = STBI_rgb_alpha;
			SDL_Surface* icon = NULL;
			unsigned char* pixels = stbi_load("../Common/Resources/Assets/Textures/fireflies_icon.png", &width, &height, &orig_format, req_format);

			Uint32 rmask, gmask, bmask, amask;
	#if SDL_BYTEORDER == SDL_BIG_ENDIAN
			int shift = (req_format == STBI_rgb) ? 8 : 0;
			rmask = 0xff000000 >> shift;
			gmask = 0x00ff0000 >> shift;
			bmask = 0x0000ff00 >> shift;
			amask = 0x000000ff >> shift;
	#else // little endian, like x86
			rmask = 0x000000ff;
			gmask = 0x0000ff00;
			bmask = 0x00ff0000;
			amask = (req_format == STBI_rgb) ? 0 : 0xff000000;
	#endif

			int depth, pitch;
			if (req_format == STBI_rgb)
			{
				depth = 24;
				pitch = 3 * width; // 3 bytes per pixel * pixels per row
			}
			else
			{ // STBI_rgb_alpha (RGBA)
				depth = 32;
				pitch = 4 * width;
			}

			icon = SDL_CreateRGBSurfaceFrom((void*)pixels, width, height, depth, pitch,
				rmask, gmask, bmask, amask);

			if (icon == NULL)
			{
				SDL_Log("Creating surface failed: %s", SDL_GetError());
				stbi_image_free(pixels);
				exit(1);
			}

			SDL_SetWindowIcon(gpWindow, icon);
			stbi_image_free(pixels);
		}
	*/
	gl_context = SDL_GL_CreateContext(gpWindow);

	if (!gladLoadGLLoader((GLADloadproc)SDL_GL_GetProcAddress))
	{
		assert(0);
	}

	SDL_GL_MakeCurrent(gpWindow, gl_context);
}

void Window::exitWindow()
{
	SDL_GL_DeleteContext(gl_context);
	SDL_DestroyWindow(gpWindow);
	SDL_Quit();
}

void Window::update()
{
	// mouse coordinates
	lastMouseX = (int32_t)MouseX;
	lastMouseY = (int32_t)MouseY;

	SDL_Event e;
	while (SDL_PollEvent(&e) != 0)
	{
		ImGui_ImplSDL2_ProcessEvent(&e);
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
				//if (resizeCallback)
				//	resizeCallback(e.window.data1, e.window.data2);
				break;
			case SDL_WINDOWEVENT_MINIMIZED:
				//if (focusCallback)
				//	focusCallback(false);
				break;
			}

			break;

		case SDL_MOUSEBUTTONUP:
			pCurrentMouseStates[e.button.button - 1] = 0;
			break;

		case SDL_MOUSEBUTTONDOWN:
			pCurrentMouseStates[e.button.button - 1] = 1;
			break;

		case SDL_MOUSEMOTION:
			MouseX = e.motion.x;
			MouseY = e.motion.y;
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

int Window::windowWidth()
{
	return SCR_WIDTH;
}

int Window::windowHeight()
{
	return SCR_HEIGHT;
}

bool Window::windowShouldClose()
{
	return exitProgram;
}

void Window::swapWindow()
{
	SDL_GL_SwapWindow(gpWindow);
}


///////////////////////////////////////////
//// GUI

void Window::initGui()
{
	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	ImGuiIO& io = ImGui::GetIO(); (void)io;
	io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;

	ImGui::StyleColorsDark();

	ImGui_ImplSDL2_InitForOpenGL(gpWindow, gl_context);
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
	ImGui_ImplSDL2_NewFrame(gpWindow);
	ImGui::NewFrame();
}

void Window::endGuiFrame()
{
	ImGui::Render();
	ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
	ImGui::EndFrame();
}


// FRAME RATE
uint32_t mTickStart = 0 , mTickEnd = 0;
float mNeededTicksPerFrame = 16.67f, mFrameTime = 0.0f, mActualFrameTime = 0.0f;

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