#include "Menu.h"

int main()
{
	{
		HWND hwnd = GetConsoleWindow();
		ShowWindow(hwnd, 1); // show console
	}

	// init
	{
		int height = 800;
		float ratio = 16.0f / 9.0f;
		int width = height * ratio;

		int result = GraphicController::init
		(
			width, height, false, 460
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