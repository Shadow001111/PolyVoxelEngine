#pragma once
#include <Windows.h>
#include <dxgi.h>
#pragma comment(lib, "dxgi.lib") // Link with DXGI library
#undef max
#undef min

#include "TextRenderer.h"
#include "HardwareUsageInfo.h"

class Game
{
public:
	Game();
	void run();
};

