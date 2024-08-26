#pragma once
#include <chrono>
#include <unordered_map>
#include <cstdint>
#include <glm/glm.hpp>

struct ProfilerData
{
	std::chrono::steady_clock::time_point lastTimeSample;
	uint32_t timeNS = 0;
};

constexpr size_t PROFILER_MEMORY_TABLE_SIZE = 50;
constexpr size_t PROFILER_SAMPLES_COUNT = 5;

#define BLOCK_GENERATION_INDEX 0
#define BLOCK_LIGHT_UPDATE_INDEX 1
#define SKY_LIGHT_UPDATE_INDEX 2
#define MESH_GENERATION_INDEX 3
#define LOAD_CHUNKS_INDEX 4

constexpr float PROFILER_DRAW_WIDTH = 0.5f;
constexpr float PROFILER_DRAW_HEIGHT = 0.5f;
constexpr float PROFILER_DRAW_COLOR_RECT_SIZE = 0.05f;
constexpr float PROFILER_DRAW_COLOR_RECT_Y_OFFSET = 0.02f;

extern const std::string profilerSamplesNames[PROFILER_SAMPLES_COUNT];
extern const glm::vec3 profilerSamplesColors[PROFILER_SAMPLES_COUNT];

class Profiler
{
	static ProfilerData profilerData[PROFILER_SAMPLES_COUNT];
public:
	static uint16_t memoryTable[PROFILER_MEMORY_TABLE_SIZE][PROFILER_SAMPLES_COUNT];
	static size_t memoryTableIndex;
	static uint16_t maxTimeTable[PROFILER_MEMORY_TABLE_SIZE];
	static uint16_t maxTime;

	static void start(size_t index);
	static void end(size_t index);
	static void reset(size_t index);
	static void clean();

	static void saveToMemory();
};

