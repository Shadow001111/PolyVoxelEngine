#include "Game.h"
#include "settings.h"
#include "Camera.h"
#include "World.h"
#include "Profiler.h"
#include <format>
#include <thread>
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <glm/fwd.hpp>
#include <glm/gtx/norm.inl>
#include "GraphicController.h"
#include "HardwareUsageInfo.h"
#include "PhysicEntity.h"
#include "Player.h"
#include "TextRenderer.h"
#include "VAO.h"
#include "VBO.h"
#include <chrono>
#include <exception>
#include <string>

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

static void gameKeyCallback(GLFWwindow* window, int key, int scancode, int action, int mods)
{
	Game* game = (Game*)glfwGetWindowUserPointer(window);
	Player* player = game->player;
	player->keyCallback(key, scancode, action, mods);
}

static void gameMouseButtonCallback(GLFWwindow* window, int button, int action, int mods)
{
	auto& io = ImGui::GetIO();
	if (io.WantCaptureMouse || io.WantCaptureKeyboard)
	{
		if (button >= 0 && button < IM_ARRAYSIZE(io.MouseDown))
		{
			io.MouseDown[button] = (action == GLFW_PRESS);
		}
		return;
	}

	Game* game = (Game*)glfwGetWindowUserPointer(window);
	Player* player = game->player;
	player->mouseButtonCallback(button, action);
}

static void gameScrollCallback(GLFWwindow* window, double xoffset, double yoffset)
{
	auto& io = ImGui::GetIO();
	if (io.WantCaptureMouse || io.WantCaptureKeyboard)
	{
		return;
	}

	Game* game = (Game*)glfwGetWindowUserPointer(window);
	Player* player = game->player;
	player->scrollCallback(yoffset);
}

