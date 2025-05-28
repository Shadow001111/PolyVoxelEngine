#include "TerrainGenerator.h"
#include "settings.h"
#include "Profiler.h"
#include <iostream>
#include <filesystem>
#include <fstream>

int TerrainGenerator::seed = 0;
FastNoise::SmartNode<FastNoise::Simplex> TerrainGenerator::simplexNoise;
std::unordered_map<int, ChunkColumnData*> TerrainGenerator::heightMaps;
AllocatedObjectPool<ChunkColumnData> TerrainGenerator::heightMapPool(0);

Spline TerrainGenerator::continentalSpline = {"res/Splines/continental.bin"};

thread_local float TerrainGenerator::chunkCaveNoiseArray[Settings::CHUNK_SIZE_CUBED];
float TerrainGenerator::chunkNoiseCalculationsArray2D[Settings::CHUNK_SIZE_SQUARED];


int pos2_hash(int x, int y)
{
	constexpr int shift = sizeof(int) * 8 / 2; // sizeof(return_type) * bits_count / 2
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

float TerrainGenerator::noise(float x, float y)
{
	return simplexNoise->GenSingle2D(x, y, seed);
}

float TerrainGenerator::noise(float x, float y, float z)
{
	return simplexNoise->GenSingle3D(x, y, z, seed);
}

float TerrainGenerator::getLayeredNoise2D(float x, float y, int layers, float amp0, float freq0, float f_amp, float f_freq)
{
	float amp = amp0;
	float freq = freq0;

	float layered_value = 0.0f;
	for (int i = 0; i < layers; i++)
	{
		float terrain_noise = noise(x * freq, y * freq);
		layered_value += (terrain_noise + 1.0f) * 0.5f * amp;
		amp *= f_amp;
		freq *= f_freq;
	}
	float inv_max_sum = (1.0f - f_amp) / (1.0f - powf(f_amp, layers)); // div by amp0
	return layered_value * inv_max_sum;
}

float TerrainGenerator::getLayeredNoise2D(float x, float y, const LayeredNoiseData& data)
{
	return getLayeredNoise2D(x, y, data.layersCount, data.amplitude, data.frequency, data.amplitudeFactor, data.frequencyFactor);
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

void TerrainGenerator::getNoiseArray2D(float* array, float x, float y, int sizeX, int sizeY, float frequency)
{
	simplexNoise->GenUniformGrid2D(array, x, y, sizeX, sizeY, frequency, seed);
}

void TerrainGenerator::getNoiseArray3D(float* array, float x, float y, float z, int sizeX, int sizeY, int sizeZ, float frequency)
{
	simplexNoise->GenUniformGrid3D(array, x, y, z, sizeX, sizeY, sizeZ, frequency, seed);
}

void TerrainGenerator::getLayeredNoiseArray2D(float* array, float x, float y, int sizeX, int sizeY, float amplitude, float frequency, int layers, float amplitudeFactor, float frequencyFactor)
{
	for (int y = 0; y < sizeY; y++)
	{
		for (int x = 0; x < sizeX; x++)
		{
			size_t index = x + y * sizeX;
			array[index] = 0;
		}
	}
	for (int i = 0; i < layers; i++)
	{
		getNoiseArray2D(chunkNoiseCalculationsArray2D, x, y, sizeX, sizeY, frequency);
		for (int y = 0; y < sizeY; y++)
		{
			for (int x = 0; x < sizeX; x++)
			{
				size_t index = x + y * sizeX;
				float value = chunkNoiseCalculationsArray2D[index];
				value = (value + 1.0f) * 0.5f * amplitude;
				array[index] += value;
			}
		}
		amplitude *= amplitudeFactor;
		frequency *= frequencyFactor;
	}

	float inv_max_sum = (amplitudeFactor == 1.0f || layers == 1) ? layers : (1.0f - amplitudeFactor) / (1.0f - powf(amplitudeFactor, layers));
	if (inv_max_sum != 1.0f)
	{
		for (int y = 0; y < sizeY; y++)
		{
			for (int x = 0; x < sizeX; x++)
			{
				size_t index = x + y * sizeX;
				array[index] *= inv_max_sum;
			}
		}
	}
}

void TerrainGenerator::getLayeredNoiseArray2D(float* array, float x, float y, int sizeX, int sizeY, const LayeredNoiseData& data)
{
	return getLayeredNoiseArray2D(array, x, y, sizeX, sizeY, data.amplitude, data.frequency, data.layersCount, data.amplitudeFactor, data.frequencyFactor);
}

int TerrainGenerator::getInitialHeight(int globalX, int globalZ)
{
	int chunkX = floorf((float)globalX / (float)Settings::CHUNK_SIZE);
	int chunkZ = floorf((float)globalZ / (float)Settings::CHUNK_SIZE);

	Biome biome = getBiome(chunkX, chunkZ);
	const BiomeData& biomeData_ = biomeData[(size_t)biome];

	float continentalNoise = getLayeredNoise2D(globalX, globalZ, biomeData_.continentalLayer);
	float erosion = getLayeredNoise2D(globalX, globalZ, biomeData_.erosionLayer);
	float weirdness = getLayeredNoise2D(globalX, globalZ, biomeData_.weirdnessLayer);

	float continental = continentalSpline.get(continentalNoise) * biomeData_.continentalAmplitude;
	
	float value = continental;
	if (erosion > biomeData_.erosionThreshold)
	{
		value -= (erosion - biomeData_.erosionThreshold) / (1.0f - biomeData_.erosionThreshold) * biomeData_.erosionAmplitude;
	}

	float pv = (1 - fabsf(3 * fabsf(weirdness) - 2)) * biomeData_.weirdnessAmplitude;
	value += pv;
	return value;
}

void TerrainGenerator::getInitialHeightArray(int* heightArray, int chunkX, int chunkZ, Biome biome)
{
	// TODO: add height interpolating
	int globalChunkX = chunkX * Settings::CHUNK_SIZE;
	int globalChunkZ = chunkZ * Settings::CHUNK_SIZE;

	const BiomeData& biomeData_ = biomeData[(size_t)biome];

	float continentalNoiseArray[Settings::CHUNK_SIZE_SQUARED];
	getLayeredNoiseArray2D(continentalNoiseArray, globalChunkX, globalChunkZ, Settings::CHUNK_SIZE, Settings::CHUNK_SIZE, biomeData_.continentalLayer);

	float erosionNoiseArray[Settings::CHUNK_SIZE_SQUARED];
	getLayeredNoiseArray2D(erosionNoiseArray, globalChunkX, globalChunkZ, Settings::CHUNK_SIZE, Settings::CHUNK_SIZE, biomeData_.erosionLayer);

	float weirdnessNoiseArray[Settings::CHUNK_SIZE_SQUARED];
	getLayeredNoiseArray2D(weirdnessNoiseArray, globalChunkX, globalChunkZ, Settings::CHUNK_SIZE, Settings::CHUNK_SIZE, biomeData_.weirdnessLayer);

	for (int y = 0; y < Settings::CHUNK_SIZE; y++)
	{
		for (int x = 0; x < Settings::CHUNK_SIZE; x++)
		{
			size_t index = x + y * Settings::CHUNK_SIZE;
			float continental = continentalSpline.get(continentalNoiseArray[index]) * biomeData_.continentalAmplitude;
			float erosion = erosionNoiseArray[index];
			float weirdness = weirdnessNoiseArray[index];

			float value = continental;
			if (erosion > biomeData_.erosionThreshold)
			{
				value -= (erosion - biomeData_.erosionThreshold) / (1.0f - biomeData_.erosionThreshold) * biomeData_.erosionAmplitude;
			}

			float pv = (1 - fabsf(3 * fabsf(weirdness) - 2)) * biomeData_.weirdnessAmplitude;
			value += pv;

			heightArray[index] = value;
		}
	}
}

void TerrainGenerator::generateChunkCaveNoise(int chunkX, int chunkY, int chunkZ)
{
	float x = (float)chunkX * Settings::CHUNK_SIZE;
	float y = (float)chunkY * Settings::CHUNK_SIZE;
	float z = (float)chunkZ * Settings::CHUNK_SIZE;
	float scale = 0.08f;
	getNoiseArray3D(chunkCaveNoiseArray, x, y, z, Settings::CHUNK_SIZE, Settings::CHUNK_SIZE, Settings::CHUNK_SIZE, scale);
}

ChunkColumnData* TerrainGenerator::getHeightMap(int chunkX, int chunkZ)
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
	else if (y == height)
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
		return IsCaveInChunk(x, y, z) ? Block::Air : Block::Stone;
	}
	else if (y < height)
	{
		int level = mountainStoneLevel + sinf((x + z) * mountainStoneLevelVariationFreq) * mountainStoneLevelVariationAmpl;
		return y < level ? Block::Dirt : (IsCaveInChunk(x, y, z) ? Block::Air : Block::Stone);
	}
	return Block::Stone;
}

