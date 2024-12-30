#include "World.h"
#include <iostream>
#include <unordered_set>
#include "GraphicController.h"
#include "TerrainGenerator.h"
#include "Profiler.h"
#include <filesystem>
#include <fstream>
#include <algorithm>

#define _USE_MATH_DEFINES
#include <math.h>

struct QuadInstanceVertex
{
	glm::vec3 position;

	QuadInstanceVertex(const glm::vec3& pos) : position(pos)
	{}

	QuadInstanceVertex(float x, float y, float z) : position(x, y, z)
	{}
};

const QuadInstanceVertex quadInstanceVertices[4] =
{
	{0.0f, 0.0f, 1.0f},
	{1.0f, 0.0f, 1.0f},
	{1.0f, 0.0f, 0.0f},
	{0.0f, 0.0f, 0.0f}
};

template <typename T> int signum(T val) 
{
	return (T(0) < val) - (val < T(0));
}

World::ChunkDistance::ChunkDistance(Chunk* chunk, float distance) : chunk(chunk), distance(distance) {}

static float intbound(float s, float ds)
{
	if (ds < 0.0f)
	{
		return intbound(-s, -ds);
	}
	else
	{
		s = fmodf(s, 1.0f);
		return (1.0f - s) / ds;
	}
}

Chunk* World::getChunk(int x, int y, int z)
{
	std::lock_guard<std::mutex> lock(chunkPoolMutex);
	Chunk* ret = chunkPool.acquire();
	// TODO: remove
	if (ret->state != Chunk::State::NotLoaded)
	{
		std::cerr << "GetChunk: " << toString(ret->state) << std::endl;
	}
	ret->init(x, y, z);
	return ret;
}

void World::releaseChunk(Chunk* chunk, bool returnDrawIdToPool = true)
{
	if (chunk->state == Chunk::State::InLoadingQueue)
	{
		{
			std::lock_guard<std::mutex> lock(generateChunkVectorMutex);
			size_t count = chunkGenerateVector.size();
			for (size_t i = 0; i < count; i++)
			{
				auto& item = chunkGenerateVector[i];
				if (item == chunk)
				{
					chunkGenerateVector[i] = chunkGenerateVector.back();
					chunkGenerateVector.pop_back();
					break;
				}
			}
		}
		chunk->state = Chunk::State::NotLoaded;
		{
			std::lock_guard<std::mutex> lock(chunkPoolMutex);
			chunkPool.release(chunk);
		}
	}
	else if (chunk->state == Chunk::State::Loaded)
	{
		chunk->destroy();
		if (returnDrawIdToPool)
		{
			// TODO: switch to pool class
			std::lock_guard<std::mutex> lock(chunkIDPoolMutex);
			chunkIDPool[++chunkIDPoolIndex] = chunk->drawCommand.offset / Settings::FACE_INSTANCES_PER_CHUNK;
		}
		chunk->state = Chunk::State::NotLoaded;
		{
			std::lock_guard<std::mutex> lock(chunkPoolMutex);
			chunkPool.release(chunk);
		}
	}
	else if (chunk->state == Chunk::State::Loading)
	{
		std::cerr << "Chunk was added to releasedLoadingChunks" << std::endl;
		std::lock_guard<std::mutex> lock(releasedLoadingChunksMutex);
		releasedLoadingChunks.push_back(chunk);
	}
}

World::World(const WorldData& worldData)
	: lastChunkLoaderPosition{0.0f, 0.0f, 0.0f},

	quadInstanceVBO((const char*)quadInstanceVertices, 4 * sizeof(QuadInstanceVertex), GL_STATIC_DRAW),
	quadInstanceVAO(),

	indirectBuffer(Settings::MAX_CHUNK_DRAW_COMMANDS_COUNT),

	chunkPositionSSBO(Settings::MAX_RENDERED_CHUNKS_COUNT * sizeof(glm::vec3)),
	chunkPositionIndexSSBO(Settings::MAX_CHUNK_DRAW_COMMANDS_COUNT * sizeof(unsigned int)),

	time(worldData.worldTime),

	blockTextures("res/Textures.png", 0, Settings::BLOCK_TEXTURE_SIZE, Settings::BLOCK_TEXTURES_IN_ROW, Settings::BLOCK_TEXTURES_COUNT, Settings::BLOCK_TEXTURES_NUM_CHANNELS, GL_REPEAT, true),
	numberTextures("res/Numbers.png", 1, 8, 4, 16, 1, GL_CLAMP_TO_BORDER, false),

	threadPool(4),
	chunkPool(Settings::MAX_RENDERED_CHUNKS_COUNT)
{
	chunkIDPool = new unsigned int[Settings::MAX_RENDERED_CHUNKS_COUNT];
	for (size_t i = 0; i < Settings::MAX_RENDERED_CHUNKS_COUNT; i++)
	{
		chunkIDPool[i] = i;
	}
	chunkIDPoolIndex = Settings::MAX_RENDERED_CHUNKS_COUNT - 1;

	GraphicController::chunkProgram->bind();
	GraphicController::chunkProgram->setUniformInt("diffuse0", 0);
	GraphicController::chunkProgram->setUniformInt("numbers", 1);

	//
	quadInstanceVAO.linkFloat(3, sizeof(QuadInstanceVertex));
	Chunk::faceInstancesVBO = new FaceInstancesVBO(Settings::MAX_RENDERED_CHUNKS_COUNT * Settings::FACE_INSTANCES_PER_CHUNK, quadInstanceVAO.getLayout());
	VAO::unbind();

	Chunk::facesData = new Face[Settings::CHUNK_SIZE_CUBED * 6];
	Chunk::faceInstancesData = new FaceInstanceData[Settings::FACE_INSTANCES_PER_CHUNK];

	chunkPositionSSBO.bindBase(0);
	chunkPositionIndexSSBO.bindBase(1);

	//
	TerrainGenerator::init(worldData.seed);

	drawCommands = new DrawArraysIndirectCommand[Settings::MAX_CHUNK_DRAW_COMMANDS_COUNT];
	for (size_t i = 0; i < Settings::MAX_CHUNK_DRAW_COMMANDS_COUNT; i++)
	{
		DrawArraysIndirectCommand& command = drawCommands[i];
		command.count = 4;
		command.first = 0;
	}

	chunkPositions = new glm::vec3[Settings::MAX_RENDERED_CHUNKS_COUNT];
	chunkPositionIndexes = new unsigned int[Settings::MAX_CHUNK_DRAW_COMMANDS_COUNT];

	//
	if (!std::filesystem::exists(Settings::chunkSavesPath))
	{
		std::filesystem::create_directories(Settings::chunkSavesPath);
	}
}

World::~World()
{
	// shutdown threads
	threadPool.destroy();
	
	//
	quadInstanceVBO.clean();
	quadInstanceVAO.clean();
	indirectBuffer.clean();
	chunkPositionSSBO.clean();
	chunkPositionIndexSSBO.clean();
	blockTextures.clean();
	numberTextures.clean();

	Chunk::faceInstancesVBO->clean();
	delete Chunk::faceInstancesVBO;
	delete[] Chunk::faceInstancesData;
	delete[] Chunk::facesData;
	
	//
	for (const auto& it : Chunk::chunkMap)
	{
		Chunk* chunk = it.second;
		chunk->destroy();
		delete chunk;
	}
	Chunk::chunkMap.clear();

	delete[] chunkIDPool;
	delete[] drawCommands;
	delete[] chunkPositions;
	delete[] chunkPositionIndexes;

	TerrainGenerator::clear();
}

