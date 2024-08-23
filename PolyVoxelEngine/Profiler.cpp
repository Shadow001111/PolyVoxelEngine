#include "Profiler.h"

static std::unordered_map<std::string, ProfilerData> dataMap;

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
	data.time = duration.count();
	data.lastTimeSample = end;
}

float Profiler::get(const std::string& name)
{
	auto it = dataMap.find(name);
	if (it == dataMap.end())
	{
		return 0.0f;
	}
	ProfilerData& data = it->second;
	return data.time;
}

void Profiler::clean()
{
	dataMap.clear();
}
