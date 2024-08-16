#pragma once
#include <cstdint>

enum class Biome : uint8_t
{
	Grass,
	Tundra,
	Desert,
	Tropic,
	Count
};

struct BiomePoint
{
	Biome biome;
	float temperature, humidity;
};

struct BiomeData
{
	int layersCount;
	int minHeight;
	int maxHeight;
	float amplFactor;
	float freq;
	float freqFactor;
	float erosionFreq;
	float erosionPower;
};

extern const BiomePoint biomePoints[(size_t)Biome::Count];
extern const BiomeData biomeData[(size_t)Biome::Count];

Biome getBiomeByTH(float temperature, float humidity);