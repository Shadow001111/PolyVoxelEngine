#include "Chunk.h"
#include <cstring>
#include <string>
#include <iostream>
#include "TerrainGenerator.h"
#include <unordered_set>
#include "Profiler.h"

Face* Chunk::facesData = nullptr;
FaceInstanceData* Chunk::faceInstancesData = nullptr;
FaceInstancesVBO* Chunk::faceInstancesVBO = nullptr;
std::unordered_map<int, Chunk*> Chunk::chunkMap;
std::queue<Light> Chunk::lightingFloodFillQueue;
std::queue<Light> Chunk::darknessFloodFillQueue;
std::vector<LightUpdate> Chunk::lightingUpdateVector;

inline constexpr int min_int(int a, int b)
{
	return a < b ? a : b;
}

inline constexpr int max_int(int a, int b)
{
	return a > b ? a : b;
}

inline constexpr int clamp_int(int value, int min_, int max_)
{
	return min_int(max_, max_int(min_, value));
}

int pos3_hash(int x, int y, int z)
{
	constexpr int shift = sizeof(int) * 8 / 3;
	constexpr int mod = (1 << shift) - 1;
	return (x & mod) | ((y & mod) << shift) | ((z & mod) << (shift * 2));
}

Chunk::Chunk(unsigned int ID) : blocks{}, lightingMap{}, X(0), Y(0), Z(0), neighbours{ nullptr, nullptr, nullptr, nullptr, nullptr, nullptr }
{
	drawCommand.offset = unsigned int(ID * Settings::FACE_INSTANCES_PER_CHUNK);
}

Chunk::~Chunk()
{}

void Chunk::init(int x, int y, int z)
{
	X = x;
	Y = y;
	Z = z;

	chunkMap[posHash()] = this;

	neighbours[0] = getChunkAt(X + 1, Y, Z);
	neighbours[1] = getChunkAt(X - 1, Y, Z);
	neighbours[2] = getChunkAt(X, Y + 1, Z);
	neighbours[3] = getChunkAt(X, Y - 1, Z);
	neighbours[4] = getChunkAt(X, Y, Z + 1);
	neighbours[5] = getChunkAt(X, Y, Z - 1);

	for (size_t i = 0; i < 6; i++)
	{
		if (neighbours[i])
		{
			neighbours[i]->neighbours[i ^ 1] = this;
		}
	}

	// lighting
	memset(lightingMap, 0, sizeof(lightingMap));
}

void Chunk::destroy()
{
	if (physicEntities.getSize() > 0)
	{
		std::cerr << "Entities are in destroyed chunk" << std::endl;
	}
	physicEntities.clear();

	saveData(blockChanges, X, Y, Z);
}

void Chunk::generateBlocks()
{
	Profiler::start(BLOCK_GENERATION_INDEX);
	blocksCount = 0;
	for (size_t i = 0; i < Settings::CHUNK_SIZE_CUBED; i++)
	{
		blocks[i] = Block::Air;
	}

	Biome biome = TerrainGenerator::getBiome(X, Z);
	int chunkMaxY;
	{
		BiomeData biomeData_ = biomeData[(size_t)biome];
		chunkMaxY = ceilf((float)biomeData_.maxHeight / (float)Settings::CHUNK_SIZE);
	}

	const HeightMap* heightMap = TerrainGenerator::getHeightMap(X, Z);
	if (Y <= chunkMaxY)
	{
		for (size_t z = 0; z < Settings::CHUNK_SIZE; z++)
		{
			int globalZ = Z * Settings::CHUNK_SIZE + z;
			for (size_t x = 0; x < Settings::CHUNK_SIZE; x++)
			{
				int globalX = X * Settings::CHUNK_SIZE + x;

				int globalHeight = heightMap->getHeightAt(x, z);
				int localHeight = globalHeight - Y * Settings::CHUNK_SIZE;

				size_t height = clamp_int(localHeight, 0, Settings::CHUNK_SIZE);
				for (size_t y = 0; y < Settings::CHUNK_SIZE; y++)
				{
					int globalY = Y * Settings::CHUNK_SIZE + y;
					Block block = TerrainGenerator::getBlock(globalX, globalY, globalZ, globalHeight, biome);
					setBlockAtNoSave(x, y, z, block);
					setLightingAtInBoundaries(x, y, z, 0, false);
				}
			}
		}
	}
	Profiler::end(BLOCK_GENERATION_INDEX);

	Profiler::start(CHUNK_LOAD_DATA_INDEX);
	loadData(blockChanges, X, Y, Z);
	Profiler::end(CHUNK_LOAD_DATA_INDEX);

	applyChanges();

	// lighting
	Profiler::start(CHUNK_LIGHTING_INDEX);
	for (size_t x = 0; x < Settings::CHUNK_SIZE; x++)
	{
		int globalX = (int)x + X * Settings::CHUNK_SIZE;
		for (size_t z = 0; z < Settings::CHUNK_SIZE; z++)
		{
			int globalZ = (int)z + Z * Settings::CHUNK_SIZE;
			int slmh = heightMap->getSlMHAt(x, z);

			for (size_t y = 0; y < Settings::CHUNK_SIZE; y++)
			{
				int globalY = (int)y + Y * Settings::CHUNK_SIZE;

				Block block = getBlockAtInBoundaries(x, y, z);
				if (ALL_BLOCK_DATA[(size_t)block].transparent)
				{
					if (globalY < slmh)
					{
						darknessFloodFillQueue.emplace(
							globalX, globalY, globalZ,
							15, true
						);
					}
					lightingMap[getIndex(x, y, z)] = 240;
				}
				else
				{
					lightingMap[getIndex(x, y, z)] = 0;
				}
			}
		}
	}
	for (int x = -1; x <= Settings::CHUNK_SIZE; x += (Settings::CHUNK_SIZE + 1))
	{
		int globalX = x + X * Settings::CHUNK_SIZE;
		for (int z = 0; z < Settings::CHUNK_SIZE; z++)
		{
			int globalZ = z + Z * Settings::CHUNK_SIZE;
			for (int y = 0; y < Settings::CHUNK_SIZE; y++)
			{
				int globalY = y + Y * Settings::CHUNK_SIZE;

				uint8_t lighting = getLightingAtSideCheck(x, y, z, x == -1 ? 1 : 0) & 15;
				if (lighting > 1)
				{
					lightingFloodFillQueue.emplace
					(
						globalX, globalY, globalZ,
						lighting, false
					);
				}
			}
		}
	}
	for (int y = -1; y <= Settings::CHUNK_SIZE; y += (Settings::CHUNK_SIZE + 1))
	{
		int globalY = y + Y * Settings::CHUNK_SIZE;
		for (int z = 0; z < Settings::CHUNK_SIZE; z++)
		{
			int globalZ = z + Z * Settings::CHUNK_SIZE;
			for (int x = 0; x < Settings::CHUNK_SIZE; x++)
			{
				int globalX = x + X * Settings::CHUNK_SIZE;

				uint8_t lighting = getLightingAtSideCheck(x, y, z, y == -1 ? 3 : 2) & 15;
				if (lighting > 1)
				{
					lightingFloodFillQueue.emplace
					(
						globalX, globalY, globalZ,
						lighting, false
					);
				}
			}
		}
	}
	for (int z = -1; z <= Settings::CHUNK_SIZE; z += (Settings::CHUNK_SIZE + 1))
		{
			int globalZ = z + Z * Settings::CHUNK_SIZE;
			for (int x = 0; x < Settings::CHUNK_SIZE; x++)
			{
				int globalX = x + X * Settings::CHUNK_SIZE;
				for (int y = 0; y < Settings::CHUNK_SIZE; y++)
				{
					int globalY = y + Y * Settings::CHUNK_SIZE;

					uint8_t lighting = getLightingAtSideCheck(x, y, z, z == -1 ? 5 : 4) & 15;
					if (lighting > 1)
					{
						lightingFloodFillQueue.emplace
						(
							globalX, globalY, globalZ,
							lighting, false
						);
					}
				}
			}
		}
	Profiler::end(CHUNK_LIGHTING_INDEX);
}

