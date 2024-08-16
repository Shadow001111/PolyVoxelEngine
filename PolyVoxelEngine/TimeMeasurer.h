#pragma once
#include <chrono>

class TimeMeasurer
{
	std::chrono::steady_clock::time_point start;
public:
	TimeMeasurer();
	void stop(const char* text);
};