void World::update(const glm::vec3& pos, bool isMoving)
{
	// SaveDataChunks
	// TODO: maybe it is need a mutex
	if (!temporalSaveDataChunks.empty())
	{
		for (const auto& pos : temporalSaveDataChunks)
		{
			auto it = temporalChunkBlockChanges.find(pos3_hash(pos.x, pos.y, pos.z));

			if (it == temporalChunkBlockChanges.end())
			{
				std::cout << "Temporal SaveDataChunks failed" << std::endl;
				continue;
			}
			std::unordered_map<Block, Vector<uint16_t, Settings::CHUNK_SIZE_CUBED>> blockChanges = it->second;
			Chunk::saveData(blockChanges, pos.x, pos.y, pos.z);
		}
		temporalSaveDataChunks.clear();
	}
	temporalChunkBlockChanges.clear();

	// released loading chunks
	if (!releasedLoadingChunks.empty())
	{
		std::cout << releasedLoadingChunks.size() << std::endl;
		std::lock_guard<std::mutex> lock(releasedLoadingChunksMutex);
		for (auto it = releasedLoadingChunks.begin(); it != releasedLoadingChunks.end();)
		{
			Chunk* chunk = *it;
			if (chunk->state == Chunk::State::Loaded)
			{
				releaseChunk(chunk, false);
				it = releasedLoadingChunks.erase(it);
			}
			else
			{
				++it;
			}
		}
	}

	// load chunks
	chunkLoaderPosition = glm::ivec3(pos / (float)Settings::CHUNK_SIZE);
	Profiler::start(LOAD_CHUNKS_INDEX);
	loadChunks(false);
	Profiler::end(LOAD_CHUNKS_INDEX);
	
	// generate blocks
	generateChunksBlocks(pos, isMoving);

	// update lighting
	updateLighting();

	// generate faces
	generateChunksFaces();

	// day night cycle
	time++;
	if (time >= 24000)
	{
		time = 0;
	}
	float angle = (float)time / 24000.0f * 2.0f * (float)M_PI;
	GraphicController::chunkProgram->bind();
	GraphicController::chunkProgram->setUniformFloat("dayNightCycleSkyLightingSubtraction", (cosf(angle) + 1.0f) * 0.5f);

	// shrinking vectors
	dataShrinkingTick++;
	if (dataShrinkingTick > 20)
	{
		dataShrinkingTick = 0;

		{
			std::lock_guard<std::mutex> lock(Chunk::lightingUpdateMutex);
			Chunk::lightingUpdateVector.shrink_to_fit();
		}
		{
			std::lock_guard<std::mutex> lock(Chunk::lightingFloodFillMutex);
			Chunk::lightingFloodFillVector.shrink_to_fit();
		}
		{
			std::lock_guard<std::mutex> lock(Chunk::darknessFloodFillMutex);
			Chunk::darknessFloodFillVector.shrink_to_fit();
		}
		{
			std::lock_guard<std::mutex> lock(generateChunkVectorMutex);
			chunkGenerateVector.shrink_to_fit();
		}
	}
}

void World::generateChunksBlocks(const glm::vec3& pos, bool isMoving)
{
	std::lock_guard<std::mutex> lock(generateChunkVectorMutex);
	size_t chunksCount = chunkGenerateVector.size();
	if (chunksCount == 0)
	{
		return;
	}
	size_t generateCount = std::min(chunksCount, size_t(isMoving ? Settings::DynamicSettings::generateChunksPerTickMoving : Settings::DynamicSettings::generateChunksPerTickStationary));

	// generate
	size_t range = chunksCount - generateCount;

	std::vector<std::function<void()>> tasks;
	tasks.reserve(generateCount);
	{
		for (size_t i = range; i < chunksCount; i++)
		{
			Chunk* chunk = chunkGenerateVector[i];
			// TODO: remove
			if (chunk->state != Chunk::State::InLoadingQueue)
			{
				std::cerr << "GenerateChunksBlocks: " << toString(chunk->state) << std::endl;
				continue;
			}
			tasks.push_back([this, chunk]() {
				generateChunkBlocksThread(chunk);
							});
		}
	}
	threadPool.addTasks(tasks);

	threadPool.waitForCompletion(); // TODO: remove and fix errors
	chunkGenerateVector.resize(range);
}

void World::generateChunkBlocksThread(Chunk* chunk)
{
	if (getSquaredDistanceToChunkLoader(glm::vec3(chunk->X, chunk->Y, chunk->Z)) > Settings::CHUNK_LOAD_RADIUS * Settings::CHUNK_LOAD_RADIUS)
	{
		releaseChunk(chunk);
		return;
	}
	
	chunk->generateBlocks();
	// TODO: remove
	if (chunk->state != Chunk::State::Loaded)
	{
		std::cerr << "GenerateChunkBlocksThread: " << toString(chunk->state) << std::endl;
	}
	
	if (getSquaredDistanceToChunkLoader(glm::vec3(chunk->X, chunk->Y, chunk->Z)) > Settings::CHUNK_LOAD_RADIUS * Settings::CHUNK_LOAD_RADIUS)
	{
		releaseChunk(chunk);
		return;
	}

	{
		std::lock_guard<std::mutex> lock(chunkIDPoolMutex);
		chunk->setDrawID(chunkIDPool[chunkIDPoolIndex]);
		chunkIDPool[chunkIDPoolIndex] = 0;
		chunkIDPoolIndex--;
	}

	addChunkToGenerateFaces(chunk);
	addSurroundingChunksToGenerateFaces(chunk);
}

void World::sortGenerateChunksQueue()
{
	// sort chunks in ascending order by distance
	// later chunks will be generated from back to front
	std::lock_guard<std::mutex> lock(generateChunkVectorMutex);
	const size_t chunksCount = chunkGenerateVector.size();
	std::vector<ChunkDistance> chunkDistances;
	chunkDistances.reserve(chunksCount);

	for (Chunk* chunk : chunkGenerateVector)
	{
		auto dpos = glm::vec3(chunk->X, chunk->Y, chunk->Z) - glm::vec3(chunkLoaderPosition);
		float distance = glm::dot(dpos, dpos);
		chunkDistances.emplace_back(chunk, distance);
	}

	std::sort(chunkDistances.begin(), chunkDistances.end(), [&](const ChunkDistance& a, const ChunkDistance& b)
			  {
				  return a.distance > b.distance;
			  });

	for (size_t i = 0; i < chunksCount; i++)
	{
		chunkGenerateVector[i] = chunkDistances[i].chunk;
	}
}

void World::generateChunksFaces()
{
	std::lock_guard<std::mutex> lock(generateFacesSetMutex);
	if (generateFacesSet.empty())
	{
		return;
	}
	Chunk::faceInstancesVBO->bind();

	for (Chunk* chunk : generateFacesSet)
	{
		chunk->generateFaces();
	}
	generateFacesSet.clear();
}