void Chunk::generateFaces()
{
	drawCommand.resetFaces();
	hasAnyFaces = false;

	if (blocksCount == 0 || isChunkClosed())
	{
		return;
	}

	// generate
	int offsets[3] = { 0, 0, 0 };

	const char packOffsets[6][4] =
	{
		// offset = 3 - order
		{1, 2, 3, 0},
		{0, 3, 2, 1},
		{3, 2, 1, 0},
		{0, 1, 2, 3},
		{0, 3, 2, 1},
		{1, 2, 3, 0}
	};

	// get grid face  data
	for (int x = 0; x < Settings::CHUNK_SIZE; x++)
	{
		for (int y = 0; y < Settings::CHUNK_SIZE; y++)
		{
			for (int z = 0; z < Settings::CHUNK_SIZE; z++)
			{
				Block block = getBlockAtInBoundaries(x, y, z);
				BlockData blockData = ALL_BLOCK_DATA[(size_t)block];
				if (!blockData.createFaces)
				{
					continue;
				}

				const auto& textures = blockData.textures;

				bool maxAO = blockData.lightPower > 0;
				for (size_t normalID = 0; normalID < 6; normalID++)
				{
					size_t planeIndex = normalID >> 1;
					offsets[planeIndex] = ((normalID & 1 ^ 1) << 1) - 1;

					Block faceBlock = getBlockAtSideCheck(x + offsets[0], y + offsets[1], z + offsets[2], normalID);
					if (faceBlock != Block::Void && faceBlock != block && ALL_BLOCK_DATA[(size_t)faceBlock].transparent)
					{
						auto& face = facesData[normalID + (z + (y + x * Settings::CHUNK_SIZE) * Settings::CHUNK_SIZE) * 6];
						face.none = false;
						face.transparent = blockData.transparent;
						face.textureID = textures[normalID];
						face.lighting = getLightingAtSideCheck(x + offsets[0], y + offsets[1], z + offsets[2], normalID);
#if ENABLE_SMOOTH_LIGHTING
						char ao = getAOandSmoothLighting(maxAO, x + offsets[0], y + offsets[1], z + offsets[2], planeIndex, packOffsets[normalID], face.smoothLighting);
#else
						char ao = 255;
						if (blockData.lightPower == 0)
						{
							ao = getAO(x + offsets[0], y + offsets[1], z + offsets[2], planeIndex, packOffsets[normalID]);
						}
#endif
						face.ao = ao;
					}
					offsets[planeIndex] = 0;
				}
			}
		}
	}

	greedyMeshing(facesData);
	hasAnyFaces = drawCommand.anyFaces();
	if (hasAnyFaces)
	{
		updateFacesData();
	}
}

void Chunk::updateFacesData()
{
	for (size_t i = 0; i < 6; i++)
	{
		size_t count = drawCommand.facesCount[i];
		size_t offset = i * (Settings::FACE_INSTANCES_PER_CHUNK / 6);
		faceInstancesVBO->setData(faceInstancesData + offset, drawCommand.offset + offset, count);

		count = drawCommand.facesCount[6 + i];
		offset = (i + 1) * (Settings::FACE_INSTANCES_PER_CHUNK / 6) - count;
		faceInstancesVBO->setData(faceInstancesData + offset, drawCommand.offset + offset, count);
	}
}

