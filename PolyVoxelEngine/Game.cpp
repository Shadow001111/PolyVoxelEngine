#include "Game.h"
#include "settings.h"
#include "Camera.h"
#include "World.h"
#include "TerrainGenerator.h"
#include "Profiler.h"
#include <format>

struct Tick
{
	float timer = 0.0f;
	float delay = 0.0f;

	Tick(int updateRate)
	{
		if (updateRate == 0)
		{
			delay = 0.0f;
			return;
		}
		delay = 1.0f / updateRate;
	}

	void add(float add)
	{
		timer += add;
	}

	bool checkLoop()
	{
		if (delay == 0.0f)
		{
			throw std::exception("Dont use 'checkLoop' when delay = 0");
		}
		else if (timer >= delay)
		{
			timer -= delay;
			return true;
		}
		return false;
	}

	bool checkOnce()
	{
		if (delay == 0.0f)
		{
			return true;
		}
		else if (timer >= delay)
		{
			timer = 0.0f;
			return true;
		}
		return false;
	}

	float getRatio() const
	{
		return timer / delay;
	}
};

void gameKeyCallback(GLFWwindow* window, int key, int scancode, int action, int mods)
{
	Game* game = (Game*)glfwGetWindowUserPointer(window);
	Player* player = game->player;
	player->keyCallback(key, scancode, action, mods);
}

void gameMouseButtonCallback(GLFWwindow* window, int button, int action, int mods)
{
	Game* game = (Game*)glfwGetWindowUserPointer(window);
	Player* player = game->player;
	player->mouseButtonCallback(button, action);
}

void gameScrollCallback(GLFWwindow* window, double xoffset, double yoffset)
{
	Game* game = (Game*)glfwGetWindowUserPointer(window);
	Player* player = game->player;
	player->scrollCallback(yoffset);
}

Game::Game()
{}