RaycastHit World::raycast(const glm::vec3& startPos, const glm::vec3& dir, float length)
{
	RaycastHit hit;

	glm::vec3 endPos = startPos + dir * length;

	glm::ivec3 currentVoxelPos = glm::floor(startPos);
	int stepDir = -1;

	glm::vec3 deltaPos = endPos - startPos;
	glm::vec3 step = glm::sign(dir);

	glm::vec3 tDelta =
	{
		step.x == 0 ? 1000000 : fminf(step.x / deltaPos.x, 1000000),
		step.y == 0 ? 1000000 : fminf(step.y / deltaPos.y, 1000000),
		step.z == 0 ? 1000000 : fminf(step.z / deltaPos.z, 1000000)
	};

	glm::vec3 tMax =
	{
		step.x > 0 ? tDelta.x * (1.0f - glm::fract(startPos.x)) : tDelta.x * glm::fract(startPos.x),
		step.y > 0 ? tDelta.y * (1.0f - glm::fract(startPos.y)) : tDelta.y * glm::fract(startPos.y),
		step.z > 0 ? tDelta.z * (1.0f - glm::fract(startPos.z)) : tDelta.z * glm::fract(startPos.z)
	};

	glm::ivec3 prevChunkPos = currentVoxelPos - 1;
	Chunk* chunk = nullptr;

	// TODO: implement raycast if we arent in chunk
	while (!(tMax.x > 1.0f && tMax.y > 1.0f && tMax.z > 1.0f))
	{
		glm::ivec3 chunkPos = glm::floor(glm::vec3(currentVoxelPos) / (float)Settings::CHUNK_SIZE);
		if (chunkPos != prevChunkPos)
		{
			prevChunkPos = chunkPos;
			chunk = Chunk::getChunkAt
			(
				chunkPos.x,
				chunkPos.y,
				chunkPos.z
			);
			if (!chunk)
			{
				return hit;
			}
		}

		size_t x = currentVoxelPos.x & (Settings::CHUNK_SIZE - 1);
		size_t y = currentVoxelPos.y & (Settings::CHUNK_SIZE - 1);
		size_t z = currentVoxelPos.z & (Settings::CHUNK_SIZE - 1);
		
		Block block = chunk->getBlockAtInBoundaries(x, y, z);

		if (ALL_BLOCK_DATA[(size_t)block].createFaces)
		{
			hit.hit = true;
			hit.globalPos = currentVoxelPos;
			hit.normal[stepDir] = -step[stepDir];
			hit.block = block;
			return hit;
		}

		if (tMax.x < tMax.y)
		{
			if (tMax.x < tMax.z)
			{
				currentVoxelPos.x += step.x;
				tMax.x += tDelta.x;
				stepDir = 0;
			}
			else
			{
				currentVoxelPos.z += step.z;
				tMax.z += tDelta.z;
				stepDir = 2;
			}
		}
		else
		{
			if (tMax.y < tMax.z)
			{
				currentVoxelPos.y += step.y;
				tMax.y += tDelta.y;
				stepDir = 1;
			}
			else
			{
				currentVoxelPos.z += step.z;
				tMax.z += tDelta.z;
				stepDir = 2;
			}
		}
	}

	return hit;
}

void World::setBlockAt(int x, int y, int z, Block block)
{
	// get chunk
	int chX = floorf((float)x / (float)Settings::CHUNK_SIZE);
	int chY = floorf((float)y / (float)Settings::CHUNK_SIZE);
	int chZ = floorf((float)z / (float)Settings::CHUNK_SIZE);

	Chunk* chunk = Chunk::getChunkAt(chX, chY, chZ);

	if (chunk)
	{
		// check for entity collision
		if (block != Block::Air)
		{
			bool collides = false;
			glm::vec3 voxelCenter = glm::vec3(x, y, z) + 0.5f;
			const Chunk* chunksToCheck[7] = { chunk };
			memcpy(chunksToCheck + 1, chunk->neighbours, sizeof(chunk->neighbours));
			for (const Chunk* checkChunk : chunksToCheck)
			{
				if (!checkChunk)
				{
					continue;
				}
				for (const auto& collider : checkChunk->physicEntities)
				{
					glm::vec3 dpos = glm::abs(collider->position - voxelCenter);
					if ((dpos.x < collider->size.x + 0.5f) && (dpos.y < collider->size.y + 0.5f) && (dpos.z < collider->size.z + 0.5f))
					{
						collides = true;
						break;
					}
				}
				if (collides)
				{
					return;
				}
			}
		}

		// place block
		x &= Settings::CHUNK_SIZE - 1;
		y &= Settings::CHUNK_SIZE - 1;
		z &= Settings::CHUNK_SIZE - 1;

		const BlockData& blockData = ALL_BLOCK_DATA[(size_t)block];

		if (chunk->setBlockAtInBoundaries(x, y, z, block))
		{
			for (int dx = -1; dx <= 1; dx++)
			{
				for (int dy = -1; dy <= 1; dy++)
				{
					for (int dz = -1; dz <= 1; dz++)
					{
						Chunk* chunk = Chunk::getChunkAt(chX + dx, chY + dy, chZ + dz);
						if (chunk)
						{
							addChunkToGenerateFaces(chunk);
						}
					}
				}
			}
		}
	}
	else
	{
		// place block
		x &= Settings::CHUNK_SIZE - 1;
		y &= Settings::CHUNK_SIZE - 1;
		z &= Settings::CHUNK_SIZE - 1;

		std::unordered_map<Block, Vector<uint16_t, Settings::CHUNK_SIZE_CUBED>> blockChanges;

		auto it = temporalChunkBlockChanges.find(pos3_hash(chX, chY, chZ));
		if (it == temporalChunkBlockChanges.end())
		{
			Chunk::loadData(blockChanges, chX, chY, chZ);
			temporalChunkBlockChanges[pos3_hash(chX, chY, chZ)] = blockChanges;
		}
		else
		{
			blockChanges = it->second;
		}


		uint16_t placeBlockIndex = Chunk::getIndex(x, y, z);

		bool push = true;
		for (auto& pair : blockChanges)
		{
			Block block_ = pair.first;
			auto& vector = pair.second;

			bool stop = false;
			size_t size = vector.getSize();
			for (size_t i = 0; i < size; i++)
			{
				uint16_t index = vector[i];
				if (index == placeBlockIndex)
				{
					if (block != block_)
					{
						vector.pop(i);
					}
					else
					{
						push = false;
					}
					stop = true;
					break;
				}

				/*const auto& it = blockChanges.find(prevBlock);
				if (it != blockChanges.end())
				{
					auto& prevVec = it->second;

					prevVec.remove(saveIndex);
					if (prevVec.getSize() == 0)
					{
						blockChanges.erase(it);
					}
				}

				blockChanges[block].push(saveIndex);*/
			}
			if (stop)
			{
				break;
			}
		}
		if (push)
		{
			blockChanges[block].push(placeBlockIndex);
			temporalChunkBlockChanges[pos3_hash(chX, chY, chZ)] = blockChanges;
			temporalSaveDataChunks.emplace(chX, chY, chZ);
		}
	}
}

Block World::getBlockAt(int x, int y, int z) const
{
	int chX = floorf((float)x / Settings::CHUNK_SIZE);
	int chY = floorf((float)y / Settings::CHUNK_SIZE);
	int chZ = floorf((float)z / Settings::CHUNK_SIZE);

	Chunk* chunk = Chunk::getChunkAt(chX, chY, chZ);
	if (!chunk)
	{
		return Block::Void;
	}

	x &= Settings::CHUNK_SIZE - 1;
	y &= Settings::CHUNK_SIZE - 1;
	z &= Settings::CHUNK_SIZE - 1;

	return chunk->getBlockAtInBoundaries(x, y, z);
}

float World::getDistanceToChunkLoader(const glm::vec3& chunkPos) const
{
	return glm::distance(chunkPos, glm::vec3(chunkLoaderPosition));
}

