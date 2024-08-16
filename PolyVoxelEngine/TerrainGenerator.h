#pragma once
#include "Block.h"
#include "Biome.h"
#include "settings.h"
#include <unordered_map>
#include "SimplexNoise.h"

struct HeightMap
{
	int heightMap[Settings::CHUNK_SIZE_SQUARED];
	int skyLightMaxHeight[Settings::CHUNK_SIZE_SQUARED];

	void setHeightAt(size_t x, size_t z, int height);
	int getHeightAt(size_t x, size_t z) const;

	void setSlMHAt(size_t x, size_t z, int height);
	int getSlMHAt(size_t x, size_t z) const;
};

class TerrainGenerator
{
	static SimplexNoise* simplexNoise;
	static std::unordered_map<int, HeightMap*> heightMaps;
	static HeightMap** heightMapPool;
	static size_t heightMapPoolSize;
	static size_t heightMapPoolIndex;

	static float* noise2Values;

	static float calculateInitialHeight(int globalX, int globalZ, Biome biome);
public:
	static void init(unsigned int seed);
	static void clear();

	static int calculateHeight(int globalX, int globalZ);

	static void loadHeightMap(int chunkX, int chunkZ);
	static void unloadHeightMap(int chunkX, int chunkZ);

	static float getLayeredNoise(float x, float y, int layers, float amp0, float freq0, float f_amp, float f_freq, float dx, float dy);
	static float getLayeredNoise3D(float x, float y, float z, int layers, float amp0, float freq0, float f_amp, float f_freq, float dx, float dy, float dz);
	static float noise(float x, float y);
	static float noise(float x, float y, float z);
	static HeightMap* getHeightMap(int chunkX, int chunkZ);

	static Block getBlock(int x, int y, int z, int height, Biome biome);
	static bool IsCave(int x, int y, int z);

	static Biome getBiome(int chunkX, int chunkZ);
};
