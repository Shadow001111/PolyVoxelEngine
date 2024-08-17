#include "Menu.h"
#include "Button.h"

Menu::Menu()
{}

void onClick()
{

}

void Menu::run()
{
	Button startButton(0.0f, 0.25f, 0.5f, 0.25f, "START", onClick, AligmentX::Center, AligmentY::Center);
	Button exitButton(0.0f, -0.25f, 0.5f, 0.25f, "EXIT", onClick, AligmentX::Center, AligmentY::Center);


	while (!GraphicController::shouldWindowClose())
	{
		GraphicController::beforeRender();
		{
			GraphicController::buttonProgram->bind();
			startButton.draw();
			exitButton.draw();

			TextRenderer::beforeTextRender();
			{
				startButton.drawText();
				exitButton.drawText();
			}
			TextRenderer::afterTextRender();
		}
		GraphicController::afterRender();
		glfwPollEvents();
	}
	//Game game;
	//game.run();
}