float World::getSquaredDistanceToChunkLoader(const glm::vec3& chunkPos) const
{
	glm::vec3 dpos = chunkPos - glm::vec3(chunkLoaderPosition);
	return glm::dot(dpos, dpos);
}

bool World::loadChunks(bool forced)
{
	if (!forced && chunkLoaderPosition == lastChunkLoaderPosition)
	{
		return false;
	}
	lastChunkLoaderPosition = chunkLoaderPosition;
	{
		int radius = Settings::CHUNK_LOAD_RADIUS;
		int rsq = radius * radius;
		std::lock_guard<std::mutex> lock(chunkMapMutex);

		// unload chunks
		for (auto it = Chunk::chunkMap.begin(); it != Chunk::chunkMap.end();)
		{
			Chunk* chunk = it->second;
			if (chunk->state != Chunk::State::Loaded && chunk->state != Chunk::State::InLoadingQueue)
			{
				it++;
				continue;
			}

			int dx = lastChunkLoaderPosition.x - chunk->X;
			int dy = lastChunkLoaderPosition.y - chunk->Y;
			int dz = lastChunkLoaderPosition.z - chunk->Z;
			int D1 = dx * dx + dz * dz;

			if (D1 > rsq)
			{
				TerrainGenerator::unloadHeightMap(chunk->X, chunk->Z);
				it = Chunk::chunkMap.erase(it);
				releaseChunk(chunk);
			}
			else if (D1 + dy * dy > rsq)
			{
				it = Chunk::chunkMap.erase(it);
				releaseChunk(chunk);
			}
			else
			{
				it++;
			}
		}

		// load chunks
		for (int dx = -radius; dx <= radius; dx++)
		{
			int D1 = rsq - dx * dx;
			int maxZ = (int)sqrtf(D1);
			for (int dz = -maxZ; dz <= maxZ; dz++)
			{
				TerrainGenerator::loadHeightMap(chunkLoaderPosition.x + dx, chunkLoaderPosition.z + dz);

				int D2 = D1 - dz * dz;
				int maxY = (int)sqrtf(D2);
				for (int dy = -maxY; dy <= maxY; dy++)
				{
					Chunk* chunk = Chunk::getChunkAt(chunkLoaderPosition.x + dx, chunkLoaderPosition.y + dy, chunkLoaderPosition.z + dz);
					if (chunk)
					{
						continue;
					}

					chunk = getChunk(chunkLoaderPosition.x + dx, chunkLoaderPosition.y + dy, chunkLoaderPosition.z + dz);
					// TODO: remove
					if (chunk->state != Chunk::State::NotLoaded)
					{
						std::cerr << std::format("Chunk state mismatch in loadChunks. State: {}, should be: {}", toString(chunk->state), toString(Chunk::State::NotLoaded)) << std::endl;
					}
					{
						chunk->state = Chunk::State::InLoadingQueue;
						std::lock_guard<std::mutex> lock(generateChunkVectorMutex);
						chunkGenerateVector.push_back(chunk);
					}
				}
			}
		}
	}

	sortGenerateChunksQueue();

	return true;
}

void World::draw(const Camera& camera)
{
	drawCommandsCount = 0;

	// get render chunks
	std::vector<ChunkDistance> renderChunks;
	getRenderChunks(renderChunks, camera);

	// sort render chunks
	std::sort(renderChunks.begin(), renderChunks.end(), [&](const ChunkDistance& a, const ChunkDistance& b)
	{
		return a.distance < b.distance;
	});
	
	// draw solid faces
	blockTextures.bind();
	numberTextures.bind();

	size_t commandsCount, chunkPositionsCount;
	getDrawCommands(renderChunks, camera, commandsCount, chunkPositionsCount, false);
	drawCommandsCount += commandsCount;
	glEnable(GL_DEPTH_TEST);
	glDisable(GL_BLEND);
	if (commandsCount > 0)
	{
		chunkPositionSSBO.setData((const char*)chunkPositions, chunkPositionsCount * sizeof(glm::vec3));
		chunkPositionIndexSSBO.setData((const char*)chunkPositionIndexes, commandsCount * sizeof(unsigned int));
		indirectBuffer.setData(drawCommands, commandsCount);

		quadInstanceVAO.bind();

		glEnable(GL_CULL_FACE);
		glDepthFunc(GL_LESS);

		if (GraphicController::zPrePass)
		{
			GraphicController::deferredChunkProgram->bind();
			glEnable(GL_RASTERIZER_DISCARD);
			glMultiDrawArraysIndirect(GL_TRIANGLE_FAN, nullptr, commandsCount, 0);

			GraphicController::chunkProgram->bind();
			glDisable(GL_RASTERIZER_DISCARD);
			glDepthFunc(GL_LEQUAL);
			glMultiDrawArraysIndirect(GL_TRIANGLE_FAN, nullptr, commandsCount, 0);
		}
		else
		{
			GraphicController::chunkProgram->bind();
			glMultiDrawArraysIndirect(GL_TRIANGLE_FAN, nullptr, commandsCount, 0);
		}
	}
	
	// draw transparent
	std::reverse(renderChunks.begin(), renderChunks.end());
	getDrawCommands(renderChunks, camera, commandsCount, chunkPositionsCount, true);
	drawCommandsCount += commandsCount;
	glDisable(GL_CULL_FACE);
	if (commandsCount > 0)
	{
		chunkPositionSSBO.setData((const char*)chunkPositions, chunkPositionsCount * sizeof(glm::vec3));
		chunkPositionIndexSSBO.setData((const char*)chunkPositionIndexes, commandsCount * sizeof(unsigned int));
		indirectBuffer.setData(drawCommands, commandsCount);
	
		quadInstanceVAO.bind();

		if (GraphicController::zPrePass)
		{
			GraphicController::deferredChunkProgram->bind();
		}
		else
		{
			GraphicController::chunkProgram->bind();
		}
		glEnable(GL_RASTERIZER_DISCARD);
		glDepthFunc(GL_LESS);
		glMultiDrawArraysIndirect(GL_TRIANGLE_FAN, nullptr, commandsCount, 0);

		if (GraphicController::zPrePass)
		{
			GraphicController::chunkProgram->bind();
		}
		glDisable(GL_RASTERIZER_DISCARD);
		glDepthFunc(GL_LEQUAL);
		glEnable(GL_BLEND);
		glMultiDrawArraysIndirect(GL_TRIANGLE_FAN, nullptr, commandsCount, 0);
	}
}

void World::regenerateChunks()
{
	std::lock_guard<std::mutex> lock(generateChunkVectorMutex);
	chunkGenerateVector.clear();
	chunkGenerateVector.reserve(Chunk::chunkMap.size());
	for (const auto& pair : Chunk::chunkMap)
	{
		Chunk* chunk = pair.second;
		if (chunk->state != Chunk::State::Loaded)
		{
			continue;
		}
		addChunkToGenerateFaces(chunk);
	}
}

