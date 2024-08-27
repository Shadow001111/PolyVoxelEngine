#include "Game.h"
#include "settings.h"
#include "Camera.h"
#include "World.h"
#include "TerrainGenerator.h"
#include "Profiler.h"

float ceilTo(float value, int n)
{
	float x = powf(10.0f, n);
	return ceilf(value * x) / x;
}

bool checkMemory()
{
	float kb = 1024.0f;
	float mb = kb * 1024.0f;
	float gb = mb * 1024.0f;
	{
		MEMORYSTATUSEX memStatus;
		memStatus.dwLength = sizeof(memStatus);
		GlobalMemoryStatusEx(&memStatus);

		float maxRAMLimit = memStatus.ullTotalPhys;
		float RAMLimit = maxRAMLimit * (7.0f / 8.0f);

		size_t memory = 0;

		memory += Settings::MAX_RENDERED_CHUNKS_COUNT * (sizeof(Chunk) + sizeof(Chunk*)) +
			Settings::MAX_CHUNK_DRAW_COMMANDS_COUNT * sizeof(DrawArraysIndirectCommand) +
			Settings::MAX_RENDERED_CHUNKS_COUNT * sizeof(glm::vec3) +
			Settings::MAX_CHUNK_DRAW_COMMANDS_COUNT * sizeof(unsigned int) +
			Settings::FACE_INSTANCES_PER_CHUNK * sizeof(FaceInstanceData) +
			calcArea(Settings::CHUNK_LOAD_RADIUS) * (sizeof(int) + sizeof(HeightMap*) * 2 + sizeof(HeightMap)) +
			Settings::CHUNK_SIZE_CUBED * 6 * sizeof(Face)
			;

		if (memory > gb)
		{
			std::cout << "Ram: " << ceilTo(memory / gb, 2) << " / " << ceilTo(RAMLimit / gb, 2) << " gb" << std::endl;
		}
		else
		{
			std::cout << "Ram: " << ceilTo(memory / mb, 2) << " / " << ceilTo(RAMLimit / mb, 2) << " mb" << std::endl;
		}

		if (memory > RAMLimit)
		{
			std::cerr << "Not enough memory" << std::endl;
			return true;
		}
	}
	{
		IDXGIFactory* pFactory = nullptr;
		if (FAILED(CreateDXGIFactory(__uuidof(IDXGIFactory), (void**)&pFactory)))
		{
			std::cerr << "Failed to create DXGI factory" << std::endl;
			return true;
		}
		IDXGIAdapter* pAdapter = nullptr;
		if (FAILED(pFactory->EnumAdapters(0, &pAdapter)))
		{
			pFactory->Release();
			std::cerr << "Failed to get adapter" << std::endl;
			return true;
		}
		DXGI_ADAPTER_DESC adapterDesc;
		if (FAILED(pAdapter->GetDesc(&adapterDesc)))
		{
			pAdapter->Release();
			pFactory->Release();
			std::cerr << "Failed to get adapter description" << std::endl;
			return true;
		}

		float maxVRAMLimit = adapterDesc.DedicatedVideoMemory + adapterDesc.SharedSystemMemory;
		float VRAMLimit = maxVRAMLimit * (7.0f / 8.0f);

		pAdapter->Release();
		pFactory->Release();

		float vmemory = Settings::MAX_RENDERED_CHUNKS_COUNT * Settings::FACE_INSTANCES_PER_CHUNK * sizeof(FaceInstanceData) +
			Settings::MAX_CHUNK_DRAW_COMMANDS_COUNT * sizeof(DrawArraysIndirectCommand) +
			137065 * 3 * sizeof(float) +
			137065 * 12 * sizeof(unsigned int);

		int blockTexturesMipmapLevels = 1 + (int)log2f(Settings::BLOCK_TEXTURE_SIZE);
		for (int lvl = 0; lvl < blockTexturesMipmapLevels; lvl++)
		{
			int div = 1 << lvl;
			vmemory += Settings::BLOCK_TEXTURE_SIZE_IN_BYTES * Settings::BLOCK_TEXTURES_COUNT / (div * div);
		}

		if (vmemory > gb)
		{
			std::cout << "VRam: " << ceilTo(vmemory / gb, 2) << " / " << ceilTo(VRAMLimit / gb, 2) << " gb" << std::endl;
		}
		else
		{
			std::cout << "VRam: " << ceilTo(vmemory / mb, 2) << " / " << ceilTo(VRAMLimit / mb, 2) << " mb" << std::endl;
		}

		if (vmemory > VRAMLimit)
		{
			std::cerr << "Not enough video-memory" << std::endl;
			return true;
		}
	}
	return false;
}

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

