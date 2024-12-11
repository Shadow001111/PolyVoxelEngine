#include "Profiler.h"
#include <iostream>

std::unordered_map<std::thread::id, Profiler::PerThreadData> Profiler::threadProfilerData;
uint32_t Profiler::memoryTable[PROFILER_MEMORY_TABLE_SIZE][PROFILER_SAMPLES_COUNT] = {};
size_t Profiler::memoryTableIndex = 0;
uint32_t Profiler::maxTimeTable[PROFILER_MEMORY_TABLE_SIZE] = {};
uint32_t Profiler::maxTime = 0;
std::mutex Profiler::addDataMutex;

const std::string profilerSamplesNames[PROFILER_SAMPLES_COUNT] =
{
	"BlockGeneration",
	"BlockLightUpdate",
	"SkyLightUpdate",
	"MeshGeneration",
	"LoadChunks",
	"ChunkLoadData",
	"ChunkLighting",
	"GreedyMeshing"
};

const glm::vec3 profilerSamplesColors[PROFILER_SAMPLES_COUNT] =
{
	{1.0f, 0.0f, 0.0f},
	{0.0f, 1.0f, 0.0f},
	{0.0f, 0.0f, 1.0f},

	{1.0f, 1.0f, 0.0f},
	{0.0f, 1.0f, 1.0f},
	{1.0f, 0.0f, 1.0f},

	{0.5f, 0.5f, 0.5f},
	{1.0f, 1.0f, 1.0f}
};

void Profiler::start(size_t index)
{
	std::lock_guard<std::mutex> lock(addDataMutex);
	threadProfilerData[std::this_thread::get_id()].profilerData[index].lastTimeSample = std::chrono::steady_clock::now();
}

void Profiler::end(size_t index)
{
	auto end = std::chrono::steady_clock::now();
	if (index >= PROFILER_SAMPLES_COUNT)
	{
		return;
	}
	std::lock_guard<std::mutex> lock(addDataMutex);
	ProfilerData& data = threadProfilerData[std::this_thread::get_id()].profilerData[index];
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
	std::lock_guard<std::mutex> lock(addDataMutex);
	for (auto& it : threadProfilerData)
	{
		for (size_t i = 0; i < PROFILER_SAMPLES_COUNT; i++)
		{
			it.second.profilerData[i].timeNS = 0.0f;
		}
	}
}

void Profiler::clean()
{
	
}

void Profiler::saveToMemory()
{
	std::lock_guard<std::mutex> lock(addDataMutex);
	uint32_t timeSum = 0;

	for (size_t i = 0; i < PROFILER_SAMPLES_COUNT; i++)
	{
		memoryTable[memoryTableIndex][i] = 0;
	}
	for (auto& it : threadProfilerData)
	{
		PerThreadData& perThreadData = it.second;
		for (size_t i = 0; i < PROFILER_SAMPLES_COUNT; i++)
		{
			ProfilerData& data = perThreadData.profilerData[i];
			timeSum += data.timeNS;
			memoryTable[memoryTableIndex][i] += data.timeNS;
			data.timeNS = 0;
		}
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