Chunk* Chunk::getChunkAt(int x, int y, int z)
{
	const auto& it = chunkMap.find(pos3_hash(x, y, z));
	if (it == chunkMap.end())
	{
		return nullptr;
	}
	return it->second;
}

char Chunk::getAO(int x, int y, int z, char side, const char* packOffsets) const
{
	bool a, b, c, d, e, f, g, h;
	switch (side)
	{
	case 0:
		a = ALL_BLOCK_DATA[(size_t)getBlockAt(x, y, z - 1)].transparent;
		b = ALL_BLOCK_DATA[(size_t)getBlockAt(x, y - 1, z - 1)].transparent;
		c = ALL_BLOCK_DATA[(size_t)getBlockAt(x, y - 1, z)].transparent;
		d = ALL_BLOCK_DATA[(size_t)getBlockAt(x, y - 1, z + 1)].transparent;
		e = ALL_BLOCK_DATA[(size_t)getBlockAt(x, y, z + 1)].transparent;
		f = ALL_BLOCK_DATA[(size_t)getBlockAt(x, y + 1, z + 1)].transparent;
		g = ALL_BLOCK_DATA[(size_t)getBlockAt(x, y + 1, z)].transparent;
		h = ALL_BLOCK_DATA[(size_t)getBlockAt(x, y + 1, z - 1)].transparent;
		break;
	case 1:
		a = ALL_BLOCK_DATA[(size_t)getBlockAt(x, y, z - 1)].transparent;
		b = ALL_BLOCK_DATA[(size_t)getBlockAt(x - 1, y, z - 1)].transparent;
		c = ALL_BLOCK_DATA[(size_t)getBlockAt(x - 1, y, z)].transparent;
		d = ALL_BLOCK_DATA[(size_t)getBlockAt(x - 1, y, z + 1)].transparent;
		e = ALL_BLOCK_DATA[(size_t)getBlockAt(x, y, z + 1)].transparent;
		f = ALL_BLOCK_DATA[(size_t)getBlockAt(x + 1, y, z + 1)].transparent;
		g = ALL_BLOCK_DATA[(size_t)getBlockAt(x + 1, y, z)].transparent;
		h = ALL_BLOCK_DATA[(size_t)getBlockAt(x + 1, y, z - 1)].transparent;
		break;
	case 2:
		a = ALL_BLOCK_DATA[(size_t)getBlockAt(x - 1, y, z)].transparent;
		b = ALL_BLOCK_DATA[(size_t)getBlockAt(x - 1, y - 1, z)].transparent;
		c = ALL_BLOCK_DATA[(size_t)getBlockAt(x, y - 1, z)].transparent;
		d = ALL_BLOCK_DATA[(size_t)getBlockAt(x + 1, y - 1, z)].transparent;
		e = ALL_BLOCK_DATA[(size_t)getBlockAt(x + 1, y, z)].transparent;
		f = ALL_BLOCK_DATA[(size_t)getBlockAt(x + 1, y + 1, z)].transparent;
		g = ALL_BLOCK_DATA[(size_t)getBlockAt(x, y + 1, z)].transparent;
		h = ALL_BLOCK_DATA[(size_t)getBlockAt(x - 1, y + 1, z)].transparent;
		break;
	}
	char ao0 = a + b + c;
	char ao1 = g + h + a;
	char ao2 = e + f + g;
	char ao3 = c + d + e;
	return  
		(ao0 << (packOffsets[0] << 1)) |
		(ao1 << (packOffsets[1] << 1)) |
		(ao2 << (packOffsets[2] << 1)) |
		(ao3 << (packOffsets[3] << 1));
}

size_t Chunk::getIndex(size_t x, size_t y, size_t z)
{
	return x + y * Settings::CHUNK_SIZE + z * Settings::CHUNK_SIZE_SQUARED;
}

SizeT3 Chunk::getCoordinatesByIndex(size_t index)
{
	size_t z = index / Settings::CHUNK_SIZE_SQUARED;
	size_t index_xy = index - z * Settings::CHUNK_SIZE_SQUARED;
	size_t y = index_xy / Settings::CHUNK_SIZE;
	size_t x = index_xy - y * Settings::CHUNK_SIZE;
	return SizeT3(x, y, z);
}

