#include "TerrainGenerator.h"
#include "settings.h"
#include "Profiler.h"
#include <iostream>

SimplexNoise* TerrainGenerator::simplexNoise = nullptr;
std::unordered_map<int, HeightMap*> TerrainGenerator::heightMaps;
HeightMap** TerrainGenerator::heightMapPool = nullptr;
size_t TerrainGenerator::heightMapPoolSize = 0;
size_t TerrainGenerator::heightMapPoolIndex = 0;

constexpr int noise2ValuesRes = 1;
float* TerrainGenerator::noise2Values = nullptr;

Spline TerrainGenerator::continentalSpline = {"res/Splines/continental.bin"};

int pos2_hash(int x, int y)
{
	constexpr int shift = sizeof(int) * 8 / 2;
	constexpr int mod = (1 << shift) - 1;
	return (x & mod) | ((y & mod) << shift);
}

inline float smoothStep(float a, float b, float x)
{
	x = x * x * (3.0f - 2.0f * x);
	return a * (1.0f - x) + b * x;
}

inline float linearInterpolation(float a, float b, float x)
{
	return a * (1.0f - x) + b * x;
}

inline float biInterpolation(float a, float b, float c, float d, float x, float y, float(*interpolation)(float, float, float))
{
	return interpolation(interpolation(a, b, x), interpolation(c, d, x), y);
}

void calculatePosNegCoord(int localCoord, int chunkCoord, int& positiveCoord, int& negativeCoord, int& positiveChunkCoord, int& negativeChunkCoord)
{
	if (localCoord < Settings::CHUNK_SIZE >> 1)
	{
		positiveCoord = chunkCoord * Settings::CHUNK_SIZE + Settings::BIOME_BORDER_INTERPOLATION_SIZE;
		negativeCoord = chunkCoord * Settings::CHUNK_SIZE - Settings::BIOME_BORDER_INTERPOLATION_SIZE - 1;

		positiveChunkCoord = chunkCoord;
		negativeChunkCoord = chunkCoord - 1;
	}
	else
	{
		positiveCoord = (chunkCoord + 1) * Settings::CHUNK_SIZE + Settings::BIOME_BORDER_INTERPOLATION_SIZE;
		negativeCoord = (chunkCoord + 1) * Settings::CHUNK_SIZE - Settings::BIOME_BORDER_INTERPOLATION_SIZE - 1;

		positiveChunkCoord = chunkCoord + 1;
		negativeChunkCoord = chunkCoord;
	}
}

inline float step(float x, float steps)
{
	return floorf(x * steps) / (steps - 1.0f);
}


float TerrainGenerator::getLayeredNoise(float x, float y, int layers, float amp0, float freq0, float f_amp, float f_freq, float dx, float dy)
{
	float amp = amp0;
	float freq = freq0;

	float layered_value = 0.0f;
	for (int i = 0; i < layers; i++)
	{
		float terrain_noise = noise(x * freq + dx, y * freq + dy);
		layered_value += (terrain_noise + 1.0f) * 0.5f * amp;
		amp *= f_amp;
		freq *= f_freq;
	}
	float inv_max_sum = (1.0f - f_amp) / (1.0f - powf(f_amp, layers)); // div by amp0
	return layered_value * inv_max_sum;
}

float TerrainGenerator::getLayeredNoise3D(float x, float y, float z, int layers, float amp0, float freq0, float f_amp, float f_freq, float dx, float dy, float dz)
{
	float amp = amp0;
	float freq = freq0;

	float layered_value = 0.0f;
	for (int i = 0; i < layers; i++)
	{
		float terrain_noise = noise(x * freq + dx, y * freq + dy, z * freq + dz);
		layered_value += (terrain_noise + 1.0f) * 0.5f * amp;
		amp *= f_amp;
		freq *= f_freq;
	}
	float inv_max_sum = (1.0f - f_amp) / (1.0f - powf(f_amp, layers));
	return layered_value * inv_max_sum;
}

float TerrainGenerator::noise(float x, float y)
{
	return simplexNoise->noise(x, y);
}

