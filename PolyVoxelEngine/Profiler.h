#pragma once
#include <chrono>
#include <unordered_map>

struct ProfilerData
{
	std::chrono::steady_clock::time_point lastTimeSample;
	float time = 0.0f;
};

class Profiler
{
	static std::unordered_map<std::string, ProfilerData> dataMap;
public:
	static void start(const std::string& name);
	static void end(const std::string& name);
	static float get(const std::string& name);
	static void clean();
};