char Chunk::getAOandSmoothLighting(bool maxAO, int x, int y, int z, size_t side, const char* packOffsets, uint8_t* smoothLighting) const
{
	BlockAndLighting center, a, b, c, d, e, f, g, h;

	center = getBlockAndLightingAt(x, y, z);

	switch (side)
	{
	case 0:
		a = getBlockAndLightingAt(x, y, z - 1);
		b = getBlockAndLightingAt(x, y - 1, z - 1);
		c = getBlockAndLightingAt(x, y - 1, z);
		d = getBlockAndLightingAt(x, y - 1, z + 1);
		e = getBlockAndLightingAt(x, y, z + 1);
		f = getBlockAndLightingAt(x, y + 1, z + 1);
		g = getBlockAndLightingAt(x, y + 1, z);
		h = getBlockAndLightingAt(x, y + 1, z - 1);
		break;
	case 1:
		a = getBlockAndLightingAt(x, y, z - 1);
		b = getBlockAndLightingAt(x - 1, y, z - 1);
		c = getBlockAndLightingAt(x - 1, y, z);
		d = getBlockAndLightingAt(x - 1, y, z + 1);
		e = getBlockAndLightingAt(x, y, z + 1);
		f = getBlockAndLightingAt(x + 1, y, z + 1);
		g = getBlockAndLightingAt(x + 1, y, z);
		h = getBlockAndLightingAt(x + 1, y, z - 1);
		break;
	case 2:
		a = getBlockAndLightingAt(x - 1, y, z);
		b = getBlockAndLightingAt(x - 1, y - 1, z);
		c = getBlockAndLightingAt(x, y - 1, z);
		d = getBlockAndLightingAt(x + 1, y - 1, z);
		e = getBlockAndLightingAt(x + 1, y, z);
		f = getBlockAndLightingAt(x + 1, y + 1, z);
		g = getBlockAndLightingAt(x, y + 1, z);
		h = getBlockAndLightingAt(x - 1, y + 1, z);
		break;
	}

	bool bcenter = ALL_BLOCK_DATA[(size_t)center.block].transparent;
	bool ba = ALL_BLOCK_DATA[(size_t)a.block].transparent;
	bool bb = ALL_BLOCK_DATA[(size_t)b.block].transparent;
	bool bc = ALL_BLOCK_DATA[(size_t)c.block].transparent;
	bool bd = ALL_BLOCK_DATA[(size_t)d.block].transparent;
	bool be = ALL_BLOCK_DATA[(size_t)e.block].transparent;
	bool bf = ALL_BLOCK_DATA[(size_t)f.block].transparent;
	bool bg = ALL_BLOCK_DATA[(size_t)g.block].transparent;
	bool bh = ALL_BLOCK_DATA[(size_t)h.block].transparent;

	uint8_t lcenter = center.lighting;
	uint8_t la = a.lighting;
	uint8_t lb = b.lighting;
	uint8_t lc = c.lighting;
	uint8_t ld = d.lighting;
	uint8_t le = e.lighting;
	uint8_t lf = f.lighting;
	uint8_t lg = g.lighting;
	uint8_t lh = h.lighting;

	/*if (
		(!bcenter && lcenter > 0) ||
		(!ba && la > 0) ||
		(!bb && lb > 0) ||
		(!bc && lc > 0) ||
		(!bd && ld > 0) ||
		(!be && le > 0) ||
		(!bf && lf > 0) ||
		(!bg && lg > 0) ||
		(!bh && lh > 0))
	{
		std::cerr << "Lighting in solid block" << std::endl;
	}*/

	uint8_t sum0 = bcenter + ba + bb + bc;
	uint8_t sum1 = bcenter + bg + bh + ba;
	uint8_t sum2 = bcenter + be + bf + bg;
	uint8_t sum3 = bcenter + bc + bd + be;

	uint8_t bl0 = ((lcenter & 15) + (la & 15) + (lb & 15) + (lc & 15)) / sum0;
	uint8_t bl1 = ((lcenter & 15) + (lg & 15) + (lh & 15) + (la & 15)) / sum1;
	uint8_t bl2 = ((lcenter & 15) + (le & 15) + (lf & 15) + (lg & 15)) / sum2;
	uint8_t bl3 = ((lcenter & 15) + (lc & 15) + (ld & 15) + (le & 15)) / sum3;

	uint8_t sl0 = ((lcenter >> 4) + (la >> 4) + (lb >> 4) + (lc >> 4)) / sum0;
	uint8_t sl1 = ((lcenter >> 4) + (lg >> 4) + (lh >> 4) + (la >> 4)) / sum1;
	uint8_t sl2 = ((lcenter >> 4) + (le >> 4) + (lf >> 4) + (lg >> 4)) / sum2;
	uint8_t sl3 = ((lcenter >> 4) + (lc >> 4) + (ld >> 4) + (le >> 4)) / sum3;

	smoothLighting[packOffsets[0]] = (sl0 << 4) | bl0;
	smoothLighting[packOffsets[1]] = (sl1 << 4) | bl1;
	smoothLighting[packOffsets[2]] = (sl2 << 4) | bl2;
	smoothLighting[packOffsets[3]] = (sl3 << 4) | bl3;

	if (maxAO)
	{
		return 255;
	}

	char ao0 = ba + bb + bc;
	char ao1 = bg + bh + ba;
	char ao2 = be + bf + bg;
	char ao3 = bc + bd + be;
	return  
		(ao0 << (packOffsets[0] << 1)) |
		(ao1 << (packOffsets[1] << 1)) |
		(ao2 << (packOffsets[2] << 1)) |
		(ao3 << (packOffsets[3] << 1));
}

void Chunk::setBlockByIndexNoSave(size_t index, Block block)
{
	Block prevBlock = blocks[index];
	if (prevBlock == block)
	{
		return;
	}
	blocks[index] = block;
	if (prevBlock == Block::Air)
	{
		blocksCount++;
	}
	else if (block == Block::Air)
	{
		blocksCount--;
	}

	auto pos = getCoordinatesByIndex(index);
	updateLightingAt(pos.x, pos.y, pos.z, block, prevBlock);
}

void Chunk::setBlockAtNoSave(size_t x, size_t y, size_t z, Block block)
{
	size_t index = getIndex(x, y, z);
	Block prevBlock = blocks[index];
	if (prevBlock == block)
	{
		return;
	}
	blocks[index] = block;
	if (prevBlock == Block::Air)
	{
		blocksCount++;
	}
	else if (block == Block::Air)
	{
		blocksCount--;
	}

	updateLightingAt(x, y, z, block, prevBlock);
}

