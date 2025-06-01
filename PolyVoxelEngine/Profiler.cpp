#include "Profiler.h"
#include <iostream>

std::unordered_map<std::thread::id, Profiler::ThreadData> Profiler::profilerThreadData;
uint32_t Profiler::systemTimeSamplesMemoryTable[PROFILER_CATEGORIES_COUNT][PROFILER_MEMORY_TABLE_SIZE] = {};
size_t Profiler::memoryTableIndex = 0;

const std::string profilerSamplesNames[PROFILER_CATEGORIES_COUNT] =
{
	"BlockGeneration",
	"BlockLightUpdate",
	"SkyLightUpdate",
	"FetchingFaces",
	"GreedyMeshing",
	"LoadChunks",
	"UnloadChunks",
	"ChunkLoadData",
	"ChunkLighting",
	"Test1",
	"Test2",
	"Test3",
	"Test4",
	"Test5",
};

void Profiler::start(size_t index)
{
	profilerThreadData[std::this_thread::get_id()].samples[index].lastTimeSample = std::chrono::steady_clock::now();
}

void Profiler::end(size_t index)
{
	auto end = std::chrono::steady_clock::now();
	if (index >= PROFILER_CATEGORIES_COUNT)
	{
		return;
	}
	ThreadData& threadData = profilerThreadData[std::this_thread::get_id()];
	SystemSampleData& data = threadData.samples[index];

	auto duration = std::chrono::duration_cast<std::chrono::nanoseconds>(end - data.lastTimeSample);
	auto durationInt = duration.count();

	data.timeNS += durationInt;
	data.lastTimeSample = end;
	threadData.sumTimeNS += durationInt;
}

void Profiler::reset(size_t index)
{
	if (index >= PROFILER_CATEGORIES_COUNT)
	{
		return;
	}
	for (auto& it : profilerThreadData)
	{
		Profiler::ThreadData& threadData = it.second;
		for (size_t i = 0; i < PROFILER_CATEGORIES_COUNT; i++)
		{
			threadData.samples[i].timeNS = 0.0f;
		}
		threadData.sumTimeNS = 0.0f;
	}
}

void Profiler::clean()
{
	
}

void Profiler::saveToMemory()
{
	for (size_t i = 0; i < PROFILER_CATEGORIES_COUNT; i++)
	{
		systemTimeSamplesMemoryTable[i][memoryTableIndex] = 0;
	}

	for (auto& it : profilerThreadData)
	{
		ThreadData& threadData = it.second;
		for (size_t i = 0; i < PROFILER_CATEGORIES_COUNT; i++)
		{
			SystemSampleData& data = threadData.samples[i];
			systemTimeSamplesMemoryTable[i][memoryTableIndex] += data.timeNS;
			data.timeNS = 0;
		}
		threadData.timeSamplesMemoryTable[memoryTableIndex] = threadData.sumTimeNS;
		threadData.sumTimeNS = 0;
	}

	memoryTableIndex++;
	if (memoryTableIndex >= PROFILER_MEMORY_TABLE_SIZE)
	{
		memoryTableIndex = 0;
	}
}
