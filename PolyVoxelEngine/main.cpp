#include "Menu.h"
#include "IniParser.h"

int main()
{
	{
		HWND hwnd = GetConsoleWindow();
		ShowWindow(hwnd, 1); // show console
	}

	// init
	{
		auto parser = IniParser("settings.ini");

		int width = parser.Get<int>("VideoSettings", "Width", 1200);
		int height = parser.Get<int>("VideoSettings", "Height", 800);
		bool vsync = parser.Get<bool>("VideoSettings", "VSync", 0);
		bool fullscreen = parser.Get<bool>("VideoSettings", "Fullscreen", 0);

		int menuMaxFps = parser.Get<int>("FPS", "MenuMaxFps", 20);
		int gameMaxFps = parser.Get<int>("FPS", "GameMaxFps", 60);

		int result = GraphicController::init
		(
			460, width, height, vsync, fullscreen, menuMaxFps, gameMaxFps
		);
		if (result != 0)
		{
			return result;
		}

		result = HardwareUsageInfo::init();
		if (result != 0)
		{
			GraphicController::clean();
			return result;
		}

		result = TextRenderer::init(GraphicController::textProgram);
		if (result != 0)
		{
			HardwareUsageInfo::destroy();
			GraphicController::clean();
			return result;
		}
	}

	// menu
	Menu menu;
	menu.run();

	GraphicController::clean();
	HardwareUsageInfo::destroy();
	TextRenderer::destroy();
	return 0;
}