void Chunk::loadData(std::unordered_map<Block, Vector<uint16_t, Settings::CHUNK_SIZE_CUBED>>& blockChanges, int X, int Y, int Z)
{
	blockChanges.clear();

	std::string filepath = getFilepath(X, Y, Z);
	if (!std::filesystem::exists(filepath))
	{
		return;
	}

	std::ifstream file(filepath, std::ios::binary);
	if (!file.is_open())
	{
		std::cerr << "Failed to open chunk data file" << std::endl;
		return;
	}

	size_t sizeOfBlock = 0;
	unsigned int mapSize = 0;

	file.read(reinterpret_cast<char*>(&sizeOfBlock), 1);
	file.read(reinterpret_cast<char*>(&mapSize), sizeOfBlock);

	Vector<uint16_t, Settings::CHUNK_SIZE_CUBED> indexes;

	for (unsigned int i = 0; i < mapSize; i++)
	{
		Block block;
		uint16_t count;

		file.read(reinterpret_cast<char*>(&count), sizeof(count));
		if (count == 0)
		{
			std::cerr << "Chunk::loadData: block count is 0" << std::endl;
			continue;
		}

		file.read(reinterpret_cast<char*>(&block), sizeOfBlock);

		indexes.resize(count);
		file.read
		(
			reinterpret_cast<char*>(indexes.getData()),
			count * sizeof(indexes[0])
		);

		blockChanges[block] = std::move(indexes);
	}

	file.close();
}

void Chunk::saveData(std::unordered_map<Block, Vector<uint16_t, Settings::CHUNK_SIZE_CUBED>>& blockChanges, int X, int Y, int Z)
{
	if (blockChanges.empty())
	{
		return;
	}

	std::string filepath = getFilepath(X, Y, Z);
	std::ofstream file(filepath, std::ios::binary | std::ios::trunc);
	if (!file.is_open())
	{
		std::cerr << "Failed to open chunk data file " << std::endl;
		return;
	}

	try 
	{
		size_t sizeOfBlock = sizeof(Block);
		Block mapSize = (Block)blockChanges.size();

		file.write
		(
			reinterpret_cast<const char*>(&sizeOfBlock),
			1
		);
		file.write
		(
			reinterpret_cast<const char*>(&mapSize),
			sizeOfBlock
		);

		for (const auto& pair : blockChanges)
		{
			const auto& indexes = pair.second;
			Block block = pair.first;
			uint16_t count = (uint16_t)indexes.getSize();

			file.write
			(
				reinterpret_cast<const char*>(&count),
				sizeof(count)
			);
			file.write
			(
				reinterpret_cast<const char*>(&block),
				sizeOfBlock
			);
			file.write
			(
				reinterpret_cast<const char*>(indexes.getData()),
				indexes.getSize() * sizeof(indexes[0])
			);
		}
	}
	catch (const std::exception& e) 
	{
		std::cerr << "Failed to write to chunk data file: " << e.what() << std::endl;
	}
	file.close();
}

void Chunk::applyChanges()
{
	for (const auto& pair : blockChanges)
	{
		Block block = pair.first;
		const auto& vector = pair.second;
		for (uint16_t index : vector)
		{
			setBlockByIndexNoSave(index, block);
		}
	}
}

bool Chunk::isChunkClosed() const
{
	// right
	const Chunk* neighbour = neighbours[0];
	if (neighbour)
	{
		for (int y = 0; y < Settings::CHUNK_SIZE; y++)
		{
			for (int z = 0; z < Settings::CHUNK_SIZE; z++)
			{
				Block faceBlock = neighbour->getBlockAtInBoundaries(0, y, z);
				if (ALL_BLOCK_DATA[(size_t)faceBlock].transparent)
				{
					return false;
				}
			}
		}
	}
	// left
	neighbour = neighbours[1];
	if (neighbour)
	{
		for (int y = 0; y < Settings::CHUNK_SIZE; y++)
		{
			for (int z = 0; z < Settings::CHUNK_SIZE; z++)
			{
				Block faceBlock = neighbour->getBlockAtInBoundaries(Settings::CHUNK_SIZE - 1, y, z);
				if (ALL_BLOCK_DATA[(size_t)faceBlock].transparent)
				{
					return false;
				}
			}
		}
	}
	// top
	neighbour = neighbours[2];
	if (neighbour)
	{
		for (int x = 0; x < Settings::CHUNK_SIZE; x++)
		{
			for (int z = 0; z < Settings::CHUNK_SIZE; z++)
			{
				Block faceBlock = neighbour->getBlockAtInBoundaries(x, 0, z);
				if (ALL_BLOCK_DATA[(size_t)faceBlock].transparent)
				{
					return false;
				}
			}
		}
	}
	// bottom
	neighbour = neighbours[3];
	if (neighbour)
	{
		for (int x = 0; x < Settings::CHUNK_SIZE; x++)
		{
			for (int z = 0; z < Settings::CHUNK_SIZE; z++)
			{
				Block faceBlock = neighbour->getBlockAtInBoundaries(x, Settings::CHUNK_SIZE - 1, z);
				if (ALL_BLOCK_DATA[(size_t)faceBlock].transparent)
				{
					return false;
				}
			}
		}
	}
	// front
	neighbour = neighbours[4];
	if (neighbour)
	{
		for (int x = 0; x < Settings::CHUNK_SIZE; x++)
		{
			for (int y = 0; y < Settings::CHUNK_SIZE; y++)
			{
				Block faceBlock = neighbour->getBlockAtInBoundaries(x, y, 0);
				if (ALL_BLOCK_DATA[(size_t)faceBlock].transparent)
				{
					return false;
				}
			}
		}
	}
	// front
	neighbour = neighbours[5];
	if (neighbour)
	{
		for (int x = 0; x < Settings::CHUNK_SIZE; x++)
		{
			for (int y = 0; y < Settings::CHUNK_SIZE; y++)
			{
				Block faceBlock = neighbour->getBlockAtInBoundaries(x, y, Settings::CHUNK_SIZE - 1);
				if (ALL_BLOCK_DATA[(size_t)faceBlock].transparent)
				{
					return false;
				}
			}
		}
	}
	return true;
}

