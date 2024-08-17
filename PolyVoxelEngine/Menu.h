#pragma once
#include "Game.h"
#include "Button.h"

class Menu
{
	std::vector<Button> buttons;
public:
	Menu();
	void run();

	void keyCallback(int key, int scancode, int action, int mods);
	void mouseButtonCallback(int button, int action);
	void scrollCallback(int yOffset);
};