bool TerrainGenerator::IsCaveInChunk(int x, int y, int z)
{
	// TODO: few chunks are 'closed' because caves are too frequent
	x &= Settings::CHUNK_SIZE_MASK;
	y &= Settings::CHUNK_SIZE_MASK;
	z &= Settings::CHUNK_SIZE_MASK;

	float cheese = chunkCaveNoiseArray[x + y * Settings::CHUNK_SIZE + z * Settings::CHUNK_SIZE_SQUARED];
	return cheese < 0.5f;
}

Biome TerrainGenerator::getBiome(int chunkX, int chunkZ)
{
	float frequency = 0.01f;
	float temperature = TerrainGenerator::getLayeredNoise2D((float)chunkX, (float)chunkZ, 3, 1.0f, frequency, 0.5f, 2.0f);
	float humidity = TerrainGenerator::getLayeredNoise2D((float)chunkX + 0.5 / frequency, (float)chunkZ + 0.1 / frequency, 3, 1.0f, frequency, 0.5f, 2.0f);
	return getBiomeByTH(temperature, humidity);
}

int TerrainGenerator::calculateHeight(int globalX, int globalZ)
{
	return 0;
	/*int chunkX = floorf((float)globalX / (float)Settings::CHUNK_SIZE);
	int chunkZ = floorf((float)globalZ / (float)Settings::CHUNK_SIZE);

	Biome biome = getBiome(chunkX, chunkZ);
	return TerrainGenerator::calculateInitialHeight(globalX, globalZ, biome);

	int localX = globalX & Settings::CHUNK_SIZE_MASK;
	int localZ = globalZ & Settings::CHUNK_SIZE_MASK;

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
	}*/
}

