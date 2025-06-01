#include "Spline.h"
#include <filesystem>
#include <iostream>
#include <fstream>

Spline::Spline()
{}

Spline::Spline(const char* filepath)
{
	if (!std::filesystem::exists(filepath))
	{
		std::cerr << "Can't load spline: " << filepath << "\n";
		return;
	}

	std::ifstream file(filepath, std::ios::binary);
	if (!file.is_open())
	{
		std::cerr << "Failed to open spline file" << filepath << "\n";
		return;
	}

	unsigned int count = 0;
	file.read(reinterpret_cast<char*>(&count), sizeof(count));
	if (count == 0)
	{
		std::cerr << "Spline points count is 0\n";
		return;
	}

	mainPoints.resize(count);
	file.read(reinterpret_cast<char*>(mainPoints.data()), count * sizeof(Point));

	helpPoints.resize(count - 1);
	file.read(reinterpret_cast<char*>(helpPoints.data()), (count - 1) * sizeof(Point));

	// check if sorted
	float lastX = 0.0f;
	for (unsigned int i = 0; i < count; i++)
	{
		const auto& main = mainPoints[i];
		if (main.x < lastX)
		{
			std::cerr << "Spline: '" << filepath << "', is not ordered right\n";
			return;
		}
		lastX = main.x;
		if (i < count - 1)
		{
			const auto& help = helpPoints[i];
			if (help.x < lastX)
			{
				std::cerr << "Spline: '" << filepath << "', is not ordered right\n";
				return;
			}
			lastX = help.x;
		}
	}
}

float Spline::get(float x) const
{
	const auto& first = mainPoints.front();
	if (x < first.x)
	{
		return first.y;
	}

	const auto& last = mainPoints.back();
	if (x > last.x)
	{
		return last.y;
	}

	auto comparator = [](const Point& point, float x) {
		return point.x < x;
	};

	auto it = std::lower_bound(mainPoints.begin(), mainPoints.end(), x, comparator);

	const auto& right = it;
	const auto& left = std::prev(it);
	int index = std::distance(mainPoints.begin(), left);

	float t = (x - left->x) / (right->x - left->x);
	float t2 = 1.0f - t;

	return
		t2 * t2 * left->y +
		2.0f * t2 * t * helpPoints[index].y +
		t * t * right->y;
}
