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

		GraphicSettings graphicSettings =
		{
			460,
			parser.Get<int>("VideoSettings", "Width", 1200),
			parser.Get<int>("VideoSettings", "Height", 800),
			parser.Get<bool>("VideoSettings", "VSync", 0),
			parser.Get<bool>("VideoSettings", "Fullscreen", 0),
			parser.Get<int>("FPS", "MenuMaxFps", 20),
			parser.Get<int>("FPS", "GameMaxFps", 60)
		};

		GameSettings gameSettings =
		{
			parser.Get<float>("Mouse", "Sensitivity", 50),
			parser.Get<bool>("Mouse", "RawMouseInput", false),
		};

		int result = GraphicController::init
		(
			graphicSettings, gameSettings
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