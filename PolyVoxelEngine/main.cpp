#include "Camera.h"
#include "Player.h"
#include "World.h"
#include "TerrainGenerator.h"

#include <Windows.h>
#include <dxgi.h>
#pragma comment(lib, "dxgi.lib") // Link with DXGI library
#undef max
#undef min

#include "HardwareUsageInfo.h"
#include "TextRenderer.h"

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

int main()
{
	{
		HWND hwnd = GetConsoleWindow();
		ShowWindow(hwnd, 1); // show console
	}

	if (checkMemory())
	{
		return -1;
	}

	// init
	{
		int result = GraphicController::init
		(
			1200, 800, false, 460
		);
		if (result != 0)
		{
			return result;
		}

		result = HardwareUsageInfo::init();
		if (result != 0)
		{
			GraphicController::clean();
			return result;
		}

		result = TextRenderer::init();
		if (result != 0)
		{
			HardwareUsageInfo::destroy();
			GraphicController::clean();
			return result;
		}
	}

	//
	Player player({ 0.0f, 0.0f, 0.0f}, 80.0f, 0.1f, Settings::MAX_RENDER_DISTANCE);

	World world(0);
	PhysicEntity::world = &world;
	{
		WorldData worldData = world.loadWorldData();

		auto& pos = player.physicEntity.position;
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
	}

	// Ticks
	Tick worldTick(20);
	Tick playerTick(40);
	Tick guiTick(10);

	//
	GUIData guiData;
	
	// main loop
	float previousTime = 0.0f;
	while (!GraphicController::shouldWindowClose())
	{
		float currentTime = (float)glfwGetTime();
		float deltaTime = currentTime - previousTime;
		previousTime = currentTime;

		worldTick.add(deltaTime);
		if (currentTime > 1.0f)
		{
			playerTick.add(deltaTime);
		}
		guiTick.add(deltaTime);

		while (playerTick.checkLoop())
		{
			player.physicUpdate(playerTick.delay, currentTime);
		}
		player.update(playerTick.getRatio());

		while (worldTick.checkLoop())
		{
			world.update(player.physicEntity.position, glm::length2(player.physicEntity.velocity) > 5.0f);
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

		GraphicController::beforeRender();
		{
			player.BeforeRender();

			world.draw(player.camera);
			player.draw();

			TextRenderer::beforeTextRender(GraphicController::textProgram);
			{
				std::string FPS = "FPS: " + std::to_string(guiData.fps);
				std::string CPU = "CPU: " + std::to_string(guiData.cpu);
				std::string RAM = "RAM: " + std::to_string(guiData.ram);
				std::string GPU = "GPU: " + std::to_string(guiData.gpu);
				std::string VRAM = "VRAM: " + std::to_string(guiData.vram) + " mb";

				const float offsetX = 0.02f;
				const float offsetY = offsetX;

				float scale = 0.05f;
				float offsetYPerText = scale * 2.0f;

				TextRenderer::renderText(GraphicController::textProgram, FPS, -1.0f + offsetX, 1.0f - offsetY, scale, glm::vec3(1.0f, 0.0f, 0.0f));
				TextRenderer::renderText(GraphicController::textProgram, CPU, -1.0f + offsetX, 1.0f - offsetYPerText - offsetY, scale, glm::vec3(1.0f, 0.0f, 0.0f));
				TextRenderer::renderText(GraphicController::textProgram, RAM, -1.0f + offsetX, 1.0f - offsetYPerText * 2.0f - offsetY, scale, glm::vec3(1.0f, 0.0f, 0.0f));
				TextRenderer::renderText(GraphicController::textProgram, GPU, -1.0f + offsetX, 1.0f - offsetYPerText * 3.0f - offsetY, scale, glm::vec3(1.0f, 0.0f, 0.0f));
				TextRenderer::renderText(GraphicController::textProgram, VRAM, -1.0f + offsetX, 1.0f - offsetYPerText * 4.0f - offsetY, scale, glm::vec3(1.0f, 0.0f, 0.0f));
			}
			TextRenderer::afterTextRender();
		}
		GraphicController::afterRender();
		
		glfwPollEvents();
	}

	// Save game
	{
		WorldData worldData;

		worldData.playerPosition = player.physicEntity.position;

		world.saveWorldData(worldData);
	}

	GraphicController::clean();
	HardwareUsageInfo::destroy();
	TextRenderer::destroy();
	return 0;
}