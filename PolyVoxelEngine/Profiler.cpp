#include "Profiler.h"
#include <iostream>

ProfilerData Profiler::profilerData[PROFILER_SAMPLES_COUNT];
uint16_t Profiler::memoryTable[PROFILER_MEMORY_TABLE_SIZE][PROFILER_SAMPLES_COUNT] = {};
size_t Profiler::memoryTableIndex = 0;
uint16_t Profiler::maxTimeTable[PROFILER_MEMORY_TABLE_SIZE] = {};
uint16_t Profiler::maxTime = 0;

const std::string profilerSamplesNames[PROFILER_SAMPLES_COUNT] =
{
	"BlockGeneration",
	"BlockLightUpdate",
	"SkyLightUpdate",
	"MeshGeneration",
	"LoadChunks",
	"ChunkLoadData",
	"ChunkLighting"
};

const glm::vec3 profilerSamplesColors[PROFILER_SAMPLES_COUNT] =
{
	{1.0f, 0.0f, 0.0f},
	{0.0f, 1.0f, 0.0f},
	{0.0f, 0.0f, 1.0f},
	{1.0f, 1.0f, 0.0f},
	{0.0f, 1.0f, 1.0f},
	{1.0f, 0.0f, 1.0f},
	{0.5f, 0.5f, 0.5f}
};

void Profiler::start(size_t index)
{
	profilerData[index].lastTimeSample = std::chrono::steady_clock::now();
}

void Profiler::end(size_t index)
{
	auto end = std::chrono::steady_clock::now();
	if (index >= PROFILER_SAMPLES_COUNT)
	{
		return;
	}
	ProfilerData& data = profilerData[index];
	auto duration = std::chrono::duration_cast<std::chrono::nanoseconds>(end - data.lastTimeSample);
	data.timeNS += duration.count();
	data.lastTimeSample = end;
}

void Profiler::reset(size_t index)
{
	if (index >= PROFILER_SAMPLES_COUNT)
	{
		return;
	}
	profilerData[index].timeNS = 0.0f;
}

void Profiler::clean()
{
	
}

void Profiler::saveToMemory()
{
	uint16_t timeSum = 0;
	for (size_t i = 0; i < PROFILER_SAMPLES_COUNT; i++)
	{
		ProfilerData& data = profilerData[i];
		uint16_t time = data.timeNS / 1000000;
		data.timeNS = 0;
		memoryTable[memoryTableIndex][i] = time;
		timeSum += time;
	}
	maxTimeTable[memoryTableIndex] = timeSum;

	maxTime = 0;
	for (size_t i = 0; i < PROFILER_MEMORY_TABLE_SIZE; i++)
	{
		maxTime = std::max(maxTime, maxTimeTable[i]);
	}

	memoryTableIndex++;
	if (memoryTableIndex >= PROFILER_MEMORY_TABLE_SIZE)
	{
		memoryTableIndex = 0;
	}
}