void Chunk::greedyMeshing(Face* facesData)
{
	auto getFaceIndex = [](const size_t* coords, size_t normalID)
	{
		return normalID + (coords[2] + (coords[1] + coords[0] * Settings::CHUNK_SIZE) * Settings::CHUNK_SIZE) * 6;
	};

	const size_t wIndexes[3] = {1, 0, 0};
	const size_t hIndexes[3] = { 2, 2, 1 };

	unsigned int* facesCount = drawCommand.facesCount;

	for (size_t normalID = 0; normalID < 6; normalID++)
	{
		size_t plane = normalID >> 1;
		size_t wCoordIndex = wIndexes[plane];
		size_t hCoordIndex = hIndexes[plane];

		size_t coords[3] = { 0, 0, 0 };
		for (coords[0] = 0; coords[0] < Settings::CHUNK_SIZE; coords[0]++)
		{
			for (coords[1] = 0; coords[1] < Settings::CHUNK_SIZE; coords[1]++)
			{
				for (coords[2] = 0; coords[2] < Settings::CHUNK_SIZE; coords[2]++)
				{
					const auto& currentFace = facesData[getFaceIndex(coords, normalID)];
					if (currentFace.none)
					{
						continue;
					}
					int currentW = 1;
					int currentH = 1;

					// expand W
					size_t copyCoords[3] = {0, 0, 0};
					memcpy(copyCoords, coords, sizeof(copyCoords));
					copyCoords[wCoordIndex]++;
					while (coords[wCoordIndex] + currentW < Settings::CHUNK_SIZE + 1)
					{
						const auto& tempFace = facesData[getFaceIndex(copyCoords, normalID)];
						if (tempFace.none || !(tempFace == currentFace))
						{
							break;
						}
						currentW++;
						copyCoords[wCoordIndex]++;
					}

					// expand H
					memcpy(copyCoords, coords, sizeof(copyCoords));
					copyCoords[hCoordIndex]++;
					while (coords[hCoordIndex] + currentH < Settings::CHUNK_SIZE)
					{
						bool stopExpandH = false;
						for (size_t dw = 0; dw < currentW; dw++)
						{
							copyCoords[wCoordIndex] = coords[wCoordIndex] + dw;
							const auto& tempFace = facesData[getFaceIndex(copyCoords, normalID)];
							if (tempFace.none || !(tempFace == currentFace))
							{
								stopExpandH = true;
								break;
							}
						}
						if (stopExpandH)
						{
							break;
						}
						currentH++;
						copyCoords[hCoordIndex]++;
					}

					// fill
					memcpy(copyCoords, coords, sizeof(copyCoords));
					for (size_t dw = 0; dw < currentW; dw++)
					{
						copyCoords[hCoordIndex] = coords[hCoordIndex];
						for (size_t dh = 0; dh < currentH; dh++)
						{
							auto& tempFace = facesData[getFaceIndex(copyCoords, normalID)];
							tempFace.none = true;
							copyCoords[hCoordIndex]++;
						}
						copyCoords[wCoordIndex]++;
					}

					// add face
					size_t index;
					if (currentFace.transparent)
					{
						size_t faceIndex = facesCount[6 + normalID]++;
						index = (normalID + 1) * (Settings::FACE_INSTANCES_PER_CHUNK / 6) - 1 - faceIndex;
					}
					else
					{
						size_t faceIndex = facesCount[normalID]++;
						index = normalID * (Settings::FACE_INSTANCES_PER_CHUNK / 6) + faceIndex;
					}
#if ENABLE_SMOOTH_LIGHTING
					faceInstancesData[index].set(
						coords[0], coords[1], coords[2], currentW, currentH, normalID, currentFace.ao, currentFace.textureID, currentFace.lighting, currentFace.smoothLighting
					);
#else
					faceInstancesData[index].set(
						coords[0], coords[1], coords[2], currentW, currentH, normalID, currentFace.ao, currentFace.textureID, currentFace.lighting
					);
#endif
				}
			}
		}
	}
}

void Chunk::updateLightingAt(size_t x, size_t y, size_t z, Block block, Block prevBlock)
{
	lightingUpdateVector.emplace_back
	(
		this, x, y, z,
		block, prevBlock
	);
}

Block Chunk::getBlockAtInBoundaries(size_t x, size_t y, size_t z) const
{
	return blocks[getIndex(x, y, z)];
}

bool Chunk::setBlockAtInBoundaries(size_t x, size_t y, size_t z, Block block)
{
	size_t index = getIndex(x, y, z);
	Block prevBlock = blocks[index];
	if (prevBlock == block)
	{
		return false;
	}
	blocks[index] = block;
	if (prevBlock == Block::Air)
	{
		blocksCount++;
	}
	else if (block == Block::Air)
	{
		blocksCount--;
	}

	// lighting
	updateLightingAt(x, y, z, block, prevBlock);

	// save changes
	uint16_t saveIndex = (uint16_t)index;

	const auto& it = blockChanges.find(prevBlock);
	if (it != blockChanges.end())
	{
		auto& prevVec = it->second;

		prevVec.remove(saveIndex);
		if (prevVec.getSize() == 0)
		{
			blockChanges.erase(it);
		}
	}

	blockChanges[block].push(saveIndex);
	return true;
}

