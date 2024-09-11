#include "Menu.h"
#include <chrono>
#include <thread>

void menuKeyCallback(GLFWwindow* window, int key, int scancode, int action, int mods)
{
	Menu* menu = (Menu*)glfwGetWindowUserPointer(window);
	menu->keyCallback(key, scancode, action, mods);
}

void menuMouseButtonCallback(GLFWwindow* window, int button, int action, int mods)
{
	Menu* menu = (Menu*)glfwGetWindowUserPointer(window);
	menu->mouseButtonCallback(button, action);
}

void menuScrollCallback(GLFWwindow* window, double xoffset, double yoffset)
{
	Menu* menu = (Menu*)glfwGetWindowUserPointer(window);
	menu->scrollCallback(yoffset);
}

Menu::Menu()
{}

void Menu::run()
{
	GraphicController::setCursorMode(GLFW_CURSOR_NORMAL);

	glfwSetWindowUserPointer(GraphicController::window, (void*)this);
	glfwSetKeyCallback(GraphicController::window, &menuKeyCallback);
	glfwSetScrollCallback(GraphicController::window, &menuScrollCallback);
	glfwSetMouseButtonCallback(GraphicController::window, &menuMouseButtonCallback);

	bool closeMenu = false;

	auto startGame = [&]()
		{
			closeMenu = true;
		};

	auto closeApp = []()
		{
			GraphicController::closeWindow();
		};

	buttons.emplace_back(0.0f, 0.25f, 0.5f, 0.25f, "START", startGame, AligmentX::Center, AligmentY::Center);
	buttons.emplace_back(0.0f, -0.25f, 0.5f, 0.25f, "EXIT", closeApp, AligmentX::Center, AligmentY::Center);

	float frameDelay;
	if (GraphicController::menuMaxFps < 1)
	{
		frameDelay = 0.0f;
	}
	else
	{
		frameDelay = 1.0f / GraphicController::menuMaxFps;
	}

	while (!GraphicController::shouldWindowClose())
	{
		while (!closeMenu && !GraphicController::shouldWindowClose())
		{
			float frameStart = glfwGetTime();
			GraphicController::beforeRender();
			{
				GraphicController::buttonProgram->bind();
				for (const Button& button : buttons)
				{
					button.draw();
				}

				TextRenderer::beforeTextRender();
				{
					for (const Button& button : buttons)
					{
						button.drawText();
					}
				}
				TextRenderer::afterTextRender();
			}
			GraphicController::afterRender();
			glfwPollEvents();
			
			float frameTime = glfwGetTime() - frameStart;
			if (frameTime < frameDelay)
			{
				std::chrono::milliseconds duration(int((frameDelay - frameTime) * 1000.0f));
				std::this_thread::sleep_for(duration);
			}
		}
		closeMenu = false;
		if (!GraphicController::shouldWindowClose())
		{
			Game game;
			game.run();
		}
	}
}

void Menu::keyCallback(int key, int scancode, int action, int mods)
{
	
}

void Menu::mouseButtonCallback(int button, int action)
{
	if (action != GLFW_PRESS)
	{
		double mouseX, mouseY;
		glfwGetCursorPos(GraphicController::window, &mouseX, &mouseY);

		mouseX = GraphicController::aspectRatio * ((float)mouseX / (float)GraphicController::width * 2.0f - 1.0f);
		mouseY = -((float)mouseY / (float)GraphicController::height * 2.0f - 1.0f);

		for (const Button& button : buttons)
		{
			if (button.click(mouseX, mouseY))
			{
				break;
			}
		}
	}
}

void Menu::scrollCallback(int yOffset)
{
}