void TerrainGenerator::init()
{
	simplexNoise = FastNoise::New<FastNoise::Simplex>();

	size_t heightMapPoolSize = calcArea(Settings::CHUNK_LOAD_RADIUS);
	heightMapPool.reserve(heightMapPoolSize);
}

void TerrainGenerator::clear()
{
	for (const auto& pair : heightMaps)
	{
		ChunkColumnData* data = pair.second;
		saveSkyLightMaxHeightMapToFile(data);
		delete data;
	}
	heightMapPool.clear();
}

void TerrainGenerator::loadHeightMap(int chunkX, int chunkZ)
{
	auto hash = pos2_hash(chunkX, chunkZ);
	const auto& it = heightMaps.find(hash);
	if (it != heightMaps.end())
	{
		return;
	}

	ChunkColumnData* chunkColumnData = heightMapPool.acquire();
	chunkColumnData->startUsing();
	chunkColumnData->X = chunkX;
	chunkColumnData->Z = chunkZ;
	chunkColumnData->biome = getBiome(chunkX, chunkZ);

	// height
	getInitialHeightArray(chunkColumnData->heightMap, chunkX, chunkZ, chunkColumnData->biome);

	// slmh
	if (!loadSkyLightMaxHeightMapFromFile(chunkX, chunkZ, chunkColumnData))
	{
		for (int z = 0; z < Settings::CHUNK_SIZE; z++)
		{
			for (int x = 0; x < Settings::CHUNK_SIZE; x++)
			{
				chunkColumnData->setSlMHAt(x, z, INT_MIN);
			}
		}
	}
	chunkColumnData->stopUsing();

	//
	heightMaps[hash] = chunkColumnData;
}