void World::getRenderChunks(std::vector<ChunkDistance>& renderChunks, const Camera& camera) const
{
	Box chunkShape(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(Settings::HALF_CHUNK_SIZE));
	renderChunks.reserve(Settings::MAX_RENDERED_CHUNKS_COUNT >> 2);
	for (const auto& pair : Chunk::chunkMap)
	{
		Chunk* chunk = pair.second;
		if (!chunk->hasAnyFaces)
		{
			continue;
		}

		chunkShape.center.x = (chunk->X + 0.5f) * Settings::CHUNK_SIZE;
		chunkShape.center.y = (chunk->Y + 0.5f) * Settings::CHUNK_SIZE;
		chunkShape.center.z = (chunk->Z + 0.5f) * Settings::CHUNK_SIZE;
		if (!camera.isOnFrustum(chunkShape))
		{
			continue;
		}
		auto dpos = chunkShape.center - camera.position;
		float distance = glm::dot(dpos, dpos);
		renderChunks.emplace_back(chunk, distance);
	}
}

void World::getDrawCommands(const std::vector<ChunkDistance>& renderChunks, const Camera& camera, size_t& commandsCount, size_t& positionsCount, bool transparent)
{
	commandsCount = 0;
	positionsCount = 0;

	size_t normalOffset = transparent ? 6 : 0;
	for (const ChunkDistance& pair : renderChunks)
	{
		const Chunk* chunk = pair.chunk;
		float X = chunk->X * Settings::CHUNK_SIZE;
		float Y = chunk->Y * Settings::CHUNK_SIZE;
		float Z = chunk->Z * Settings::CHUNK_SIZE;

		const DrawCommand& command = chunk->drawCommand;
		bool anyFace = false;
		for (size_t normalID = 0; normalID < 6; normalID++)
		{
			GLuint facesCount = command.facesCount[normalID + normalOffset];
			if (facesCount > 0 && chunk->canSideBeSeen(camera.position, normalID))
			{
				size_t index = commandsCount++;
				chunkPositionIndexes[index] = positionsCount;
				anyFace = true;

				DrawArraysIndirectCommand& indirectCommand = drawCommands[index];
				indirectCommand.instancesCount = facesCount;

				if (transparent)
				{
					indirectCommand.baseInstance = command.offset + (normalID + 1) * (Settings::FACE_INSTANCES_PER_CHUNK / 6) - facesCount;
				}
				else
				{
					indirectCommand.baseInstance = command.offset + normalID * (Settings::FACE_INSTANCES_PER_CHUNK / 6);
				}
			}
		}
		if (anyFace)
		{
			chunkPositions[positionsCount] = { X, Y, Z };
			positionsCount++;
		}
	}
}

void World::addChunkToGenerateFaces(Chunk* chunk)
{
	if (chunk->state == Chunk::State::Loaded)
	{
		std::lock_guard<std::mutex> lock(generateFacesSetMutex);
		generateFacesSet.emplace(chunk);
	}
}

void World::addSurroundingChunksToGenerateFaces(const Chunk* chunk)
{
	int chX = chunk->X;
	int chY = chunk->Y;
	int chZ = chunk->Z;
	for (int dx = -1; dx <= 1; dx += 2)
	{
		for (int dy = -1; dy <= 1; dy += 2)
		{
			for (int dz = -1; dz <= 1; dz += 2)
			{
				Chunk* chunk;
				{
					std::lock_guard<std::mutex> lock(chunkMapMutex);
					chunk = Chunk::getChunkAt(chX + dx, chY + dy, chZ + dz);
				}
				if (chunk)
				{
					addChunkToGenerateFaces(chunk);
				}
			}
		}
	}

	for (Chunk* neighbour : chunk->neighbours)
	{
		if (neighbour)
		{
			addChunkToGenerateFaces(neighbour);
		}
	}
}

uint8_t World::getLightingAt(int x, int y, int z) const
{
	int chX = floorf((float)x / Settings::CHUNK_SIZE);
	int chY = floorf((float)y / Settings::CHUNK_SIZE);
	int chZ = floorf((float)z / Settings::CHUNK_SIZE);

	Chunk* chunk = Chunk::getChunkAt(chX, chY, chZ);
	if (!chunk)
	{
		return 0;
	}

	x &= Settings::CHUNK_SIZE - 1;
	y &= Settings::CHUNK_SIZE - 1;
	z &= Settings::CHUNK_SIZE - 1;

	return chunk->getLightingAtInBoundaries(x, y, z);
}

void World::setLightingAt(int x, int y, int z, uint8_t power, bool lightOrSky)
{
	int chX = floorf((float)x / Settings::CHUNK_SIZE);
	int chY = floorf((float)y / Settings::CHUNK_SIZE);
	int chZ = floorf((float)z / Settings::CHUNK_SIZE);

	Chunk* chunk = Chunk::getChunkAt(chX, chY, chZ);
	if (!chunk)
	{
		return;
	}

	x &= Settings::CHUNK_SIZE - 1;
	y &= Settings::CHUNK_SIZE - 1;
	z &= Settings::CHUNK_SIZE - 1;

	chunk->setLightingAtInBoundaries(x, y, z, power, lightOrSky);
	addChunkToGenerateFaces(chunk);
}

void World::lightingFloodFill()
{
	// TODO: maybe use queue?
	// TODO: if not loaded, wait for loading
	std::lock_guard<std::mutex> lock(Chunk::lightingFloodFillMutex);
	auto& needToCheck = Chunk::lightingFloodFillVector;
	while (!needToCheck.empty())
	{
		auto light = needToCheck.back();
		needToCheck.pop_back();

		int globalX = light.pos.x;
		int globalY = light.pos.y;
		int globalZ = light.pos.z;

		int chX = floorf((float)globalX / Settings::CHUNK_SIZE);
		int chY = floorf((float)globalY / Settings::CHUNK_SIZE);
		int chZ = floorf((float)globalZ / Settings::CHUNK_SIZE);
		Chunk* chunk = Chunk::getChunkAt(chX, chY, chZ);
		if (!chunk || chunk->state != Chunk::State::Loaded)
		{
			continue;
		}

		bool blockOrSky = light.blockOrSky;

		int chunkGlobalX = chX * Settings::CHUNK_SIZE;
		int chunkGlobalY = chY * Settings::CHUNK_SIZE;
		int chunkGlobalZ = chZ * Settings::CHUNK_SIZE;

		int x = globalX & (Settings::CHUNK_SIZE - 1);
		int y = globalY & (Settings::CHUNK_SIZE - 1);
		int z = globalZ & (Settings::CHUNK_SIZE - 1);

		uint8_t currentLightLevel = (chunk->getLightingAtInBoundaries(x, y, z) >> (4 * blockOrSky)) & 15;
		if (currentLightLevel <= 1)
		{
			continue;
		}
		for (size_t side = 0; side < 6; side++)
		{
			size_t axis = side >> 1;
			int offCoords[3] = { (int)x, (int)y, (int)z };
			offCoords[axis] += (side & 1) ? -1 : 1;

			Chunk::BlockAndLighting blockAndLighting = chunk->getBlockAndLightingAtSideCheck(offCoords[0], offCoords[1], offCoords[2], side);
			if (blockAndLighting.block == Block::Void)
			{
				continue;
			}

			const BlockData& blockData = ALL_BLOCK_DATA[(size_t)blockAndLighting.block];
			uint8_t lightLevel = (blockAndLighting.lighting >> (4 * blockOrSky)) & 15;
			if (blockData.transparent && lightLevel + 1 < currentLightLevel)
			{
				chunk->setLightingAtSideCheck(offCoords[0], offCoords[1], offCoords[2], side, currentLightLevel - 1, blockOrSky);
				needToCheck.emplace_back
				(
					chunkGlobalX + offCoords[0],
					chunkGlobalY + offCoords[1],
					chunkGlobalZ + offCoords[2],
					blockOrSky
				);
			}
		}
	}
}

