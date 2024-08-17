#pragma once
#include <Windows.h>
#include <dxgi.h>
#pragma comment(lib, "dxgi.lib") // Link with DXGI library
#undef max
#undef min

#include "TextRenderer.h"
#include "HardwareUsageInfo.h"

#include "Player.h"

class Game
{
	Player* player = nullptr;
public:
	Game();
	void run();
};