void TerrainGenerator::unloadHeightMap(int chunkX, int chunkZ)
{
	auto hash = pos2_hash(chunkX, chunkZ);
	const auto& it = heightMaps.find(hash);
	if (it == heightMaps.end())
	{
		return;
	}
	auto usedBy = it->second->usedBy.load();
	if (usedBy > 0)
	{
		std::cerr << "Unloaded ChunkColumnData was used by: " << usedBy << " chunks" << std::endl;
	}

	ChunkColumnData* chunkColumnData = it->second;

	chunkColumnData->startUsing();
	saveSkyLightMaxHeightMapToFile(chunkColumnData);
	chunkColumnData->stopUsing();

	heightMapPool.release(chunkColumnData);
	heightMaps.erase(it);

}

bool TerrainGenerator::loadSkyLightMaxHeightMapFromFile(int chunkX, int chunkZ, ChunkColumnData* chunkColumnData)
{
	if (!Settings::loadSMLHFiles)
	{
		return false;
	}

	std::string filepath = ChunkColumnData::slmhGetFilepath(chunkX, chunkZ);
	if (!std::filesystem::exists(filepath))
	{
		return false;
	}

	std::ifstream file(filepath, std::ios::binary);
	if (!file.is_open())
	{
		std::cerr << "Failed to open sky light max height map file" << std::endl;
		return false;
	}

	// TODO: TAKES TIME!
	file.read
	(
		reinterpret_cast<char*>(chunkColumnData->skyLightMaxHeightMap),
		sizeof(chunkColumnData->skyLightMaxHeightMap)
	);
	file.close();
	return true;
}

void TerrainGenerator::saveSkyLightMaxHeightMapToFile(const ChunkColumnData* chunkColumnData)
{
	if (!Settings::loadSMLHFiles)
	{
		return;
	}

	std::string filepath = ChunkColumnData::slmhGetFilepath(chunkColumnData->X, chunkColumnData->Z);
	std::ofstream file(filepath, std::ios::binary | std::ios::trunc);
	if (!file.is_open())
	{
		std::cerr << "Failed to open sky light max height map file" << std::endl;
		return;
	}

	try
	{
		file.write
		(
			reinterpret_cast<const char*>(&chunkColumnData->skyLightMaxHeightMap),
			sizeof(chunkColumnData->skyLightMaxHeightMap)
		);
	}
	catch (const std::exception& e)
	{
		std::cerr << "Failed to write to sky light max height map file: " << e.what() << std::endl;
	}
	file.close();
}

ChunkColumnData::ChunkColumnData() : X(0), Z(0), usedBy(0)
{
}

void ChunkColumnData::setHeightAt(size_t x, size_t z, int height)
{
	heightMap[x + z * Settings::CHUNK_SIZE] = height;
}

int ChunkColumnData::getHeightAt(size_t x, size_t z) const
{
	return heightMap[x + z * Settings::CHUNK_SIZE];
}

int ChunkColumnData::getHeightAtByIndex(size_t index) const
{
	return heightMap[index];
}

void ChunkColumnData::setSlMHAt(size_t x, size_t z, int height)
{
	// TODO: add mutex
	skyLightMaxHeightMap[x + z * Settings::CHUNK_SIZE] = height;
}

int ChunkColumnData::getSlMHAt(size_t x, size_t z) const
{
	// TODO: add mutex
	return skyLightMaxHeightMap[x + z * Settings::CHUNK_SIZE];
}

Biome ChunkColumnData::getBiome() const
{
	return biome;
}

std::string ChunkColumnData::slmhGetFilepath(int chunkX, int chunkZ)
{
	std::string path = Settings::skyLightMaxHeightMapSavesPath;
	path += std::to_string(chunkX);
	path += "_";
	path += std::to_string(chunkZ);
	path += ".bin";
	return path;
}

void ChunkColumnData::startUsing()
{
	usedBy.fetch_add(1);
}

void ChunkColumnData::stopUsing()
{
	usedBy.fetch_sub(1);
}
