#pragma once

enum class Biome : char
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

struct LayeredNoiseData
{
	// TODO: add offset
	int layersCount = 3;
	float amplitude = 1.0f;
	float frequency = 1.0f;
	float amplitudeFactor = 1.0f;
	float frequencyFactor = 1.0f;
};

struct BiomeData
{
	float continentalAmplitude = 1.0f;
	LayeredNoiseData continentalLayer;
	float erosionAmplitude = 1.0f;
	float erosionThreshold = 0.0f;
	LayeredNoiseData erosionLayer;
	float weirdnessAmplitude = 1.0f;
	LayeredNoiseData weirdnessLayer;
};

extern const BiomePoint biomePoints[(size_t)Biome::Count];
extern const BiomeData biomeData[(size_t)Biome::Count];

Biome getBiomeByTH(float temperature, float humidity);