Block Chunk::getBlockAt(int x, int y, int z) const
{
	if (
		x >= 0 && x < Settings::CHUNK_SIZE &&
		y >= 0 && y < Settings::CHUNK_SIZE &&
		z >= 0 && z < Settings::CHUNK_SIZE
		)
	{
		return getBlockAtInBoundaries(x, y, z);
	}

	int chX = X;
	int chY = Y;
	int chZ = Z;

	if (x < 0)
	{
		chX--;
		x = Settings::CHUNK_SIZE - 1;
	}
	else if (x >= Settings::CHUNK_SIZE)
	{
		chX++;
		x = 0;
	}
	if (y < 0)
	{
		chY--;
		y = Settings::CHUNK_SIZE - 1;
	}
	else if (y >= Settings::CHUNK_SIZE)
	{
		chY++;
		y = 0;
	}
	if (z < 0)
	{
		chZ--;
		z = Settings::CHUNK_SIZE - 1;
	}
	else if (z >= Settings::CHUNK_SIZE)
	{
		chZ++;
		z = 0;
	}

	const Chunk* chunk = getChunkAt(chX, chY, chZ);
	if (!chunk)
	{
		return Block::Void;
	}
	return chunk->getBlockAtInBoundaries(x, y, z);
}

bool Chunk::canSideBeSeen(const Camera& camera, size_t side) const
{
	if (side == 0)
	{
		return camera.position.x > X * Settings::CHUNK_SIZE + 1;
	}
	else if (side == 1)
	{
		return camera.position.x < (X + 1) * Settings::CHUNK_SIZE - 1;
	}
	else if (side == 2)
	{
		return camera.position.y > Y * Settings::CHUNK_SIZE + 1;
	}
	else if (side == 3)
	{
		return camera.position.y < (Y + 1) * Settings::CHUNK_SIZE - 1;
	}
	else if (side == 4)
	{
		return camera.position.z > Z * Settings::CHUNK_SIZE + 1;
	}
	else
	{
		return camera.position.z < (Z + 1) * Settings::CHUNK_SIZE - 1;
	}
}

uint8_t Chunk::getLightingAtInBoundaries(size_t x, size_t y, size_t z) const
{
	return lightingMap[getIndex(x, y, z)];
}

uint8_t Chunk::getLightingAt(int x, int y, int z) const
{
	if (
		x >= 0 && x < Settings::CHUNK_SIZE &&
		y >= 0 && y < Settings::CHUNK_SIZE &&
		z >= 0 && z < Settings::CHUNK_SIZE
		)
	{
		return getLightingAtInBoundaries(x, y, z);
	}

	int chX = X;
	int chY = Y;
	int chZ = Z;

	if (x < 0)
	{
		chX--;
	}
	else if (x >= Settings::CHUNK_SIZE)
	{
		chX++;
	}
	if (y < 0)
	{
		chY--;
	}
	else if (y >= Settings::CHUNK_SIZE)
	{
		chY++;
	}
	if (z < 0)
	{
		chZ--;
	}
	else if (z >= Settings::CHUNK_SIZE)
	{
		chZ++;
	}

	const Chunk* chunk = getChunkAt(chX, chY, chZ);
	if (!chunk)
	{
		return 0;
	}
	x &= Settings::CHUNK_SIZE - 1;
	y &= Settings::CHUNK_SIZE - 1;
	z &= Settings::CHUNK_SIZE - 1;
	return chunk->getLightingAtInBoundaries(x, y, z);
}

uint8_t Chunk::getLightingAtSideCheck(int x, int y, int z, size_t side) const
{
	if (
		x >= 0 && x < Settings::CHUNK_SIZE &&
		y >= 0 && y < Settings::CHUNK_SIZE &&
		z >= 0 && z < Settings::CHUNK_SIZE
		)
	{
		return getLightingAtInBoundaries(x, y, z);
	}
	const Chunk* chunk = neighbours[side];
	if (!chunk)
	{
		return 0;
	}
	x &= Settings::CHUNK_SIZE - 1;
	y &= Settings::CHUNK_SIZE - 1;
	z &= Settings::CHUNK_SIZE - 1;
	return chunk->getLightingAtInBoundaries(x, y, z);
}

void Chunk::setLightingAtInBoundaries(size_t x, size_t y, size_t z, uint8_t lightPower, bool lightOrSky)
{
	size_t index = getIndex(x, y, z);
	lightingMap[index] = (lightingMap[index] & (15 << (4 * !lightOrSky))) | (lightPower << (4 * lightOrSky));
}

void Chunk::setLightingAt(int x, int y, int z, uint8_t lightPower, bool lightOrSky)
{
	if (
		x >= 0 && x < Settings::CHUNK_SIZE &&
		y >= 0 && y < Settings::CHUNK_SIZE &&
		z >= 0 && z < Settings::CHUNK_SIZE
		)
	{
		setLightingAtInBoundaries(x, y, z, lightPower, lightOrSky);
	}

	int chX = X;
	int chY = Y;
	int chZ = Z;

	if (x < 0)
	{
		chX--;
	}
	else if (x >= Settings::CHUNK_SIZE)
	{
		chX++;
	}
	if (y < 0)
	{
		chY--;
	}
	else if (y >= Settings::CHUNK_SIZE)
	{
		chY++;
	}
	if (z < 0)
	{
		chZ--;
	}
	else if (z >= Settings::CHUNK_SIZE)
	{
		chZ++;
	}

	Chunk* chunk = getChunkAt(chX, chY, chZ);
	if (!chunk)
	{
		return;
	}
	x &= Settings::CHUNK_SIZE - 1;
	y &= Settings::CHUNK_SIZE - 1;
	z &= Settings::CHUNK_SIZE - 1;
	chunk->setLightingAtInBoundaries(x, y, z, lightPower, lightOrSky);
}

