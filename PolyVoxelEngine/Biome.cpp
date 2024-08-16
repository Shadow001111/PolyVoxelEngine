#include "Biome.h"
#include <float.h>

const BiomePoint biomePoints[(size_t)Biome::Count] =
{
	{Biome::Grass, 0.5f, 0.5f},
	{Biome::Tundra, 0.2f, 0.8f},
	{Biome::Desert, 0.8f, 0.2f},
	{Biome::Tropic, 0.8f, 0.8f}
};

const BiomeData biomeData[(size_t)Biome::Count] = 
{
	{3, 70, 110, 0.5f, 0.005f, 2.0f, 0.01f, 0.95f}, // Grass
	{3, 60, 100, 0.5f, 0.005f, 2.0f, 0.01f, 0.95f}, // Tundra
	{3, 60, 100, 0.5f, 0.01f, 2.0f, 0.01f, 0.95f}, // Desert
	{3, 60, 100, 0.5f, 0.01f, 2.0f, 0.01f, 0.95f}, // Tropic
};

Biome getBiomeByTH(float temperature, float humidity)
{
	Biome closest = Biome::Grass;
	float minDistance2 = FLT_MAX;
	for (const auto& point : biomePoints)
	{
		float dt = temperature - point.temperature;
		float dh = humidity - point.humidity;
		float distance2 = dt * dt + dh * dh;
		if (distance2 < minDistance2)
		{
			minDistance2 = distance2;
			closest = point.biome;
		}
	}
	return closest;
}
