#include "Menu.h"
#include "Button.h"

Menu::Menu()
{}

void onClick()
{

}

void Menu::run()
{
	Button button(0.0f, 0.0f, 0.5f, 0.5f, "label", onClick, AligmentX::Center, AligmentY::Center);
	while (!GraphicController::shouldWindowClose())
	{
		GraphicController::beforeRender();
		{
			GraphicController::buttonProgram->bind();
			button.draw();

			TextRenderer::beforeTextRender();
			{
				button.drawText();
			}
			TextRenderer::afterTextRender();
		}
		GraphicController::afterRender();
		glfwPollEvents();
	}
	/*Game game;
	game.run();*/
}
