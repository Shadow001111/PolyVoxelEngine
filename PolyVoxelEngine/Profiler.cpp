#include "Profiler.h"
#include <iostream>

std::unordered_map<std::string, ProfilerData> Profiler::dataMap;
uint16_t Profiler::memoryTable[PROFILER_MEMORY_TABLE_SIZE][PROFILER_SAMPLES_COUNT] = {};
size_t Profiler::memoryTableIndex = 0;
uint16_t Profiler::maxTimeTable[PROFILER_MEMORY_TABLE_SIZE] = {};
uint16_t Profiler::maxTime = 0;

const std::string profilerSamplesNames[PROFILER_SAMPLES_COUNT] =
{
	"BlockGeneration",
	"LightUpdate",
	"MeshGeneration",
	"LoadChunks",
};

const glm::vec3 profilerSamplesColors[PROFILER_SAMPLES_COUNT] =
{
	{1.0f, 0.0f, 0.0f},
	{0.0f, 1.0f, 0.0f},
	{0.0f, 0.0f, 1.0f},
	{1.0f, 1.0f, 0.0f},
};

void Profiler::start(const std::string& name)
{
	auto start = std::chrono::steady_clock::now();
	dataMap[name].lastTimeSample = start;
}

void Profiler::end(const std::string& name)
{
	auto it = dataMap.find(name);
	if (it == dataMap.end())
	{
		return;
	}
	ProfilerData& data = it->second;
	auto end = std::chrono::steady_clock::now();
	auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - data.lastTimeSample);
	data.time += duration.count();
	data.samplesTaken++;
	data.lastTimeSample = end;
}

void Profiler::reset(const std::string& name)
{
	auto it = dataMap.find(name);
	if (it == dataMap.end())
	{
		return;
	}
	it->second.time = 0.0f;
}

void Profiler::clean()
{
	dataMap.clear();
}

void Profiler::saveToMemory()
{
	uint16_t timeSum = 0;
	for (size_t i = 0; i < PROFILER_SAMPLES_COUNT; i++)
	{
		auto it = dataMap.find(profilerSamplesNames[i]);
		if (it == dataMap.end())
		{
			continue;
		}
		uint16_t time = it->second.time / it->second.samplesTaken;
		it->second.time = 0;
		it->second.samplesTaken = 0;
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
