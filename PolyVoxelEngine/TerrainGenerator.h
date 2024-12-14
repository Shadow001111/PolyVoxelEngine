#pragma once
#include "Block.h"
#include "Biome.h"
#include "settings.h"
#include <unordered_map>
#include "FastNoise/FastNoise.h"
#include "Spline.h"
#include "AllocatedObjectPool.h"
#include <atomic>

class ChunkColumnData
{
	friend class TerrainGenerator;

	int heightMap[Settings::CHUNK_SIZE_SQUARED];
	int skyLightMaxHeight[Settings::CHUNK_SIZE_SQUARED];
	Biome biome;
	std::atomic<uint32_t> usedBy;
public:
	ChunkColumnData();

	void setHeightAt(size_t x, size_t z, int height);
	int getHeightAt(size_t x, size_t z) const;
	int getHeightAtByIndex(size_t index) const;

	void setSlMHAt(size_t x, size_t z, int height);
	int getSlMHAt(size_t x, size_t z) const;

	Biome getBiome() const;

	void startUsing();
	void stopUsing();
};

class TerrainGenerator
{
	static FastNoise::SmartNode<FastNoise::Simplex> simplexNoise;
	static std::unordered_map<int, ChunkColumnData*> heightMaps;
	static AllocatedObjectPool<ChunkColumnData> heightMapPool;

	static Spline continentalSpline;

	thread_local static float chunkCaveNoiseArray[Settings::CHUNK_SIZE_CUBED];
	static float chunkNoiseCalculationsArray2D[Settings::CHUNK_SIZE_SQUARED];
public:
	static int seed;

	static void init(unsigned int seed);
	static void clear();

	static int calculateHeight(int globalX, int globalZ);

	static void loadHeightMap(int chunkX, int chunkZ);
	static void unloadHeightMap(int chunkX, int chunkZ);

	static float noise(float x, float y);
	static float noise(float x, float y, float z);
	static float getLayeredNoise2D(float x, float y, int layers, float amp0, float freq0, float f_amp, float f_freq, float dx, float dy);
	static float getLayeredNoise3D(float x, float y, float z, int layers, float amp0, float freq0, float f_amp, float f_freq, float dx, float dy, float dz);
	static void getNoiseArray2D(float* array, float x, float y, int sizeX, int sizeY, float frequency);
	static void getNoiseArray3D(float* array, float x, float y, float z, int sizeX, int sizeY, int sizeZ, float frequency);
	static void getLayeredNoiseArray2D(float* array, float x, float y, int sizeX, int sizeY, float amplitude, float frequency, int layers, float amplitudeFactor, float frequencyFactor);
	static void getLayeredNoiseArray2D(float* array, float x, float y, int sizeX, int sizeY, const LayeredNoiseData& data);
	static void getInitialHeightArray(int* heightArray, int chunkX, int chunkZ, Biome biome);
	static void generateChunkCaveNoise(int chunkX, int chunkY, int chunkZ);
	static ChunkColumnData* getHeightMap(int chunkX, int chunkZ);

	static Block getBlock(int x, int y, int z, int height, Biome biome);
	static bool IsCaveInChunk(int x, int y, int z);

	static Biome getBiome(int chunkX, int chunkZ);
};