BlockAndLighting Chunk::getBlockAndLightingAtInBoundaries(size_t x, size_t y, size_t z) const
{
	size_t index = getIndex(x, y, z);
	return {blocks[index], lightingMap[index]};
}

BlockAndLighting Chunk::getBlockAndLightingAt(int x, int y, int z) const
{
	if (
		x >= 0 && x < Settings::CHUNK_SIZE &&
		y >= 0 && y < Settings::CHUNK_SIZE &&
		z >= 0 && z < Settings::CHUNK_SIZE
		)
	{
		return getBlockAndLightingAtInBoundaries(x, y, z);
	}

	int chX = X;
	int chY = Y;
	int chZ = Z;

	if (x < 0)
	{
		chX--;
	}
	else if (x >= Settings::CHUNK_SIZE)
	{
		chX++;
	}
	if (y < 0)
	{
		chY--;
	}
	else if (y >= Settings::CHUNK_SIZE)
	{
		chY++;
	}
	if (z < 0)
	{
		chZ--;
	}
	else if (z >= Settings::CHUNK_SIZE)
	{
		chZ++;
	}

	const Chunk* chunk = getChunkAt(chX, chY, chZ);
	if (!chunk)
	{
		return {Block::Void, 0};
	}
	x &= Settings::CHUNK_SIZE - 1;
	y &= Settings::CHUNK_SIZE - 1;
	z &= Settings::CHUNK_SIZE - 1;
	return chunk->getBlockAndLightingAtInBoundaries(x, y, z);
}

BlockAndLighting Chunk::getBlockAndLightingAtSideCheck(int x, int y, int z, size_t side) const
{
	if (
		x >= 0 && x < Settings::CHUNK_SIZE &&
		y >= 0 && y < Settings::CHUNK_SIZE &&
		z >= 0 && z < Settings::CHUNK_SIZE
		)
	{
		return getBlockAndLightingAtInBoundaries(x, y, z);
	}
	const Chunk* chunk = neighbours[side];
	if (!chunk)
	{
		return {Block::Void, 0};
	}
	x &= Settings::CHUNK_SIZE - 1;
	y &= Settings::CHUNK_SIZE - 1;
	z &= Settings::CHUNK_SIZE - 1;
	return chunk->getBlockAndLightingAtInBoundaries(x, y, z);
}

Block Chunk::getBlockAtSideCheck(int x, int y, int z, size_t side) const
{
	if (
		x >= 0 && x < Settings::CHUNK_SIZE &&
		y >= 0 && y < Settings::CHUNK_SIZE &&
		z >= 0 && z < Settings::CHUNK_SIZE
	)
	{
		return getBlockAtInBoundaries(x, y, z);
	}
	const Chunk* chunk = neighbours[side];
	if (!chunk)
	{
		return Block::Void;
	}
	x &= Settings::CHUNK_SIZE - 1;
	y &= Settings::CHUNK_SIZE - 1;
	z &= Settings::CHUNK_SIZE - 1;
	return chunk->getBlockAtInBoundaries(x, y, z);
}

int Chunk::posHash() const
{
	return pos3_hash(X, Y, Z);
}

std::string Chunk::getFilepath(int X, int Y, int Z)
{
	std::string path = Settings::chunkSavesPath;
	path += std::to_string(X);
	path += "_";
	path += std::to_string(Y);
	path += "_";
	path += std::to_string(Z);
	path += ".bin";
	return path;
}

DrawCommand::DrawCommand()
{}

void DrawCommand::resetFaces()
{
	memset(facesCount, 0, sizeof(facesCount));
}

bool DrawCommand::anyFaces() const
{
	return facesCount[0] || facesCount[1] || facesCount[2] || facesCount[3] || facesCount[4] || facesCount[5] || 
		facesCount[6] || facesCount[7] || facesCount[8] || facesCount[9] || facesCount[10] || facesCount[11];
}

PhysicEntityCollider::PhysicEntityCollider(glm::vec3& position, glm::vec3& size, glm::vec3& dpos) : position(position), size(size), dpos(dpos)
{}

bool Face::operator==(const Face& other) const
{
#if ENABLE_SMOOTH_LIGHTING
	return ao == other.ao && textureID == other.textureID && lighting == other.lighting &&
		smoothLighting[0] == other.smoothLighting[0] && smoothLighting[1] == other.smoothLighting[1] &&
		smoothLighting[2] == other.smoothLighting[2] && smoothLighting[3] == other.smoothLighting[3];
#else
	return ao == other.ao && textureID == other.textureID && lighting == other.lighting;
#endif
}

Light::Light() : pos(0, 0, 0), power(0), blockOrSky(false)
{}

Light::Light(int x, int y, int z, uint8_t lightPower, bool blockOrSky) : pos(x, y, z), power(lightPower), blockOrSky(blockOrSky)
{}

SizeT3::SizeT3(size_t x, size_t y, size_t z) : x(x), y(y), z(z)
{}

LightUpdate::LightUpdate(void* chunk, uint8_t x, uint8_t y, uint8_t z, Block block, Block prevBlock) : chunk(chunk), x(x), y(y), z(z), block(block), prevBlock(prevBlock)
{}
