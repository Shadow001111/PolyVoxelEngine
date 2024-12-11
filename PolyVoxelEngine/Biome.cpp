#include "Biome.h"

const BiomePoint biomePoints[(size_t)Biome::Count] =
{
	{Biome::Grass, 0.5f, 0.5f},
	{Biome::Tundra, 0.2f, 0.8f},
	{Biome::Desert, 0.8f, 0.2f},
	{Biome::Tropic, 0.8f, 0.8f}
};

const BiomeData biomeData[(size_t)Biome::Count] = 
{
	{200.0f,{3, 1.0f, 0.001f, 0.5f, 2.0f}, 10.0f, 0.8f, {1, 1.0f, 0.01f, 1.0f, 1.0f}, 20.0f, {3, 1.0f, 0.0025f, 0.25f, 4.0f}}, // Grass
	{200.0f,{3, 1.0f, 0.001f, 0.5f, 2.0f}, 10.0f, 0.8f, {1, 1.0f, 0.01f, 1.0f, 1.0f}, 20.0f, {3, 1.0f, 0.0025f, 0.25f, 4.0f}}, // Tundra
	{200.0f,{3, 1.0f, 0.001f, 0.5f, 2.0f}, 10.0f, 0.8f, {1, 1.0f, 0.01f, 1.0f, 1.0f}, 20.0f, {3, 1.0f, 0.0025f, 0.25f, 4.0f}}, // Desert
	{200.0f,{3, 1.0f, 0.001f, 0.5f, 2.0f}, 10.0f, 0.8f, {1, 1.0f, 0.01f, 1.0f, 1.0f}, 20.0f, {3, 1.0f, 0.0025f, 0.25f, 4.0f}}, // Tropic
};

Biome getBiomeByTH(float temperature, float humidity)
{
	Biome closest = Biome::Grass;
	float minDistance2 = 9999999999999999.0f;
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
