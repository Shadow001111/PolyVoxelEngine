#pragma once
#pragma comment(lib, "dxgi.lib") // Link with DXGI library
#undef max
#undef min

#include "Player.h"

class Game
{
public:
	Player* player = nullptr;

	Game();
	void run();
};

