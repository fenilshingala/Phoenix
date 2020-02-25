#define PHOENIX_MAIN(Application)									\
int main(int argc, char** argv)										\
{																	\
	try {															\
		Application* app = new Application();						\
		app->Init();												\
		app->Load();												\
																	\
		while (!app->pWindow->windowShouldClose())					\
		{															\
			app->pWindow->update();									\
			if (app->pWindow->isKeyPressed(PH_KEY_ESCAPE))			\
			{														\
				break;												\
			}														\
			app->DrawFrame();										\
		}															\
																	\
		app->waitDeviceIdle();										\
		app->UnLoad();												\
		app->Exit();												\
		delete app;													\
	}																\
	catch (const std::exception & e) {								\
		std::cerr << e.what() << std::endl;							\
		return EXIT_FAILURE;										\
	}																\
																	\
	return EXIT_SUCCESS;											\
}