struct GUIData
{
	int fps = 0;

	uint8_t cpu = 0;
	size_t ram = 0;

	uint8_t gpu = 0;
	size_t vram = 0;
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
	if (checkMemory())
	{
		return;
	}

	// callbacks
	glfwSetWindowUserPointer(GraphicController::window, (void*)this);
	glfwSetKeyCallback(GraphicController::window, &gameKeyCallback);
	glfwSetScrollCallback(GraphicController::window, &gameScrollCallback);
	glfwSetMouseButtonCallback(GraphicController::window, &gameMouseButtonCallback);

	// player, world
	player = new Player({ 0.0f, 0.0f, 0.0f }, 80.0f, 0.1f, Settings::MAX_RENDER_DISTANCE);
	World world(0);
	PhysicEntity::world = &world;

	// load data
	{
		WorldData worldData = world.loadWorldData();

		// world
		world.time = worldData.worldTime;

		// player
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
	GUIData guiData;

	const std::string debugViewModeNames[2] =
	{
		"Light", "Polygon"
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

			guiData.fps = 1.0f / deltaTime;

			guiData.cpu = HardwareUsageInfo::getCPUUsage();
			guiData.ram = HardwareUsageInfo::getRAMUsage() >> 20;

			guiData.gpu = gpu->gpu;
			guiData.vram = vram->used >> 20;
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
							const auto& color = profilerSamplesColors[i];
							GraphicController::rectangleProgram->setUniformFloat2("position", left + barIndex * barWidth, bottom + yProgress * PROFILER_DRAW_HEIGHT);
							GraphicController::rectangleProgram->setUniformFloat2("scale", barWidth, PROFILER_DRAW_HEIGHT * time);
							GraphicController::rectangleProgram->setUniformFloat3("color", color.x, color.y, color.z);
							glDrawArrays(GL_TRIANGLE_FAN, 0, 4);
							yProgress += time;
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
					std::string FPS = "FPS: " + std::to_string(guiData.fps);
					std::string CPU = "CPU: " + std::to_string(guiData.cpu);
					std::string RAM = "RAM: " + std::to_string(guiData.ram);
					std::string GPU = "GPU: " + std::to_string(guiData.gpu);
					std::string VRAM = "VRAM: " + std::to_string(guiData.vram) + " mb";

					float offsetX = 0.02f;
					float offsetY = offsetX;

					float scale = 0.03f;
					float offsetYPerText = scale * 2.0f;

					float x = -1.0f * GraphicController::aspectRatio + offsetX;

					TextRenderer::renderText(FPS, x, 1.0f - offsetY, scale, glm::vec3(1.0f, 0.0f, 0.0f), AligmentX::Left, AligmentY::Top);
					TextRenderer::renderText(CPU, x, 1.0f - offsetYPerText - offsetY, scale, glm::vec3(1.0f, 0.0f, 0.0f), AligmentX::Left, AligmentY::Top);
					TextRenderer::renderText(RAM, x, 1.0f - offsetYPerText * 2.0f - offsetY, scale, glm::vec3(1.0f, 0.0f, 0.0f), AligmentX::Left, AligmentY::Top);
					TextRenderer::renderText(GPU, x, 1.0f - offsetYPerText * 3.0f - offsetY, scale, glm::vec3(1.0f, 0.0f, 0.0f), AligmentX::Left, AligmentY::Top);
					TextRenderer::renderText(VRAM, x, 1.0f - offsetYPerText * 4.0f - offsetY, scale, glm::vec3(1.0f, 0.0f, 0.0f), AligmentX::Left, AligmentY::Top);
				}
				{
					float offsetX = 0.02f;
					float offsetY = offsetX;

					float scale = 0.05f;
					float offsetYPerText = scale * 2.0f;

					float x = 1.0f * GraphicController::aspectRatio - offsetX;
					float y = 1.0f - offsetY;

					if (player->debugViewMode > 0)
					{
						TextRenderer::renderText("ViewMode: " + debugViewModeNames[player->debugViewMode - 1], x, y, scale, glm::vec3(1.0f, 0.0f, 0.0f), AligmentX::Right, AligmentY::Top);
					}
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

	// save data
	WorldData worldData;

	worldData.worldTime = world.time;

	worldData.playerPosition = player->physicEntity.position;
	worldData.playerRotation = player->rotation;

	world.saveWorldData(worldData);

	player->clean(); delete player;

	Profiler::clean();
}