void World::darknessFloodFill()
{
	std::lock_guard<std::mutex> lock(Chunk::darknessFloodFillMutex);
	auto& needToCheck = Chunk::darknessFloodFillVector;
	while (!needToCheck.empty())
	{
		auto darkness = needToCheck.back();
		needToCheck.pop_back();

		int globalX = darkness.pos.x;
		int globalY = darkness.pos.y;
		int globalZ = darkness.pos.z;
		bool blockOrSky = darkness.blockOrSky;

		int chX = floorf((float)globalX / Settings::CHUNK_SIZE);
		int chY = floorf((float)globalY / Settings::CHUNK_SIZE);
		int chZ = floorf((float)globalZ / Settings::CHUNK_SIZE);
		Chunk* chunk = Chunk::getChunkAt(chX, chY, chZ);
		if (!chunk || chunk->state != Chunk::State::Loaded)
		{
			continue;
		}

		int chunkGlobalX = chX * Settings::CHUNK_SIZE;
		int chunkGlobalY = chY * Settings::CHUNK_SIZE;
		int chunkGlobalZ = chZ * Settings::CHUNK_SIZE;

		int x = globalX & (Settings::CHUNK_SIZE - 1);
		int y = globalY & (Settings::CHUNK_SIZE - 1);
		int z = globalZ & (Settings::CHUNK_SIZE - 1);
		
		for (size_t side = 0; side < 6; side++)
		{
			size_t axis = side >> 1;
			int offCoords[3] = { (int)x, (int)y, (int)z };
			offCoords[axis] += (side & 1) ? -1 : 1;

			Chunk::BlockAndLighting blockAndLighting = chunk->getBlockAndLightingAtSideCheck(offCoords[0], offCoords[1], offCoords[2], side);
			if (blockAndLighting.block == Block::Void)
			{
				continue;
			}

			const BlockData& blockData = ALL_BLOCK_DATA[(size_t)blockAndLighting.block];
			if (!blockData.transparent)
			{
				continue;
			}

			uint8_t lightLevel = (blockAndLighting.lighting >> (4 * blockOrSky)) & 15;
			if (lightLevel == 0)
			{
				continue;
			}

			if (lightLevel < darkness.lightValue)
			{
				chunk->setLightingAtSideCheck(offCoords[0], offCoords[1], offCoords[2], side, 0, blockOrSky);
				needToCheck.emplace_back
				(
					chunkGlobalX + offCoords[0],
					chunkGlobalY + offCoords[1],
					chunkGlobalZ + offCoords[2],
					blockOrSky,
					lightLevel
				);
			}
			else
			{
				std::lock_guard<std::mutex> lock(Chunk::lightingFloodFillMutex);
				Chunk::lightingFloodFillVector.emplace_back
				(
					chunkGlobalX + offCoords[0],
					chunkGlobalY + offCoords[1],
					chunkGlobalZ + offCoords[2],
					blockOrSky
				);
			}
		}
	}
}

void World::updateLighting()
{
	// TODO: if place 2 light source close to eachother, after removing them, 1 block light will stay in last removed one
	{
		std::lock_guard<std::mutex> lock(Chunk::lightingUpdateMutex);
		Profiler::start(BLOCK_LIGHT_UPDATE_INDEX);
		for (const auto& update : Chunk::lightingUpdateVector)
		{
			updateBlockLighting(update);
		}
		Profiler::end(BLOCK_LIGHT_UPDATE_INDEX);

		Profiler::start(SKY_LIGHT_UPDATE_INDEX);
		for (const auto& update : Chunk::lightingUpdateVector)
		{
			updateSkyLighting(update);
		}
		Profiler::end(SKY_LIGHT_UPDATE_INDEX);

		Chunk::lightingUpdateVector.clear();
	}

	// TODO: add flood fill profiling
	darknessFloodFill();
	lightingFloodFill();
}