float TerrainGenerator::noise(float x, float y, float z)
{
	Profiler::start(NOISE_3D_INDEX);
	float value = simplexNoise->noise(x, y, z);
	Profiler::end(NOISE_3D_INDEX);
	return value;
}

HeightMap* TerrainGenerator::getHeightMap(int chunkX, int chunkZ)
{
	const auto& it = heightMaps.find(pos2_hash(chunkX, chunkZ));
#ifdef _DEBUG
	if (it == heightMaps.end())
	{
		throw std::exception("'GetHeightMap' out of range");
	}
#endif
	return it->second;
}

Block TerrainGenerator::getBlock(int x, int y, int z, int height, Biome biome)
{
	constexpr int snowLevel = 130;
	constexpr int mountainStoneLevel = 130;
	constexpr float mountainStoneLevelVariationAmpl = 5.0f;
	constexpr float mountainStoneLevelVariationFreq = 0.1f;

	if (y > height)
	{
		return Block::Air;
	}

	if (y == height)
	{
		int level = snowLevel + sinf((x + z) * mountainStoneLevelVariationFreq) * mountainStoneLevelVariationAmpl;
		if (y > level)
		{
			return Block::Snow;
		}
		else
		{
			Block blocks[(size_t)Biome::Count] = { Block::Grass, Block::Snow, Block::Sand, Block::Grass };
			return blocks[(size_t)biome];
		}
	}
	else if (y < height - 10)
	{
		return IsCave(x, y, z) ? Block::Air : Block::Stone;
	}
	else if (y < height)
	{
		int level = mountainStoneLevel + sinf((x + z) * mountainStoneLevelVariationFreq) * mountainStoneLevelVariationAmpl;
		return y < level ? Block::Dirt : (IsCave(x, y, z) ? Block::Air : Block::Stone);
	}
	return Block::Stone;
}

bool TerrainGenerator::IsCave(int x, int y, int z)
{
	// TODO: few chunks are 'closed' because caves are too frequent
	float cheese = getLayeredNoise3D(x, y, z, 3, 1.0f, 0.02f, 0.5f, 2.0f, 0.0f, 0.0f, 0.0f);
	return cheese < 0.5f;
}

Biome TerrainGenerator::getBiome(int chunkX, int chunkZ)
{
	float frequency = 0.01f;
	float temperature = TerrainGenerator::getLayeredNoise(chunkX, chunkZ, 3, 1.0f, frequency, 0.5f, 2.0f, 0.0f, 0.0f);
	float humidity = TerrainGenerator::getLayeredNoise(chunkX, chunkZ, 3, 1.0f, frequency, 0.5f, 2.0f, 0.525f, 0.176f);
	return getBiomeByTH(temperature, humidity);
}

float TerrainGenerator::calculateInitialHeight(int globalX, int globalZ, Biome biome)
{
	int chunkX = floorf((float)globalX / (float)Settings::CHUNK_SIZE);
	int chunkZ = floorf((float)globalZ / (float)Settings::CHUNK_SIZE);

	float erosionThreshold = 0.8f;
	float erosionAmpl = 10.0f;
	float erosionFreq = 0.01f;
	float erosion = getLayeredNoise(globalX, globalZ, 1, 1.0f, erosionFreq, 0.0f, 0.0f, -0.235626f, 0.5252562f);;

	float continentalAmpl = 200.0f;
	float continentalFreq = 0.001f;
	float continental = getLayeredNoise(globalX, globalZ, 1, 1.0f, continentalFreq, 0.0f, 1.0f, 0.0f, 0.0f);
	continental += getLayeredNoise(globalX, globalZ, 2, 0.5f, continentalFreq * 2.0f, 0.5f, 2.0f, 0.0f, 0.0f);
	continental /= 1.5f;
	continental = continentalSpline.get(continental) * continentalAmpl -(erosion > erosionThreshold ? (erosion - erosionThreshold) / (1.0f - erosionThreshold) * erosionAmpl : 0.0f);

	float pvAmpl = 20.0f;
	float weirdnessFreq = 0.0025f;
	float weirdness = getLayeredNoise(globalX, globalZ, 3, 1.0f, weirdnessFreq, 0.25f, 4.0f, 0.1764f, -0.14151f);
	float pv = (1 - fabsf(3 * fabsf(weirdness) - 2)) * pvAmpl;

	return continental + pv;
}