void Game::renderImGui(float deltaTime, const World& world) const
{
	ImGui::SetNextWindowPos(ImVec2(0, 0), ImGuiCond_Once);

	ImGui_ImplOpenGL3_NewFrame();
	ImGui_ImplGlfw_NewFrame();
	ImGui::NewFrame();

	//
	ImGui::Begin("IGui");

	if (ImGui::CollapsingHeader("Info"))
	{
		const auto& worldInfo = world.getWorldInfo();

		//
		ImGui::Text("FPS: %d / %d", int(1.0f / deltaTime), GraphicController::gameMaxFps);
		ImGui::Text("Delta time: %f", deltaTime);
		ImGui::Dummy(ImVec2(0.0f, 20.0f));


		// world info
		{
			ImGui::Text("WorldInfo:");
			ImGui::Indent(20.0f);

			ImGui::Text("Time: %d/24000", worldInfo.time);

			ImGui::Unindent(20.0f);
			ImGui::Dummy(ImVec2(0.0f, 20.0f));
		}
		
		// chunks info
		{
			ImGui::Text("ChunksInfo:");
			ImGui::Indent(20.0f);

			ImGui::Text("Chunks count: %d", worldInfo.chunksCount);
			ImGui::Text("Chunks with faces: %d", worldInfo.chunksWithFacesCount);
			ImGui::Text("Chunks that passed frustum culling: %d", worldInfo.frustumPassedChunksCount);

			ImGui::Text("Chunks' sides 'have passed test'/'total': %d/%d | %.2f%%", worldInfo.chunkSidesPassed, worldInfo.chunkSidesTotal, (float)worldInfo.chunkSidesPassed / (float)worldInfo.chunkSidesTotal * 100.0f);

			ImGui::Text("Draw commands: %d", worldInfo.drawCommandsCount);

			ImGui::Unindent(20.0f);
			ImGui::Dummy(ImVec2(0.0f, 20.0f));
		}
	}

	if (ImGui::CollapsingHeader("Profiler"))
	{
		static float samples[PROFILER_CATEGORIES_COUNT][PROFILER_MEMORY_TABLE_SIZE];
		int index = Profiler::memoryTableIndex;

		// Prepare data and find max value (optional for color normalization or something else)
		float maxValue = 0.0f;
		for (size_t category = 0; category < PROFILER_CATEGORIES_COUNT; category++)
		{
			for (size_t i = 0; i < PROFILER_MEMORY_TABLE_SIZE; i++)
			{
				float value = static_cast<float>(Profiler::systemTimeSamplesMemoryTable[category][(index + i) % PROFILER_MEMORY_TABLE_SIZE]) * 1e-6f;
				samples[category][i] = value;
				if (value > maxValue)
				{
					maxValue = value;
				}
			}
		}

		// Prepare category labels
		const char* labels[PROFILER_CATEGORIES_COUNT];
		for (int i = 0; i < PROFILER_CATEGORIES_COUNT; i++)
		{
			labels[i] = profilerSamplesNames[i].c_str();
		}

		// Flatten samples into a row-major matrix for PlotBarGroups: [category][frame]
		std::vector<float> values(PROFILER_CATEGORIES_COUNT * PROFILER_MEMORY_TABLE_SIZE);
		for (int row = 0; row < PROFILER_CATEGORIES_COUNT; row++)
		{
			for (int col = 0; col < PROFILER_MEMORY_TABLE_SIZE; col++)
			{
				values[row * PROFILER_MEMORY_TABLE_SIZE + col] = samples[row][col];
			}
		}

		// Compute stacked max Y axis value (sum of categories per frame)
		float stackedMaxValue = 0.0f;
		for (size_t i = 0; i < PROFILER_MEMORY_TABLE_SIZE; i++)
		{
			float sum = 0.0f;
			for (size_t c = 0; c < PROFILER_CATEGORIES_COUNT; c++)
			{
				sum += samples[c][i];
			}
			if (sum > stackedMaxValue)
			{
				stackedMaxValue = sum;
			}
		}

		if (ImPlot::BeginPlot("Systems Time Usage", ImVec2(-1, 300)))
		{
			float yLimit = fmaxf(30.0f, stackedMaxValue);

			ImPlot::SetupAxes("Time stamp", "Time (ms)", ImPlotAxisFlags_NoGridLines, ImPlotAxisFlags_NoGridLines);
			ImPlot::SetupAxisLimits(ImAxis_X1, 0, static_cast<float>(PROFILER_MEMORY_TABLE_SIZE), ImGuiCond_Always);
			ImPlot::SetupAxisLimits(ImAxis_Y1, 0, yLimit, ImGuiCond_Always);

			ImPlot::PlotBarGroups(labels, values.data(), PROFILER_CATEGORIES_COUNT, PROFILER_MEMORY_TABLE_SIZE, 0.8, 0, ImPlotBarGroupsFlags_Stacked);

			ImPlot::EndPlot();
		}
	}

	if (ImGui::CollapsingHeader("Profiler2"))
	{
		int index = Profiler::memoryTableIndex;
		static float samples[PROFILER_MEMORY_TABLE_SIZE];

		for (const auto& it : Profiler::profilerThreadData)
		{
			const auto& threadData = it.second;

			// Prepare data
			float maxValue = 0.0f;
			for (size_t i = 0; i < PROFILER_MEMORY_TABLE_SIZE; i++)
			{
				float value = threadData.timeSamplesMemoryTable[(index + i) % PROFILER_MEMORY_TABLE_SIZE] * 1e-6f;
				samples[i] = value;
				if (value > maxValue)
				{
					maxValue = value;
				}
			}

			if (ImPlot::BeginPlot("Thread # Time Usage", ImVec2(-1, 300)))
			{
				float yLimit = fmaxf(30.0f, maxValue);

				ImPlot::SetupAxes("Time stamp", "Time (ms)", ImPlotAxisFlags_NoGridLines, ImPlotAxisFlags_NoGridLines);
				ImPlot::SetupAxisLimits(ImAxis_X1, 0, static_cast<float>(PROFILER_MEMORY_TABLE_SIZE), ImGuiCond_Always);
				ImPlot::SetupAxisLimits(ImAxis_Y1, 0, yLimit, ImGuiCond_Always);

				ImPlot::PlotBars("Thread #", samples, PROFILER_MEMORY_TABLE_SIZE, 0.8);

				ImPlot::EndPlot();
			}
		}
	}

	//
	ImGui::End();
	ImGui::Render();
	ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

	// TODO: add world time
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

	// world
	WorldData worldData = World::loadWorldData();
	World world(worldData);
	World::worldDataGetPlayerY(worldData);

	// player
	player = new Player({ 0.0f, 0.0f, 0.0f }, 80.0f, 0.1f, Settings::MAX_RENDER_DISTANCE);
	PhysicEntity::world = &world;

	// player data
	{
		player->physicEntity.position = worldData.playerPosition;
		player->physicEntity.previousPosition = worldData.playerPosition;

		player->rotation = worldData.playerRotation;
		player->previousRotation = worldData.playerRotation;
	}

	// Ticks
	Tick worldTick(20);
	Tick playerTick(40);
	Tick profilerTick(20);

	//

	const std::string debugViewModeNames[5] =
	{
		"Normal", "Light", "Polygon", "Black", "FullLight"
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
		glfwPollEvents();

		// update
		float currentTime = glfwGetTime();
		float deltaTime = currentTime - previousTime;
		previousTime = currentTime;

		worldTick.add(deltaTime);
		if (currentTime > 1.0f)
		{
			playerTick.add(deltaTime);
		}
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

		if (profilerTick.checkOnce())
		{
			Profiler::saveToMemory();
		}

		// render
		GraphicController::beforeRender();
		{
			player->BeforeRender();

			world.draw(player->camera);
			player->draw();

			// text
			TextRenderer::beforeTextRender();
			{
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
			}
			TextRenderer::afterTextRender();
		}
		renderImGui(deltaTime, world);
		GraphicController::afterRender();

		//
		std::cout << std::flush;

		float frameTime = glfwGetTime() - currentTime;
		if (frameTime < frameDelay)
		{
			std::chrono::milliseconds duration(int((frameDelay - frameTime) * 1000.0f));
			std::this_thread::sleep_for(duration);
		}
	}

	// save world data
	world.updateWorldInfo();
	const auto& worldInfo = world.getWorldInfo();
	worldData.worldTime = worldInfo.time;

	worldData.playerPosition = player->physicEntity.position;
	worldData.playerRotation = player->rotation;

	world.saveWorldData(worldData);

	player->clean(); delete player;

	Profiler::clean();
}
