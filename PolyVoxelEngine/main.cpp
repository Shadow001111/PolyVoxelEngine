#include "Game.h"

#include <Windows.h>
#include <dxgi.h>
#pragma comment(lib, "dxgi.lib") // Link with DXGI library
#undef max
#undef min

int main()
{
	{
		HWND hwnd = GetConsoleWindow();
		ShowWindow(hwnd, 1); // show console
	}

	// init
	{
		int result = GraphicController::init
		(
			1200, 800, false, 460
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

		result = TextRenderer::init();
		if (result != 0)
		{
			HardwareUsageInfo::destroy();
			GraphicController::clean();
			return result;
		}
	}

	// game loop
	Game game;
	game.run();

	GraphicController::clean();
	HardwareUsageInfo::destroy();
	TextRenderer::destroy();
	return 0;
}