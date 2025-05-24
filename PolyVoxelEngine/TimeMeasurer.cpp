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
	std::cout << text << ": " << duration.count() * 1000.0 << " ms\n";
}