void World::updateBlockLighting(const LightUpdate& lightUpdate)
{
	Chunk* chunk = (Chunk*)lightUpdate.chunk;
	// TODO: remove
	if (chunk->state != Chunk::State::Loaded)
	{
		std::cerr << "UpdateBlockLighting: Chunk is unloaded" << std::endl;
		return;
	}
	size_t x = lightUpdate.x;
	size_t y = lightUpdate.y;
	size_t z = lightUpdate.z;
	int X = chunk->X;
	int Y = chunk->Y;
	int Z = chunk->Z;
	int chunkGlobalX = X * Settings::CHUNK_SIZE;
	int chunkGlobalY = Y * Settings::CHUNK_SIZE;
	int chunkGlobalZ = Z * Settings::CHUNK_SIZE;
	int globalX = (int)x + chunkGlobalX;
	int globalY = (int)y + chunkGlobalY;
	int globalZ = (int)z + chunkGlobalZ;
	const BlockData& blockData = ALL_BLOCK_DATA[(size_t)lightUpdate.block];
	const BlockData& prevBlockData = ALL_BLOCK_DATA[(size_t)lightUpdate.prevBlock];
	
	bool addLights = false;
	bool removeLights = false;
	bool addLights2 = false;
	bool removeLights2 = false;

	if (blockData.lightPower > prevBlockData.lightPower)
	{
		addLights = true;
	}
	else if (blockData.lightPower < prevBlockData.lightPower)
	{
		addLights = blockData.lightPower > 0;
		removeLights = true;
	}
	else
	{
		addLights2 = blockData.transparent && !prevBlockData.transparent;
		removeLights2 = !blockData.transparent && prevBlockData.transparent;
	}

	if (addLights)
	{
		if (blockData.transparent)
		{
			chunk->setLightingAtInBoundaries(x, y, z, blockData.lightPower, false);
		}
		else
		{
			chunk->setLightingAtInBoundaries(x, y, z, 0, false);
		}
		for (size_t side = 0; side < 6; side++)
		{
			size_t axis = side >> 1;
			int offCoords[3] = { (int)x, (int)y, (int)z };
			offCoords[axis] += (side & 1) ? -1 : 1;

			Block neighbourBlock = chunk->getBlockAtSideCheck(offCoords[0], offCoords[1], offCoords[2], side);
			if (neighbourBlock == Block::Void)
			{
				continue;
			}

			const BlockData& neighbourBlockData = ALL_BLOCK_DATA[(size_t)neighbourBlock];
			if (!neighbourBlockData.transparent)
			{
				continue;
			}
			chunk->setLightingAtSideCheck(offCoords[0], offCoords[1], offCoords[2], side, blockData.lightPower, false);
			{
				std::lock_guard<std::mutex> lock(Chunk::lightingFloodFillMutex);
				Chunk::lightingFloodFillVector.emplace_back
				(
					chunkGlobalX + offCoords[0],
					chunkGlobalY + offCoords[1],
					chunkGlobalZ + offCoords[2],
					false
				);
			}
		}
	}
	if (removeLights)
	{
		if (prevBlockData.transparent)
		{
			chunk->setLightingAtInBoundaries(x, y, z, 0, false);
		}
		if (blockData.transparent)
		{
			uint8_t maxNeighbourLighting = 0;
			for (size_t side = 0; side < 6; side++)
			{
				size_t axis = side >> 1;
				int offCoords[3] = { (int)x, (int)y, (int)z };
				offCoords[axis] += (side & 1) ? -1 : 1;

				Block neighbourBlock = chunk->getBlockAtSideCheck(offCoords[0], offCoords[1], offCoords[2], side);
				if (neighbourBlock == Block::Void)
				{
					continue;
				}

				const BlockData& neighbourBlockData = ALL_BLOCK_DATA[(size_t)neighbourBlock];
				if (neighbourBlockData.lightPower > maxNeighbourLighting)
				{
					maxNeighbourLighting = neighbourBlockData.lightPower;
				}

			}
			chunk->setLightingAtInBoundaries(x, y, z, maxNeighbourLighting, false);
		}
		for (size_t side = 0; side < 6; side++)
		{
			size_t axis = side >> 1;
			int offCoords[3] = { (int)x, (int)y, (int)z };
			offCoords[axis] += (side & 1) ? -1 : 1;

			Block neighbourBlock = chunk->getBlockAtSideCheck(offCoords[0], offCoords[1], offCoords[2], side);
			if (neighbourBlock == Block::Void)
			{
				continue;
			}

			const BlockData& neighbourBlockData = ALL_BLOCK_DATA[(size_t)neighbourBlock];
			if (!neighbourBlockData.transparent)
			{
				continue;
			}


			uint8_t maxNeighbourLighting = 0;
			for (size_t side = 0; side < 6; side++)
			{
				size_t axis = side >> 1;
				int offCoords2[3] = { offCoords[0], offCoords[1], offCoords[2] };
				offCoords2[axis] += (side & 1) ? -1 : 1;

				Block neighbourBlock = chunk->getBlockAtSideCheck(offCoords2[0], offCoords2[1], offCoords2[2], side);
				if (neighbourBlock == Block::Void)
				{
					continue;
				}

				const BlockData& neighbourBlockData = ALL_BLOCK_DATA[(size_t)neighbourBlock];
				if (neighbourBlockData.lightPower > maxNeighbourLighting)
				{
					maxNeighbourLighting = neighbourBlockData.lightPower;
				}

			}

			chunk->setLightingAtSideCheck(offCoords[0], offCoords[1], offCoords[2], side, maxNeighbourLighting, false);
			{
				std::lock_guard<std::mutex> lock(Chunk::darknessFloodFillMutex);

				Chunk::darknessFloodFillVector.emplace_back(
					chunkGlobalX + offCoords[0],
					chunkGlobalY + offCoords[1],
					chunkGlobalZ + offCoords[2],
					false, prevBlockData.lightPower
				);
			}
		}
	}
	if (addLights2)
	{
		// TODO: if it will check for light block in flood fill methods, remove this
		size_t maxLighting = 0; // size_t because there can be a problem with std::max when maxLighting overflows
		size_t maxLightingSide = 0;

		for (size_t side = 0; side < 6; side++)
		{
			size_t axis = side >> 1;
			int offCoords[3] = { (int)x, (int)y, (int)z };
			offCoords[axis] += (side & 1) ? -1 : 1;

			Chunk::BlockAndLighting blockAndLighting = chunk->getBlockAndLightingAtSideCheck(offCoords[0], offCoords[1], offCoords[2], side);
			if (blockAndLighting.block == Block::Void)
			{
				continue;
			}

			const BlockData& blockData = ALL_BLOCK_DATA[(size_t)blockAndLighting.block];
			if (blockData.transparent)
			{
				uint8_t lighting = blockAndLighting.lighting & 15;
				if (lighting > maxLighting)
				{
					maxLighting = lighting;
					maxLightingSide = side;
				}
			}
			else if (blockData.lightPower > 0 && blockData.lightPower >= maxLighting)
			{
				maxLighting = blockData.lightPower + 1;
				maxLightingSide = 6;
			}
		}
		if (maxLighting > 1)
		{
			int offCoords[3] = { (int)x, (int)y, (int)z };
			if (maxLightingSide == 6)
			{
				maxLighting--;
				chunk->setLightingAtInBoundaries(offCoords[0], offCoords[1], offCoords[2], maxLighting, false);
			}
			else
			{
				size_t axis = maxLightingSide >> 1;
				offCoords[axis] += (maxLightingSide & 1) ? -1 : 1;
				chunk->setLightingAtSideCheck(offCoords[0], offCoords[1], offCoords[2], maxLightingSide, maxLighting, false);
			}
			{
				std::lock_guard<std::mutex> lock(Chunk::lightingFloodFillMutex);
				Chunk::lightingFloodFillVector.emplace_back
				(
					chunkGlobalX + offCoords[0],
					chunkGlobalY + offCoords[1],
					chunkGlobalZ + offCoords[2],
					false
				);
			}
		}
	}
	if (removeLights2)
	{
		// TODO: doing many unnecessary stuff
		uint8_t prevLighting = chunk->getLightingAtInBoundaries(x, y, z) & 15;
		chunk->setLightingAtInBoundaries(x, y, z, 0, false);
		if (prevLighting > 1)
		{
			for (size_t side = 0; side < 6; side++)
			{
				size_t axis = side >> 1;
				int offCoords[3] = { (int)x, (int)y, (int)z };
				offCoords[axis] += (side & 1) ? -1 : 1;

				Chunk::BlockAndLighting blockAndLighting = chunk->getBlockAndLightingAtSideCheck(offCoords[0], offCoords[1], offCoords[2], side);
				if (blockAndLighting.block == Block::Void)
				{
					continue;
				}

				const BlockData& neighbourBlockData = ALL_BLOCK_DATA[(size_t)blockAndLighting.block];
				if (!neighbourBlockData.transparent)
				{
					continue;
				}

				uint8_t neighbourLighting = blockAndLighting.lighting & 15;
				if (neighbourLighting <= prevLighting)
				{
					chunk->setLightingAtSideCheck(offCoords[0], offCoords[1], offCoords[2], side, 0, false);
					std::lock_guard<std::mutex> lock(Chunk::darknessFloodFillMutex);
					Chunk::darknessFloodFillVector.emplace_back
					(
						chunkGlobalX + offCoords[0],
						chunkGlobalY + offCoords[1],
						chunkGlobalZ + offCoords[2],
						false, neighbourLighting
					);
				}
			}
		}
	}
}

