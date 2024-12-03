#pragma once
#include <chrono>
#include <cstdint>
#include <glm/ext/vector_float3.hpp>
#include <mutex>
#include <unordered_map>

struct ProfilerData
{
	std::chrono::steady_clock::time_point lastTimeSample;
	uint32_t timeMS = 0;
};

constexpr size_t PROFILER_MEMORY_TABLE_SIZE = 50;
constexpr size_t PROFILER_SAMPLES_COUNT = 8;

constexpr size_t BLOCK_GENERATION_INDEX = 0;
constexpr size_t BLOCK_LIGHT_UPDATE_INDEX = 1;
constexpr size_t SKY_LIGHT_UPDATE_INDEX = 2;
constexpr size_t MESH_GENERATION_INDEX = 3;
constexpr size_t LOAD_CHUNKS_INDEX = 4;
constexpr size_t CHUNK_LOAD_DATA_INDEX = 5;
constexpr size_t CHUNK_LIGHTING_INDEX = 6;
constexpr size_t NOISE_3D_INDEX = 7;

constexpr float PROFILER_DRAW_WIDTH = 0.5f;
constexpr float PROFILER_DRAW_HEIGHT = 0.5f;
constexpr float PROFILER_DRAW_COLOR_RECT_SIZE = 0.05f;
constexpr float PROFILER_DRAW_COLOR_RECT_Y_OFFSET = 0.02f;

extern const std::string profilerSamplesNames[PROFILER_SAMPLES_COUNT];
extern const glm::vec3 profilerSamplesColors[PROFILER_SAMPLES_COUNT];

class Profiler
{
	struct PerThreadData
	{
		ProfilerData profilerData[PROFILER_SAMPLES_COUNT];
	};

	static std::unordered_map<std::thread::id, PerThreadData> threadProfilerData;
public:
	static uint32_t memoryTable[PROFILER_MEMORY_TABLE_SIZE][PROFILER_SAMPLES_COUNT];
	static size_t memoryTableIndex;
	static uint32_t maxTimeTable[PROFILER_MEMORY_TABLE_SIZE];
	static uint32_t maxTime;
	static std::mutex addDataMutex;

	static void start(size_t index);
	static void end(size_t index);
	static void reset(size_t index);
	static void clean();

	static void saveToMemory();
};

