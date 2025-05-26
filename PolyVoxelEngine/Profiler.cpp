#include "Profiler.h"

std::unordered_map<std::thread::id, Profiler::PerThreadData> Profiler::threadProfilerData;
uint32_t Profiler::memoryTable[PROFILER_CATEGORIES_COUNT][PROFILER_MEMORY_TABLE_SIZE] = {};
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
	"Test"
};

void Profiler::start(size_t index)
{
	threadProfilerData[std::this_thread::get_id()].profilerData[index].lastTimeSample = std::chrono::steady_clock::now();
}

void Profiler::end(size_t index)
{
	auto end = std::chrono::steady_clock::now();
	if (index >= PROFILER_CATEGORIES_COUNT)
	{
		return;
	}
	ProfilerData& data = threadProfilerData[std::this_thread::get_id()].profilerData[index];
	auto duration = std::chrono::duration_cast<std::chrono::nanoseconds>(end - data.lastTimeSample);
	data.timeNS += duration.count();
	data.lastTimeSample = end;
}

void Profiler::reset(size_t index)
{
	if (index >= PROFILER_CATEGORIES_COUNT)
	{
		return;
	}
	for (auto& it : threadProfilerData)
	{
		for (size_t i = 0; i < PROFILER_CATEGORIES_COUNT; i++)
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
	uint32_t timeSum = 0;

	for (size_t i = 0; i < PROFILER_CATEGORIES_COUNT; i++)
	{
		memoryTable[i][memoryTableIndex] = 0;
	}
	for (auto& it : threadProfilerData)
	{
		PerThreadData& perThreadData = it.second;
		for (size_t i = 0; i < PROFILER_CATEGORIES_COUNT; i++)
		{
			ProfilerData& data = perThreadData.profilerData[i];
			timeSum += data.timeNS;
			memoryTable[i][memoryTableIndex] += data.timeNS;
			data.timeNS = 0;
		}
	}

	memoryTableIndex++;
	if (memoryTableIndex >= PROFILER_MEMORY_TABLE_SIZE)
	{
		memoryTableIndex = 0;
	}
}