void Game::run()
{
	// callbacks
	glfwSetWindowUserPointer(GraphicController::window, (void*)this);
	glfwSetKeyCallback(GraphicController::window, &gameKeyCallback);
	glfwSetScrollCallback(GraphicController::window, &gameScrollCallback);
	glfwSetMouseButtonCallback(GraphicController::window, &gameMouseButtonCallback);

	// world data
	WorldData worldData = World::loadWorldData();

	// player, world
	player = new Player({ 0.0f, 0.0f, 0.0f }, 80.0f, 0.1f, Settings::MAX_RENDER_DISTANCE);
	World world(worldData);
	PhysicEntity::world = &world;

	// player data
	{
		auto& pos = player->physicEntity.position;
		pos.x = worldData.playerPosition.x;
		pos.z = worldData.playerPosition.z;
		if (worldData.playerPosition.y == INT_MIN)
		{
			pos.y = TerrainGenerator::calculateHeight(pos.x, pos.z) + 6;
		}
		else
		{
			pos.y = worldData.playerPosition.y;
		}

		player->rotation = worldData.playerRotation;
		player->previousRotation = worldData.playerRotation;
	}

	// Ticks
	Tick worldTick(20);
	Tick playerTick(40);
	Tick guiTick(10);
	Tick profilerTick(40);

	//
	std::string guiPerfomanceText;

	const std::string debugViewModeNames[4] =
	{
		"Normal", "Light", "Polygon", "Black"
	};

	float frameDelay;
	if (GraphicController::gameMaxFps < 1)
	{
		frameDelay = 0.0f;
	}
	else
	{
		frameDelay = 1.0f / GraphicController::gameMaxFps;
	}

	glm::vec2 rectangleVertices[4] =
	{
		{0.0f, 0.0f}, {1.0f, 0.0f}, {1.0f, 1.0f}, {0.0f, 1.0f}
	};
	VAO rectangleVAO;
	VBO rectangleVBO((const char*)rectangleVertices, sizeof(rectangleVertices), GL_STATIC_DRAW);
	rectangleVAO.linkFloat(2, sizeof(glm::vec2));
	VAO::unbind();

	float previousTime = glfwGetTime();
	while (!GraphicController::shouldWindowClose())
	{
		float currentTime = glfwGetTime();
		float deltaTime = currentTime - previousTime;
		previousTime = currentTime;

		worldTick.add(deltaTime);
		if (currentTime > 1.0f)
		{
			playerTick.add(deltaTime);
		}
		guiTick.add(deltaTime);
		profilerTick.add(deltaTime);

		while (playerTick.checkLoop())
		{
			player->physicUpdate(playerTick.delay, currentTime);
		}
		player->update(playerTick.getRatio());

		while (worldTick.checkLoop())
		{
			world.update(player->physicEntity.position, glm::length2(player->physicEntity.velocity) > 5.0f);
		}

		if (guiTick.checkOnce())
		{
			// TODO: find data per proccess
			auto* gpu = HardwareUsageInfo::getGPUUtilization();
			auto* vram = HardwareUsageInfo::getVRAMUsage();

			guiPerfomanceText.clear();

			guiPerfomanceText += "FPS: ";
			guiPerfomanceText += std::to_string(int(1.0f / deltaTime));

			guiPerfomanceText += "\nCPU: ";
			guiPerfomanceText += std::to_string(HardwareUsageInfo::getCPUUsage());

			guiPerfomanceText += "\nRAM: ";
			guiPerfomanceText += std::to_string(HardwareUsageInfo::getRAMUsage() >> 20);

			guiPerfomanceText += "\nGPU: ";
			guiPerfomanceText += std::to_string(gpu->gpu);

			guiPerfomanceText += "\nVRAM: ";
			guiPerfomanceText += std::to_string(vram->used >> 20);

			guiPerfomanceText += "\nDrawCommands: ";
			guiPerfomanceText += std::to_string(world.drawCommandsCount);
		}

		if (profilerTick.checkOnce())
		{
			Profiler::saveToMemory();
		}

		GraphicController::beforeRender();
		{
			player->BeforeRender();

			world.draw(player->camera);
			player->draw();

			// profiler
			rectangleVAO.bind();
			rectangleVBO.bind();
			GraphicController::rectangleProgram->bind();
			{
				const float barWidth = PROFILER_DRAW_WIDTH / PROFILER_MEMORY_TABLE_SIZE;
				const float left = -GraphicController::aspectRatio;
				const float bottom = -1.0f;

				// background
				GraphicController::rectangleProgram->setUniformFloat2("position", left, bottom);
				GraphicController::rectangleProgram->setUniformFloat2("scale", PROFILER_DRAW_WIDTH, PROFILER_DRAW_HEIGHT);
				GraphicController::rectangleProgram->setUniformFloat3("color", 0.0f, 0.0f, 0.0f);
				glDrawArrays(GL_TRIANGLE_FAN, 0, 4);

				// bars
				if (Profiler::maxTime > 0)
				{
					for (size_t barIndex = 0; barIndex < PROFILER_MEMORY_TABLE_SIZE; barIndex++)
					{
						size_t tableIndex = Profiler::memoryTableIndex + barIndex;
						if (tableIndex >= PROFILER_MEMORY_TABLE_SIZE)
						{
							tableIndex -= PROFILER_MEMORY_TABLE_SIZE;
						}
						float yProgress = 0.0f;
						for (size_t i = 0; i < PROFILER_SAMPLES_COUNT; i++)
						{
							float time = (float)Profiler::memoryTable[tableIndex][i] / (float)Profiler::maxTime;
							if (time > 0.0f)
							{
								const auto& color = profilerSamplesColors[i];
								GraphicController::rectangleProgram->setUniformFloat2("position", left + barIndex * barWidth, bottom + yProgress * PROFILER_DRAW_HEIGHT);
								GraphicController::rectangleProgram->setUniformFloat2("scale", barWidth, PROFILER_DRAW_HEIGHT * time);
								GraphicController::rectangleProgram->setUniformFloat3("color", color.x, color.y, color.z);
								glDrawArrays(GL_TRIANGLE_FAN, 0, 4);
								yProgress += time;
							}
						}
					}
				}

				// colors
				GraphicController::rectangleProgram->setUniformFloat2("scale", PROFILER_DRAW_COLOR_RECT_SIZE, PROFILER_DRAW_COLOR_RECT_SIZE);
				for (size_t i = 0; i < PROFILER_SAMPLES_COUNT; i++)
				{
					const auto& color = profilerSamplesColors[i];
					GraphicController::rectangleProgram->setUniformFloat2("position", left, bottom + PROFILER_DRAW_HEIGHT + 0.01f + i * (PROFILER_DRAW_COLOR_RECT_Y_OFFSET + PROFILER_DRAW_COLOR_RECT_SIZE));
					GraphicController::rectangleProgram->setUniformFloat3("color", color.x, color.y, color.z);
					glDrawArrays(GL_TRIANGLE_FAN, 0, 4);
				}
			}

			// text
			TextRenderer::beforeTextRender();
			{
				{
					float offsetX = 0.02f;
					float offsetY = offsetX;

					float x = -1.0f * GraphicController::aspectRatio + offsetX;
					float y = 1.0f - offsetY;

					float scale = 0.03f;

					TextRenderer::renderText(guiPerfomanceText, x, y, scale, glm::vec3(1.0f, 0.0f, 0.0f), AligmentX::Left, AligmentY::Top);
				}
				{
					float offsetX = 0.02f;
					float offsetY = offsetX;

					float scale = 0.05f;

					float x = 1.0f * GraphicController::aspectRatio - offsetX;
					float y = 1.0f - offsetY;

					const auto& playerPos = player->physicEntity.position;

					std::string text = "ViewMode: ";
					text += debugViewModeNames[player->debugViewMode];
					text += "\nX: ";
					text += std::to_string((int)floorf(playerPos.x));
					text += " Y: ";
					text += std::to_string((int)floorf(playerPos.y));
					text += " Z: ";
					text += std::to_string((int)floorf(playerPos.z));

					text += "\nX: ";
					text += std::format("{:.2f}", player->camera.Forward.x);
					text += " Y: ";
					text += std::format("{:.2f}", player->camera.Forward.y);
					text += " Z: ";
					text += std::format("{:.2f}", player->camera.Forward.z);

					TextRenderer::renderText(text, x, y, scale, glm::vec3(1.0f, 0.0f, 0.0f), AligmentX::Right, AligmentY::Top);
				}
				// profiler
				{
					const float left = -GraphicController::aspectRatio;
					const float bottom = -1.0f;
					for (size_t i = 0; i < PROFILER_SAMPLES_COUNT; i++)
					{
						TextRenderer::renderText(profilerSamplesNames[i],
							left + PROFILER_DRAW_COLOR_RECT_SIZE + 0.01f,
							bottom + PROFILER_DRAW_HEIGHT + 0.01f + i * (PROFILER_DRAW_COLOR_RECT_Y_OFFSET + PROFILER_DRAW_COLOR_RECT_SIZE) + PROFILER_DRAW_COLOR_RECT_SIZE,
							PROFILER_DRAW_COLOR_RECT_SIZE * 0.5f,
							{ 1.0f, 1.0f, 1.0f },
							AligmentX::Left,
							AligmentY::Top
						);
					}

					TextRenderer::renderText(std::to_string(Profiler::maxTime) + " ms",
						left + PROFILER_DRAW_WIDTH + 0.01f,
						bottom + PROFILER_DRAW_HEIGHT,
						0.025f,
						{ 1.0f, 1.0f, 1.0f },
						AligmentX::Left, AligmentY::Center);
				}
			}
			TextRenderer::afterTextRender();
		}
		GraphicController::afterRender();

		glfwPollEvents();

		float frameTime = glfwGetTime() - currentTime;
		if (frameTime < frameDelay)
		{
			std::chrono::milliseconds duration(int((frameDelay - frameTime) * 1000.0f));
			std::this_thread::sleep_for(duration);
		}
	}

	// save world data
	worldData.worldTime = world.time;

	worldData.playerPosition = player->physicEntity.position;
	worldData.playerRotation = player->rotation;

	world.saveWorldData(worldData);

	player->clean(); delete player;

	Profiler::clean();
}