int TerrainGenerator::calculateHeight(int globalX, int globalZ)
{
	int chunkX = floorf((float)globalX / (float)Settings::CHUNK_SIZE);
	int chunkZ = floorf((float)globalZ / (float)Settings::CHUNK_SIZE);

	Biome biome = getBiome(chunkX, chunkZ);
	return TerrainGenerator::calculateInitialHeight(globalX, globalZ, biome);

	int localX = globalX & (Settings::CHUNK_SIZE - 1);
	int localZ = globalZ & (Settings::CHUNK_SIZE - 1);

	bool interpolateX = (localX < Settings::BIOME_BORDER_INTERPOLATION_SIZE) || (localX > Settings::CHUNK_SIZE - Settings::BIOME_BORDER_INTERPOLATION_SIZE - 1);
	bool interpolateZ = (localZ < Settings::BIOME_BORDER_INTERPOLATION_SIZE) || (localZ > Settings::CHUNK_SIZE - Settings::BIOME_BORDER_INTERPOLATION_SIZE - 1);

	int negX = 0, posX = 0, negZ = 0, posZ = 0;
	int negChunkX = 0, posChunkX = 0, negChunkZ = 0, posChunkZ = 0;

	if (interpolateX)
	{
		calculatePosNegCoord(localX, chunkX, posX, negX, posChunkX, negChunkX);

		if (interpolateZ)
		{
			calculatePosNegCoord(localZ, chunkZ, posZ, negZ, posChunkZ, negChunkZ);

			float xt = (float)(globalX - negX) / (float)(posX - negX);
			float zt = (float)(globalZ - negZ) / (float)(posZ - negZ);

			Biome biome0 = getBiome(negChunkX, negChunkZ);
			Biome biome1 = getBiome(posChunkX, negChunkZ);
			Biome biome2 = getBiome(negChunkX, posChunkZ);
			Biome biome3 = getBiome(posChunkX, posChunkZ);

			if (biome0 == biome1 && biome1 == biome2 && biome2 == biome3)
			{
				return TerrainGenerator::calculateInitialHeight(globalX, globalZ, biome0);
			}
			else
			{
				float h0 = TerrainGenerator::calculateInitialHeight(negX, negZ, biome0);
				float h1 = TerrainGenerator::calculateInitialHeight(posX, negZ, biome1);
				float h2 = TerrainGenerator::calculateInitialHeight(negX, posZ, biome2);
				float h3 = TerrainGenerator::calculateInitialHeight(posX, posZ, biome3);
				return biInterpolation(h0, h1, h2, h3, xt, zt, smoothStep);
			}
		}
		else
		{
			float xt = (float)(globalX - negX) / (float)(posX - negX);

			Biome biome0 = getBiome(negChunkX, chunkZ);
			Biome biome1 = getBiome(posChunkX, chunkZ);

			if (biome0 == biome1)
			{
				return TerrainGenerator::calculateInitialHeight(globalX, globalZ, biome0);
			}
			else
			{
				float h0 = TerrainGenerator::calculateInitialHeight(negX, globalZ, biome0);
				float h1 = TerrainGenerator::calculateInitialHeight(posX, globalZ, biome1);
				return smoothStep(h0, h1, xt);
			}
		}
	}
	else
	{
		if (interpolateZ)
		{
			calculatePosNegCoord(localZ, chunkZ, posZ, negZ, posChunkZ, negChunkZ);

			float zt = (float)(globalZ - negZ) / (float)(posZ - negZ);

			Biome biome0 = getBiome(chunkX, negChunkZ);
			Biome biome1 = getBiome(chunkX, posChunkZ);

			if (biome0 == biome1)
			{
				return TerrainGenerator::calculateInitialHeight(globalX, globalZ, biome0);
			}
			else
			{
				float h0 = TerrainGenerator::calculateInitialHeight(globalX, negZ, biome0);
				float h1 = TerrainGenerator::calculateInitialHeight(globalX, posZ, biome1);
				return smoothStep(h0, h1, zt);
			}
		}
		else
		{
			Biome biome = getBiome(chunkX, chunkZ);
			return TerrainGenerator::calculateInitialHeight(globalX, globalZ, biome);
		}
	}
}

