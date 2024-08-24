#pragma once
#include <chrono>
#include <unordered_map>
#include <cstdint>
#include <glm/glm.hpp>

struct ProfilerData
{
	std::chrono::steady_clock::time_point lastTimeSample;
	uint16_t time = 0;
	uint16_t samplesTaken = 0;
};

constexpr size_t PROFILER_MEMORY_TABLE_SIZE = 20;
constexpr size_t PROFILER_SAMPLES_COUNT = 4;

constexpr float PROFILER_DRAW_WIDTH = 0.5f;
constexpr float PROFILER_DRAW_HEIGHT = 0.5f;
constexpr float PROFILER_DRAW_COLOR_RECT_SIZE = 0.05f;
constexpr float PROFILER_DRAW_COLOR_RECT_Y_OFFSET = 0.02f;

extern const std::string profilerSamplesNames[PROFILER_SAMPLES_COUNT];
extern const glm::vec3 profilerSamplesColors[PROFILER_SAMPLES_COUNT];

class Profiler
{
	static std::unordered_map<std::string, ProfilerData> dataMap;
public:
	static uint16_t memoryTable[PROFILER_MEMORY_TABLE_SIZE][PROFILER_SAMPLES_COUNT];
	static size_t memoryTableIndex;
	static uint16_t maxTimeTable[PROFILER_MEMORY_TABLE_SIZE];
	static uint16_t maxTime;

	static void start(const std::string& name);
	static void end(const std::string& name);
	static void reset(const std::string& name);
	static void clean();

	static void saveToMemory();
};

