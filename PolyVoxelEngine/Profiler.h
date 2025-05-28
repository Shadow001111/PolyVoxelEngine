#pragma once
#include <chrono>
#include <cstdint>
#include <glm/ext/vector_float3.hpp>
#include <thread>
#include <unordered_map>

constexpr size_t PROFILER_MEMORY_TABLE_SIZE = 50;
constexpr size_t PROFILER_CATEGORIES_COUNT = 14;

constexpr size_t BLOCK_GENERATION_INDEX = 0;
constexpr size_t BLOCK_LIGHT_UPDATE_INDEX = 1;
constexpr size_t SKY_LIGHT_UPDATE_INDEX = 2;
constexpr size_t FACE_FETCHING_INDEX = 3;
constexpr size_t GREEDY_MESHING_INDEX = 4;
constexpr size_t LOAD_CHUNKS_INDEX = 5;
constexpr size_t UNLOAD_CHUNKS_INDEX = 6;
constexpr size_t CHUNK_LOAD_DATA_INDEX = 7;
constexpr size_t CHUNK_LIGHTING_INDEX = 8;
constexpr size_t TEST1_INDEX = 9;
constexpr size_t TEST2_INDEX = 10;
constexpr size_t TEST3_INDEX = 11;
constexpr size_t TEST4_INDEX = 12;
constexpr size_t TEST5_INDEX = 13;


constexpr float PROFILER_DRAW_WIDTH = 0.5f;
constexpr float PROFILER_DRAW_HEIGHT = 0.5f;
constexpr float PROFILER_DRAW_COLOR_RECT_SIZE = 0.05f;
constexpr float PROFILER_DRAW_COLOR_RECT_Y_OFFSET = 0.02f;

extern const std::string profilerSamplesNames[PROFILER_CATEGORIES_COUNT];
extern const glm::vec3 profilerSamplesColors[PROFILER_CATEGORIES_COUNT];

class Profiler
{
	struct ProfilerData
	{
		std::chrono::steady_clock::time_point lastTimeSample;
		uint32_t timeNS = 0;
	};

	struct PerThreadData
	{
		ProfilerData profilerData[PROFILER_CATEGORIES_COUNT];
	};

	static std::unordered_map<std::thread::id, PerThreadData> threadProfilerData;
public:
	static uint32_t memoryTable[PROFILER_CATEGORIES_COUNT][PROFILER_MEMORY_TABLE_SIZE];
	static size_t memoryTableIndex;

	static void start(size_t index);
	static void end(size_t index);
	static void reset(size_t index);
	static void clean();

	static void saveToMemory();
};

