#include "Menu.h"
#include "IniParser.h"
#include "SoundEngine.h"

#include "SimplexNoise.h"

int main()
{
	//auto start = std::chrono::steady_clock::now();
	//// NOISE TEST
	//int size = 64;
	//float scale = 1.0f / size;
	//unsigned int times = (1 << 10);
	//for (unsigned int i = 0; i < times; i++)
	//{
	//	SimplexNoise noise(i);
	//	for (int x = 0; x < size; x++)
	//	{
	//		for (int y = 0; y < size; y++)
	//		{
	//			for (int z = 0; z < size; z++)
	//			{
	//				float value = noise.noise(x * scale, y * scale, z * scale);
	//			}
	//		}
	//	}
	//}

	////
	//auto end = std::chrono::steady_clock::now();
	//auto duration = std::chrono::duration_cast<std::chrono::nanoseconds>(end - start);
	//std::cout << duration.count() / times << std::endl;
	//return 0;


	{
		HWND hwnd = GetConsoleWindow();
		ShowWindow(hwnd, 1); // show console
	}

	// init
	{
		auto parser = IniParser("res/settings.ini");

		GraphicSettings graphicSettings =
		{
			460,
			"res/PolyVoxelEngine.png",

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

		result = SoundEngine::init();
		if (result != 0)
		{
			HardwareUsageInfo::destroy();
			GraphicController::clean();
			TextRenderer::destroy();
			return result;
		}
	}
	
	// menu
	Menu menu;
	menu.run();

	GraphicController::clean();
	HardwareUsageInfo::destroy();
	TextRenderer::destroy();
	SoundEngine::clean();
	return 0;
}