void TerrainGenerator::init(unsigned int seed)
{
	simplexNoise = new SimplexNoise(seed);

	heightMapPoolSize = calcArea(Settings::CHUNK_LOAD_RADIUS);
	heightMapPool = new HeightMap*[heightMapPoolSize];
	for (size_t i = 0; i < heightMapPoolSize; i++)
	{
		heightMapPool[i] = new HeightMap();
	}

	noise2Values = new float[noise2ValuesRes * noise2ValuesRes];
	for (int y = 0; y < noise2ValuesRes; y++)
	{
		for (int x = 0; x < noise2ValuesRes; x++)
		{
			noise2Values[x + y * noise2ValuesRes] = simplexNoise->noise((float)x / noise2ValuesRes, (float)y / noise2ValuesRes);
		}
	}
}

void TerrainGenerator::clear()
{
	delete simplexNoise;

	for (const auto& pair : heightMaps)
	{
		delete pair.second;
	}
	size_t left = heightMapPoolSize - heightMapPoolIndex;
	if (left > 0)
	{
		std::cout << "Height maps left in pool:" << left << std::endl;
		for (size_t i = heightMapPoolIndex; i < heightMapPoolSize; i++)
		{
			delete[] heightMapPool[i];
		}
	}
	delete[] heightMapPool;

	delete[] noise2Values;
}

void TerrainGenerator::loadHeightMap(int chunkX, int chunkZ)
{
	auto hash = pos2_hash(chunkX, chunkZ);
	const auto& it = heightMaps.find(hash);
	if (it != heightMaps.end())
	{
		return;
	}
#ifdef _DEBUG
	if (heightMapPoolIndex == heightMapPoolSize)
	{
		throw std::exception("Height map pool is empty");
	}
#endif
	HeightMap* heightMap = heightMapPool[heightMapPoolIndex++];
	int globalChunkX = chunkX * Settings::CHUNK_SIZE;
	int globalChunkZ = chunkZ * Settings::CHUNK_SIZE;

	for (int z = 0; z < Settings::CHUNK_SIZE; z++)
	{
		int globalZ = globalChunkZ + z;
		for (int x = 0; x < Settings::CHUNK_SIZE; x++)
		{
			int globalX = globalChunkX + x;
			heightMap->setHeightAt(x, z, calculateHeight(globalX, globalZ));
			heightMap->setSlMHAt(x, z, INT_MIN);
		}
	}
	heightMaps[hash] = heightMap;
}

void TerrainGenerator::unloadHeightMap(int chunkX, int chunkZ)
{
	auto hash = pos2_hash(chunkX, chunkZ);
	const auto& it = heightMaps.find(hash);
	if (it == heightMaps.end())
	{
		//std::cout << "height map was already unloaded" << std::endl;
		return;
	}
#ifdef _DEBUG
	if (heightMapPoolIndex == 0)
	{
		throw std::exception("Height map pool is full");
	}
#endif
	heightMapPool[--heightMapPoolIndex] = it->second;
	heightMaps.erase(it);
}

void HeightMap::setHeightAt(size_t x, size_t z, int height)
{
	heightMap[x + z * Settings::CHUNK_SIZE] = height;
}

int HeightMap::getHeightAt(size_t x, size_t z) const
{
	return heightMap[x + z * Settings::CHUNK_SIZE];
}

int HeightMap::getHeightAtByIndex(size_t index) const
{
	return heightMap[index];
}

void HeightMap::setSlMHAt(size_t x, size_t z, int height)
{
	skyLightMaxHeight[x + z * Settings::CHUNK_SIZE] = height;
}

int HeightMap::getSlMHAt(size_t x, size_t z) const
{
	return skyLightMaxHeight[x + z * Settings::CHUNK_SIZE];
}
