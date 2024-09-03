#pragma once
#include <vector>

class Spline
{
	struct Point
	{
		float x, y;
	};

	std::vector<Point> mainPoints;
	std::vector<Point> helpPoints;
public:
	Spline();
	Spline(const char* filepath);

	float get(float x) const;
};

