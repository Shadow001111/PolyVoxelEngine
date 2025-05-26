#pragma once
#pragma comment(lib, "dxgi.lib") // Link with DXGI library
#undef max
#undef min

#include "Player.h"

class Game
{
	void renderImGui(float deltaTime, const World& world) const;
public:
	Player* player = nullptr;

	Game();
	void run();
};

