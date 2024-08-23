#pragma once
#include <chrono>
#include <unordered_map>
#include <cstdint>

struct ProfilerData
{
	std::chrono::steady_clock::time_point lastTimeSample;
	uint16_t time = 0;
};

class Profiler
{
	static std::unordered_map<std::string, ProfilerData> dataMap;
public:
	static void start(const std::string& name);
	static void end(const std::string& name);
	static uint16_t get(const std::string& name);
	static void clean();
};