void World::updateSkyLighting(const LightUpdate& lightUpdate)
{
	//const BlockData& blockData = ALL_BLOCK_DATA[(size_t)lightUpdate.block];
	//const BlockData& prevBlockData = ALL_BLOCK_DATA[(size_t)lightUpdate.prevBlock];

	//if (blockData.transparent == prevBlockData.transparent)
	//{
	//	return;
	//}

	//Chunk* chunk = (Chunk*)lightUpdate.chunk;
	//// TODO: remove
	//if (chunk->state != Chunk::State::Loaded)
	//{
	//	std::cerr << "UpdateSkyLighting: Chunk is unloaded" << std::endl;
	//	return;
	//}
	//size_t x = lightUpdate.x;
	//size_t y = lightUpdate.y;
	//size_t z = lightUpdate.z;
	//int X = chunk->X;
	//int Y = chunk->Y;
	//int Z = chunk->Z;
	//int chunkGlobalX = X * Settings::CHUNK_SIZE;
	//int chunkGlobalY = Y * Settings::CHUNK_SIZE;
	//int chunkGlobalZ = Z * Settings::CHUNK_SIZE;
	//int globalX = (int)x + chunkGlobalX;
	//int globalY = (int)y + chunkGlobalY;
	//int globalZ = (int)z + chunkGlobalZ;

	//ChunkColumnData* chunkColumnData = TerrainGenerator::getHeightMap(X, Z);
	//chunkColumnData->startUsing();
	//const int skyLightMaxHeight = chunkColumnData->getSlMHAt(x, z);

	//uint8_t fillLightPower = 0;
	//bool findNewSLMH = false;

	//if (blockData.transparent) // !prevBlockData.transparent // Block was replaced with transparent one
	//{
	//	uint8_t maxLighting = 0;
	//	size_t maxLightingSide = 0;

	//	// finding max neighbour lighting to flood-fill from there
	//	for (size_t side = 0; side < 6; side++)
	//	{
	//		size_t axis = side >> 1;
	//		int offCoords[3] = { (int)x, (int)y, (int)z };
	//		offCoords[axis] += (side & 1) ? -1 : 1;
	//		Chunk::BlockAndLighting blockAndLighting = chunk->getBlockAndLightingAtSideCheck(offCoords[0], offCoords[1], offCoords[2], side);
	//		if (ALL_BLOCK_DATA[(size_t)blockAndLighting.block].transparent)
	//		{
	//			uint8_t lighting = blockAndLighting.lighting >> 4;
	//			if (lighting > maxLighting)
	//			{
	//				maxLighting = lighting;
	//				maxLightingSide = side;
	//			}
	//		}
	//	}

	//	// lighting of value 1 can't propagate
	//	if (maxLighting > 1)
	//	{
	//		size_t axis = maxLightingSide >> 1;
	//		int offCoords[3] = { globalX, globalY, globalZ };
	//		offCoords[axis] += (maxLightingSide & 1) ? -1 : 1;
	//		std::lock_guard<std::mutex> lock(Chunk::lightingFloodFillMutex);
	//		Chunk::lightingFloodFillVector.emplace_back
	//		(
	//			offCoords[0], offCoords[1], offCoords[2],
	//			maxLighting, true
	//		);
	//	}

	//	// destroying highest block in that column
	//	if (globalY != skyLightMaxHeight)
	//	{
	//		chunkColumnData->stopUsing();
	//		return;
	//	}
	//	fillLightPower = 15;
	//	findNewSLMH = true;

	//	// could do that in loop, but here we already know that block is transparent
	//	{
	//		std::lock_guard<std::mutex> lock(Chunk::lightingFloodFillMutex);
	//		Chunk::lightingFloodFillVector.emplace_back
	//		(
	//			globalX, globalY, globalZ,
	//			fillLightPower, true
	//		);
	//	}
	//}
	//else // prevBlockData.transparent // Block was replaced with solid one
	//{
	//	if (globalY > skyLightMaxHeight)
	//	{
	//		chunkColumnData->setSlMHAt(x, z, globalY);
	//	}
	//	//fillLightPower = 0; // is already zero

	//	uint8_t prevLighting = chunk->getLightingAtInBoundaries(x, y, z) >> 4;
	//	chunk->setLightingAtInBoundaries(x, y, z, 0, true);

	//	// lighting of value 1 can't propagate
	//	if (prevLighting > 1)
	//	{
	//		for (size_t side = 0; side < 6; side++)
	//		{
	//			size_t axis = side >> 1;
	//			int offCoords[3] = { (int)x, (int)y, (int)z };
	//			offCoords[axis] += (side & 1) ? -1 : 1;
	//			Chunk::BlockAndLighting blockAndLighting = chunk->getBlockAndLightingAtSideCheck(offCoords[0], offCoords[1], offCoords[2], side);
	//			if (ALL_BLOCK_DATA[(size_t)blockAndLighting.block].transparent)
	//			{
	//				uint8_t lighting = blockAndLighting.lighting >> 4;
	//				if (lighting < prevLighting) // && lighting > 0  it will always be higher than zero
	//				{
	//					std::lock_guard<std::mutex> lock(Chunk::darknessFloodFillMutex);
	//					Chunk::darknessFloodFillVector.emplace_back(
	//						chunkGlobalX + offCoords[0],
	//						chunkGlobalY + offCoords[1],
	//						chunkGlobalZ + offCoords[2],
	//						lighting, true
	//					);
	//				}
	//			}
	//		}
	//	}
	//}
	//
	//// fill column with light value
	//int localY = y;
	//int fillGlobalY = globalY;
	//Chunk* chunk_ = chunk;

	//int maxIterations = skyLightMaxHeight == INT_MIN ? 0 : std::max(0, globalY - skyLightMaxHeight) - !blockData.transparent;
	//auto& floodFillVector = fillLightPower == 0 ? Chunk::darknessFloodFillVector : Chunk::lightingFloodFillVector;
	//auto& floodFillMutex = fillLightPower == 0 ? Chunk::darknessFloodFillMutex : Chunk::lightingFloodFillMutex;
	//if (maxIterations > 0)
	//{
	//	std::lock_guard<std::mutex> lock(floodFillMutex);
	//	floodFillVector.reserve(floodFillVector.size() + maxIterations);
	//}
	//while (true)
	//{
	//	fillGlobalY--;
	//	localY--;
	//	if (localY < 0)
	//	{
	//		chunk_ = chunk_->neighbours[3];
	//		if (!chunk_)
	//		{
	//			break;
	//		}
	//		localY = Settings::CHUNK_SIZE - 1;
	//	}

	//	Block block = chunk_->getBlockAtInBoundaries(x, localY, z);
	//	if (!ALL_BLOCK_DATA[(size_t)block].transparent)
	//	{
	//		break;
	//	}

	//	{
	//		std::lock_guard<std::mutex> lock(floodFillMutex);
	//		floodFillVector.emplace_back
	//		(
	//			globalX,
	//			fillGlobalY,
	//			globalZ,
	//			15, true
	//		);
	//	}
	//}

	//if (findNewSLMH)
	//{
	//	if (chunk_)
	//	{
	//		chunkColumnData->setSlMHAt(x, z, fillGlobalY);
	//	}
	//	else
	//	{
	//		chunkColumnData->setSlMHAt(x, z, INT_MIN);
	//	}
	//}
	//chunkColumnData->stopUsing();
}

WorldData World::loadWorldData()
{
	WorldData worldData;
	if (!std::filesystem::exists(Settings::WORLD_DATA_PATH))
	{
		return worldData;
	}
	std::ifstream file(Settings::WORLD_DATA_PATH, std::ios::binary);
	if (!file.is_open())
	{
		std::cerr << "Failed to open player data file" << std::endl;
		return worldData;
	}
	file.read(reinterpret_cast<char*>(&worldData), sizeof(WorldData));
	file.close();
	return worldData;
}

void World::saveWorldData(const WorldData& worldData)
{
	std::ofstream file(Settings::WORLD_DATA_PATH, std::ios::binary | std::ios::trunc);
	if (!file.is_open())
	{
		std::cerr << "Failed to open player data file " << std::endl;
		return;
	}
	file.write(reinterpret_cast<const char*>(&worldData), sizeof(WorldData));
	file.close();
}

World::Int3::Int3() : x(0), y(0), z(0)
{
}

World::Int3::Int3(int x, int y, int z) : x(x), y(y), z(z)
{
}

size_t World::Int3::operator()(const Int3& pos) const noexcept
{
	return pos3_hash(pos.x, pos.y, pos.z);
}

size_t World::Int3::operator()(const glm::ivec3& pos) const noexcept
{
	return pos3_hash(pos.x, pos.y, pos.z);
}

bool World::Int3::operator==(const Int3& pos) const noexcept
{
	return (x == pos.x) && (y == pos.y) && (z == pos.z);
}
