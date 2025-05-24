#include "TimeMeasurer.h"
#include <iostream>

TimeMeasurer::TimeMeasurer()
{
	start = std::chrono::high_resolution_clock::now();
}

void TimeMeasurer::stop(const char* text)
{
	auto end = std::chrono::high_resolution_clock::now();
	std::chrono::duration<double> duration = end - start;
	std::cout << text << ": " << (float)duration.count() * 1000.0f << " ms\n";
}
