#include "World.h"
#include <iostream>
#include <unordered_set>
#include <chrono>
#include <glad/glad.h>
#include "GraphicController.h"
#include "TimeMeasurer.h"
#include "TerrainGenerator.h"
#include "Profiler.h"

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

ChunkDistance::ChunkDistance(Chunk* chunk, float distance) : chunk(chunk), distance(distance) {}

float intbound(float s, float ds)
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
#ifdef _DEBUG
	if (chunkPoolIndex == Settings::MAX_RENDERED_CHUNKS_COUNT)
	{
		throw std::exception("Chunk pool is empty");
	}
#endif
	Chunk* ret = chunkPool[chunkPoolIndex++];
	ret->init(x, y, z);
	return ret;
}

void World::releaseChunk(Chunk* chunk)
{
#ifdef _DEBUG
	if (chunkPoolIndex == 0)
	{
		throw std::exception("Chunk pool is full");
	}
#endif
	chunkPool[--chunkPoolIndex] = chunk;
	chunk->destroy();
}

World::World(unsigned int seed) 
	: lastPlayerLoadChunkPos{0.0f, 0.0f, 0.0f},

	quadInstanceVBO((const char*)quadInstanceVertices, 4 * sizeof(QuadInstanceVertex), GL_STATIC_DRAW),
	quadInstanceVAO(),

	indirectBuffer(Settings::MAX_CHUNK_DRAW_COMMANDS_COUNT),

	chunkPositionSSBO(Settings::MAX_RENDERED_CHUNKS_COUNT * sizeof(glm::vec3)),
	chunkPositionIndexSSBO(Settings::MAX_CHUNK_DRAW_COMMANDS_COUNT * sizeof(unsigned int)),

	blockTextures("res/Textures.png", 0, Settings::BLOCK_TEXTURE_SIZE, Settings::BLOCK_TEXTURES_IN_ROW, Settings::BLOCK_TEXTURES_COUNT, Settings::BLOCK_TEXTURES_NUM_CHANNELS, GL_REPEAT),
	numberTextures("res/Numbers.png", 1, 8, 4, 16, 1, GL_CLAMP_TO_BORDER)
{
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
	TerrainGenerator::init(seed);

	chunkPool = new Chunk*[Settings::MAX_RENDERED_CHUNKS_COUNT];
	for (size_t i = 0; i < Settings::MAX_RENDERED_CHUNKS_COUNT; i++)
	{
		chunkPool[i] = new Chunk(i);
	}

	drawCommands = new DrawArraysIndirectCommand[Settings::MAX_CHUNK_DRAW_COMMANDS_COUNT];
	for (size_t i = 0; i < Settings::MAX_CHUNK_DRAW_COMMANDS_COUNT; i++)
	{
		DrawArraysIndirectCommand& command = drawCommands[i];
		command.count = 4;
		command.first = 0;
	}

	chunkPositions = new glm::vec3[Settings::MAX_RENDERED_CHUNKS_COUNT];
	chunkPositionIndexes = new unsigned int[Settings::MAX_CHUNK_DRAW_COMMANDS_COUNT];

	calculateBlockColors();

	//
	if (!std::filesystem::exists(Settings::chunkSavesPath))
	{
		std::filesystem::create_directories(Settings::chunkSavesPath);
	}
}

World::~World()
{
	// shutdown threads
	shutdown = true;
	
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
	size_t left = Settings::MAX_RENDERED_CHUNKS_COUNT - chunkPoolIndex;
	if (left > 0)
	{
		std::cout << "Chunks left in pool: " << left << std::endl;
		for (size_t i = chunkPoolIndex; i < Settings::MAX_RENDERED_CHUNKS_COUNT; i++)
		{
			delete chunkPool[i];
		}
	}
	Chunk::chunkMap.clear();

	delete[] chunkPool;
	delete[] drawCommands;
	delete[] chunkPositions;
	delete[] chunkPositionIndexes;

	TerrainGenerator::clear();
}

void World::update(const glm::vec3& pos, bool isMoving)
{
	// SaveDataChunks
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

	// load chunks
	auto loadChunksPos = glm::ivec3(pos / (float)Settings::CHUNK_SIZE);
	Profiler::start(LOAD_CHUNKS_INDEX);
	loadChunks(loadChunksPos.x, loadChunksPos.y, loadChunksPos.z, false);
	Profiler::end(LOAD_CHUNKS_INDEX);
	
	// generate blocks
	generateChunksBlocks(pos, isMoving);

	// update lighting
	updateLighting();
	darknessFloodFill();
	lightingFloodFill();

	// generate faces
	generateChunksFaces();

	// day night cycle
	time++;
	if (time >= 24000)
	{
		time = 0;
	}
	float angle = (float)time / 24000.0f * 2.0f * M_PI;
	GraphicController::chunkProgram->bind();
	GraphicController::chunkProgram->setUniformFloat("dayNightCycleSkyLightingSubtraction", (cosf(angle) + 1.0f) * 0.5f);
}

void World::generateChunksBlocks(const glm::vec3& pos, bool isMoving)
{
	size_t generateCount = chunkGenerateQueue.size();
	if (generateCount == 0)
	{
		return;
	}
	generateCount = std::min(generateCount, size_t(isMoving ? DynamicSettings::generateChunksPerTickMoving : DynamicSettings::generateChunksPerTickStationary));

	// sort
	sortGenerateChunksQueueTick++;
	if (sortGenerateChunksQueueTick > 20)
	{
		sortGenerateChunksQueueTick = 0;
		// sort queue
		std::vector<ChunkDistance> chunks;
		while (!chunkGenerateQueue.empty())
		{
			Chunk* chunk = chunkGenerateQueue.front();
			chunkGenerateQueue.pop();

			float X = chunk->X * Settings::CHUNK_SIZE;
			float Y = chunk->Y * Settings::CHUNK_SIZE;
			float Z = chunk->Z * Settings::CHUNK_SIZE;
			float distance = glm::distance2({ X, Y, Z }, pos);
			chunks.emplace_back(chunk, distance);
		}

		std::sort(chunks.begin(), chunks.end(), [&](const ChunkDistance& a, const ChunkDistance& b)
		{
			return a.distance < b.distance;
		});

		for (const ChunkDistance& pair : chunks)
		{
			chunkGenerateQueue.push(pair.chunk);
		}
	}

	// generate
	for (size_t i = 0; i < generateCount; i++)
	{
		Chunk* chunk = chunkGenerateQueue.front();
		chunkGenerateQueue.pop();
		Profiler::start(BLOCK_GENERATION_INDEX);
		chunk->generateBlocks();
		Profiler::end(BLOCK_GENERATION_INDEX);
		addChunkToGenerateFaces(chunk);
		addSurroundingChunksToGenerateFaces(chunk);
	}
}

void World::generateChunksFaces()
{
	if (generateFacesSet.empty())
	{
		return;
	}
	Chunk::faceInstancesVBO->bind();
	Profiler::start(MESH_GENERATION_INDEX);
	for (Chunk* chunk : generateFacesSet)
	{
		chunk->generateFaces();
	}
	Profiler::end(MESH_GENERATION_INDEX);
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

bool World::loadChunks(int x, int y, int z, bool forced)
{
	if (
		!forced &&
		x == lastPlayerLoadChunkPos.x &&
		y == lastPlayerLoadChunkPos.y &&
		z == lastPlayerLoadChunkPos.z
	)
	{
		return false;
	}
	lastPlayerLoadChunkPos = glm::ivec3(x, y, z);

	// unload chunks
	int radius = Settings::CHUNK_LOAD_RADIUS;
	int rsq = radius * radius;
	for (auto it = Chunk::chunkMap.begin(); it != Chunk::chunkMap.end();)
	{
		Chunk* chunk = it->second;
		int dx = x - chunk->X;
		int dy = y - chunk->Y;
		int dz = z - chunk->Z;
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
			TerrainGenerator::loadHeightMap(x + dx, z + dz);

			int D2 = D1 - dz * dz;
			int maxY = (int)sqrtf(D2);
			for (int dy = -maxY; dy <= maxY; dy++)
			{
				Chunk* chunk = Chunk::getChunkAt(x + dx, y + dy, z + dz);
				if (chunk)
				{
					continue;
				}

				chunk = getChunk(x + dx, y + dy, z + dz);
				chunkGenerateQueue.push(chunk);
			}
		} 
	}
	return true;
}

void World::draw(const Camera& camera)
{
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
	glEnable(GL_DEPTH_TEST);
	glDisable(GL_BLEND);
	if (commandsCount > 0)
	{
		chunkPositionSSBO.setData((const char*)chunkPositions, chunkPositionsCount * sizeof(glm::vec3));
		chunkPositionIndexSSBO.setData((const char*)chunkPositionIndexes, commandsCount * sizeof(unsigned int));
		indirectBuffer.setData(drawCommands, commandsCount);

		quadInstanceVAO.bind();

		glEnable(GL_CULL_FACE);

		if (GraphicController::zPrePass)
		{
			GraphicController::deferredChunkProgram->bind();
			glEnable(GL_RASTERIZER_DISCARD);
			glDepthFunc(GL_LESS);
			glMultiDrawArraysIndirect(GL_TRIANGLE_FAN, nullptr, commandsCount, 0);

			GraphicController::chunkProgram->bind();
			glDisable(GL_RASTERIZER_DISCARD);
			glDepthFunc(GL_LEQUAL);
			glMultiDrawArraysIndirect(GL_TRIANGLE_FAN, nullptr, commandsCount, 0);
		}
		else
		{
			glDepthFunc(GL_LESS);
			GraphicController::chunkProgram->bind();
			glMultiDrawArraysIndirect(GL_TRIANGLE_FAN, nullptr, commandsCount, 0);
		}
	}
	
	// draw transparent
	std::reverse(renderChunks.begin(), renderChunks.end());
	getDrawCommands(renderChunks, camera, commandsCount, chunkPositionsCount, true);
	glDisable(GL_CULL_FACE);
	if (commandsCount > 0)
	{
		/*struct FaceDistance
		{
			TransparentSortFaceData* face = nullptr;
			float distance = 0.0f;
		};
		std::vector<FaceDistance> faces;

		for (const ChunkDistance& pair : renderChunks)
		{
			const Chunk* chunk = pair.chunk;
			const DrawCommand& command = chunk->drawCommand;
			for (size_t normalID = 0; normalID < 6; normalID++)
			{
				size_t axis = normalID >> 1;
				GLuint facesCount = command.facesCount[normalID + 6];
				if (facesCount > 0 && chunk->canSideBeSeen(camera, normalID))
				{
					for (size_t i = 0; i < facesCount; i++)
					{
						const auto& face = chunk->transparentSortFaceData[i];

						int x = face.dataXY >> 4;
						int y = face.dataXY & 15;
						int z = face.dataZ;
						int w = (face.dataWH >> 4) + 1;
						int h = (face.dataWH & 15) + 1;

						int extX = 0.0f;
						int extY = 0.0f;
						int extZ = 0.0f;

						if (axis == 0)
						{
							extY
						}

						glm::vec3 center = {x + extX * 0.5f, y + extY * 0.5f, z + extZ * 0.5f};
					}
				}
			}
		}*/

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

void World::buildImage(int x, int y, int z, int w, int h, int xAxis, int yAxis, const char* imagePath)
{
	int imgW, imgH;
	stbi_set_flip_vertically_on_load(true);
	unsigned char* bytes = stbi_load(imagePath, &imgW, &imgH, nullptr, 3);

	if (!bytes)
	{
		std::cerr << "Can not load '" << imagePath << "'" << std::endl;
		return;
	}

	float xMultiplier = (float)imgW / (float)w;
	float yMultiplier = (float)imgH / (float)h;

	int coords[3] = { x, y, z };

	for (int ix = 0; ix < w; ix++)
	{
		int imageX = floorf(ix * xMultiplier);
		coords[xAxis] += ix;
		for (int iy = 0; iy < h; iy++)
		{
			int imageY = floorf(iy * yMultiplier);
			coords[yAxis] += iy;

			// get block by color
			Block block = Block::BlackConcrete;

			{
				size_t index = 3 * (imageX + imageY * imgW);

				int r = bytes[index];
				int g = bytes[index + 1];
				int b = bytes[index + 2];

				uint32_t minDifference = -1;

				for (size_t block_ = (size_t)Block::Grass; block_ < (size_t)Block::Count; block_++)
				{
					RGB8 blockColor = blockColors[block_];

					int dr = (int)blockColor.r - r;
					int dg = (int)blockColor.g - g;
					int db = (int)blockColor.b - b;

					uint32_t difference = dr * dr + dg * dg + db * db;

					if (difference < minDifference)
					{
						block = (Block)block_;
						minDifference = difference;
					}
				}
			}

			// place block
			setBlockAt(coords[0], coords[1], coords[2], block);

			coords[yAxis] -= iy;
		}
		coords[xAxis] -= ix;
	}

	stbi_image_free(bytes);
}

void World::regenerateChunks()
{
	for (const auto& pair : Chunk::chunkMap)
	{
		Chunk* chunk = pair.second;
		chunk->destroy();
		chunk->init(chunk->X, chunk->Y, chunk->Z);
		chunkGenerateQueue.push(chunk);
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
		float distance = glm::distance2(chunkShape.center, camera.position);
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
			if (facesCount > 0 && chunk->canSideBeSeen(camera, normalID))
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
	generateFacesSet.emplace(chunk);
}

void World::addSurroundingChunksToGenerateFaces(const Chunk* chunk)
{
	int chX = chunk->X;
	int chY = chunk->Y;
	int chZ = chunk->Z;
	for (int dx = -1; dx <= 1; dx++)
	{
		for (int dy = -1; dy <= 1; dy++)
		{
			for (int dz = -1; dz <= 1; dz++)
			{
				if (dx == 0 || dy == 0 || dz == 0)
				{
					continue;
				}
				Chunk* chunk = Chunk::getChunkAt(chX + dx, chY + dy, chZ + dz);
				if (chunk)
				{
					addChunkToGenerateFaces(chunk);
				}
			}
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
	std::unordered_set<glm::ivec3, Int3> alreadyCheckedBlock;
	std::unordered_set<glm::ivec3, Int3> alreadyCheckedSky;
	auto& needToCheck = Chunk::lightingFloodFillQueue;
	while (!needToCheck.empty())
	{
		auto light = needToCheck.front();
		needToCheck.pop();
		int x = light.pos.x;
		int y = light.pos.y;
		int z = light.pos.z;
		bool blockOrSky = light.blockOrSky;

		// if block is void or solid we skip
		Block block = getBlockAt(x, y, z);
		if (block == Block::Void || !ALL_BLOCK_DATA[(size_t)block].transparent)
		{
			continue;
		}

		// comparing lighting
		uint8_t prevLight = (getLightingAt(x, y, z) >> (4 * blockOrSky)) & 15;
		if (light.power < prevLight)
		{
			continue;
		}
		else if (light.power == prevLight)
		{
			// check if cell was already checked
			auto& alreadyChecked = blockOrSky ? alreadyCheckedSky : alreadyCheckedBlock;
			if (alreadyChecked.find(light.pos) != alreadyChecked.end())
			{
				continue;
			}
			alreadyChecked.insert(light.pos);
		}

		setLightingAt(x, y, z, light.power, blockOrSky);
		light.power--;

		if (light.power != 0)
		{
			needToCheck.emplace(x + 1, y, z, light.power, blockOrSky);
			needToCheck.emplace(x - 1, y, z, light.power, blockOrSky);
			needToCheck.emplace(x, y + 1, z, light.power, blockOrSky);
			needToCheck.emplace(x, y - 1, z, light.power, blockOrSky);
			needToCheck.emplace(x, y, z + 1, light.power, blockOrSky);
			needToCheck.emplace(x, y, z - 1, light.power, blockOrSky);
		}
	}
}

void World::darknessFloodFill()
{
	std::unordered_set<glm::ivec3, Int3> alreadyCheckedBlock;
	std::unordered_set<glm::ivec3, Int3> alreadyCheckedSky;
	auto& needToCheck = Chunk::darknessFloodFillQueue;
	while (!needToCheck.empty())
	{
		auto light = needToCheck.front();
		needToCheck.pop();
		int x = light.pos.x;
		int y = light.pos.y;
		int z = light.pos.z;
		bool blockOrSky = light.blockOrSky;

		// if block is void or solid we skip
		Block block = getBlockAt(x, y, z);
		if (block == Block::Void || !ALL_BLOCK_DATA[(size_t)block].transparent)
		{
			continue;
		}

		// check if cell was already checked
		auto& alreadyChecked = blockOrSky ? alreadyCheckedSky : alreadyCheckedBlock;
		if (alreadyChecked.find(light.pos) != alreadyChecked.end())
		{
			continue;
		}
		alreadyChecked.insert(light.pos);

		// comparing lighting
		uint8_t prevLight = (getLightingAt(x, y, z) >> (4 * blockOrSky)) & 15;
		if (light.power > prevLight)
		{
			continue;
		}
		else if (light.power < prevLight)
		{
			Chunk::lightingFloodFillQueue.emplace(x, y, z, prevLight, blockOrSky);
			continue;
		}
		light.power--;

		setLightingAt(x, y, z, 0, blockOrSky);

		if (light.power != 0)
		{
			needToCheck.emplace(x + 1, y, z, light.power, blockOrSky);
			needToCheck.emplace(x - 1, y, z, light.power, blockOrSky);
			needToCheck.emplace(x, y + 1, z, light.power, blockOrSky);
			needToCheck.emplace(x, y - 1, z, light.power, blockOrSky);
			needToCheck.emplace(x, y, z + 1, light.power, blockOrSky);
			needToCheck.emplace(x, y, z - 1, light.power, blockOrSky);
		}
	}
}

void World::updateLighting()
{
	// TODO: if place 2 light source close to eachother, after removing them, 1 block light will stay in last removed one
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

void World::updateBlockLighting(const LightUpdate& lightUpdate)
{
	Chunk* chunk = (Chunk*)lightUpdate.chunk;
	size_t x = lightUpdate.x;
	size_t y = lightUpdate.y;
	size_t z = lightUpdate.z;
	int X = chunk->X;
	int Y = chunk->Y;
	int Z = chunk->Z;
	const BlockData& blockData = ALL_BLOCK_DATA[(size_t)lightUpdate.block];
	const BlockData& prevBlockData = ALL_BLOCK_DATA[(size_t)lightUpdate.prevBlock];

	//
	bool addLights = false;
	bool removeLights = false;
	bool addLights2 = false;
	bool removeLights2 = false;
	// TODO: sky light still need to be changed
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
		int offsets[3] = { 0, 0, 0 };
		for (size_t side = 0; side < 6; side++)
		{
			size_t axis = side >> 1;
			offsets[axis] = (side & 1) ? -1 : 1;

			int offX = x + offsets[0];
			int offY = y + offsets[1];
			int offZ = z + offsets[2];

			Chunk::lightingFloodFillQueue.emplace(
				offX + X * Settings::CHUNK_SIZE,
				offY + Y * Settings::CHUNK_SIZE,
				offZ + Z * Settings::CHUNK_SIZE,
				blockData.lightPower, false
			);

			offsets[axis] = 0;
		}

		if (blockData.transparent)
		{
			Chunk::lightingFloodFillQueue.emplace(
				x + X * Settings::CHUNK_SIZE,
				y + Y * Settings::CHUNK_SIZE,
				z + Z * Settings::CHUNK_SIZE,
				blockData.lightPower, false
			);
		}
	}
	if (removeLights)
	{
		int offsets[3] = { 0, 0, 0 };
		for (size_t side = 0; side < 6; side++)
		{
			size_t axis = side >> 1;
			offsets[axis] = (side & 1) ? -1 : 1;

			int offX = x + offsets[0];
			int offY = y + offsets[1];
			int offZ = z + offsets[2];

			Chunk::darknessFloodFillQueue.emplace(
				offX + X * Settings::CHUNK_SIZE,
				offY + Y * Settings::CHUNK_SIZE,
				offZ + Z * Settings::CHUNK_SIZE,
				prevBlockData.lightPower, false
			);

			offsets[axis] = 0;
		}
		if (prevBlockData.transparent)
		{
			Chunk::darknessFloodFillQueue.emplace(
				x + X * Settings::CHUNK_SIZE,
				y + Y * Settings::CHUNK_SIZE,
				z + Z * Settings::CHUNK_SIZE,
				prevBlockData.lightPower, false
			);
		}
	}
	if (addLights2)
	{
		uint8_t maxLighting = 0;
		size_t maxLightingSide = 0;

		int offsets[3] = { 0, 0, 0 };
		for (size_t side = 0; side < 6; side++)
		{
			size_t axis = side >> 1;
			offsets[axis] = (side & 1) ? -1 : 1;

			int offX = x + offsets[0];
			int offY = y + offsets[1];
			int offZ = z + offsets[2];

			Block block = chunk->getBlockAtSideCheck(offX, offY, offZ, side);
			const BlockData& blockData = ALL_BLOCK_DATA[(size_t)block];
			if (blockData.transparent)
			{
				uint8_t lighting = chunk->getLightingAtSideCheck(offX, offY, offZ, side) & 15;
				if (lighting > maxLighting)
				{
					maxLighting = lighting;
					maxLightingSide = side;
				}
			}
			else if (blockData.lightPower > 0)
			{
				if (blockData.lightPower + 1 > maxLighting)
				{
					maxLighting = blockData.lightPower;
					maxLightingSide = 6;
				}
			}
			offsets[axis] = 0;
		}
		if (maxLighting > 1)
		{
			if (maxLightingSide < 6)
			{
				size_t axis = maxLightingSide >> 1;
				offsets[axis] = (maxLightingSide & 1) ? -1 : 1;
			}
			int offX = x + offsets[0];
			int offY = y + offsets[1];
			int offZ = z + offsets[2];
			Chunk::lightingFloodFillQueue.emplace(
				offX + X * Settings::CHUNK_SIZE,
				offY + Y * Settings::CHUNK_SIZE,
				offZ + Z * Settings::CHUNK_SIZE,
				maxLighting, false
			);
		}
	}
	if (removeLights2)
	{
		uint8_t prevLighting = chunk->getLightingAtInBoundaries(x, y, z) & 15;
		chunk->setLightingAtInBoundaries(x, y, z, 0, false);
		if (prevLighting > 1)
		{	
			int offsets[3] = { 0, 0, 0 };
			for (size_t side = 0; side < 6; side++)
			{
				size_t axis = side >> 1;
				offsets[axis] = (side & 1) ? -1 : 1;

				int offX = x + offsets[0];
				int offY = y + offsets[1];
				int offZ = z + offsets[2];
				Block block = chunk->getBlockAtSideCheck(offX, offY, offZ, side);
				const BlockData& blockData = ALL_BLOCK_DATA[(size_t)block];
				if (blockData.transparent)
				{
					uint8_t lighting = chunk->getLightingAtSideCheck(offX, offY, offZ, side) & 15;
					if (lighting < prevLighting)
					{
						Chunk::darknessFloodFillQueue.emplace(
							offX + X * Settings::CHUNK_SIZE,
							offY + Y * Settings::CHUNK_SIZE,
							offZ + Z * Settings::CHUNK_SIZE,
							lighting, false
						);
					}
				}
				offsets[axis] = 0;
			}
		}
	}
}

void World::updateSkyLighting(const LightUpdate& lightUpdate)
{
	const BlockData& blockData = ALL_BLOCK_DATA[(size_t)lightUpdate.block];
	const BlockData& prevBlockData = ALL_BLOCK_DATA[(size_t)lightUpdate.prevBlock];

	if (blockData.transparent == prevBlockData.transparent)
	{
		return;
	}

	Chunk* chunk = (Chunk*)lightUpdate.chunk;
	size_t x = lightUpdate.x;
	size_t y = lightUpdate.y;
	size_t z = lightUpdate.z;
	int X = chunk->X;
	int Y = chunk->Y;
	int Z = chunk->Z;

	int globalX = x + X * Settings::CHUNK_SIZE;
	int globalY = y + Y * Settings::CHUNK_SIZE;
	int globalZ = z + Z * Settings::CHUNK_SIZE;

	// TODO: merge these identical if statements
	HeightMap* heightMap = TerrainGenerator::getHeightMap(X, Z);
	int skyLightMaxHeight = heightMap->getSlMHAt(x, z);

	uint8_t fillLightPower = 0;
	bool findNewSLMH = false;

	if (blockData.transparent) // !prevBlockData.transparent
	{
		uint8_t maxLighting = 0;
		size_t maxLightingSide = 0;

		int offsets[3] = { 0, 0, 0 };
		for (size_t side = 0; side < 6; side++)
		{
			size_t axis = side >> 1;
			offsets[axis] = (side & 1) ? -1 : 1;
			int offX = x + offsets[0];
			int offY = y + offsets[1];
			int offZ = z + offsets[2];
			Block block = chunk->getBlockAtSideCheck(offX, offY, offZ, side);
			if (ALL_BLOCK_DATA[(size_t)block].transparent)
			{
				uint8_t lighting = chunk->getLightingAtSideCheck(offX, offY, offZ, side) >> 4;
				if (lighting > maxLighting)
				{
					maxLighting = lighting;
					maxLightingSide = side;
					// doesnt help
					/*if (maxLighting == 15)
					{
						break;
					}*/
				}
			}
			offsets[axis] = 0;
		}
		if (maxLighting > 1)
		{
			size_t axis = maxLightingSide >> 1;
			offsets[axis] = (maxLightingSide & 1) ? -1 : 1;
			Chunk::lightingFloodFillQueue.emplace(
				globalX + offsets[0],
				globalY + offsets[1],
				globalZ + offsets[2],
				maxLighting, true
			);
		}

		if (globalY != skyLightMaxHeight)
		{
			return;
		}
		fillLightPower = 15;
		findNewSLMH = true;
		Chunk::lightingFloodFillQueue.emplace(
			globalX, globalY, globalZ,
			fillLightPower, true
		);
	}
	else // prevBlockData.transparent
	{
		uint8_t prevLighting = chunk->getLightingAtInBoundaries(x, y, z) >> 4;
		chunk->setLightingAtInBoundaries(x, y, z, 0, true);
		if (prevLighting > 1)
		{
			int offsets[3] = { 0, 0, 0 };
			for (size_t side = 0; side < 6; side++)
			{
				size_t axis = side >> 1;
				offsets[axis] = (side & 1) ? -1 : 1;
				int offX = x + offsets[0];
				int offY = y + offsets[1];
				int offZ = z + offsets[2];
				Block block = chunk->getBlockAtSideCheck(offX, offY, offZ, side);
				if (ALL_BLOCK_DATA[(size_t)block].transparent)
				{
					uint8_t lighting = chunk->getLightingAtSideCheck(offX, offY, offZ, side) >> 4;
					if (lighting < prevLighting) // && lighting > 0  it will always be higher than zero
					{
						Chunk::darknessFloodFillQueue.emplace(
							globalX + offsets[0],
							globalY + offsets[1],
							globalZ + offsets[2],
							lighting, true
						);
					}
				}
				offsets[axis] = 0;
			}
		}

		fillLightPower = 0;
		if (globalY > skyLightMaxHeight)
		{
			heightMap->setSlMHAt(x, z, globalY);
		}
	}
	
	// fill column with light value
	int localY = y;
	Chunk* chunk_ = chunk;
	while (true)
	{
		localY--;
		if (localY < 0)
		{
			chunk_ = chunk_->neighbours[3];
			if (!chunk_)
			{
				break;
			}
			localY = Settings::CHUNK_SIZE - 1;
		}

		Block block = chunk_->getBlockAtInBoundaries(x, localY, z);
		if (!ALL_BLOCK_DATA[(size_t)block].transparent)
		{
			break;
		}

		(fillLightPower == 0 ? Chunk::darknessFloodFillQueue : Chunk::lightingFloodFillQueue).emplace
		(
			globalX,
			localY + chunk_->Y * Settings::CHUNK_SIZE,
			globalZ,
			15, true
		);
	}
	if (findNewSLMH)
	{
		if (chunk_)
		{
			heightMap->setSlMHAt(x, z, localY + chunk_->Y * Settings::CHUNK_SIZE);
		}
		else
		{
			heightMap->setSlMHAt(x, z, INT_MIN);
		}
	}
}

void World::calculateBlockColors()
{
	int imgW, imgH;
	stbi_set_flip_vertically_on_load(true);
	unsigned char* bytes = stbi_load("res/Textures.png", &imgW, &imgH, nullptr, 3);

	if (bytes)
	{
		for (size_t block = 0; block < (size_t)Block::Count; block++)
		{
			size_t blockTexture = ALL_BLOCK_DATA[block].textures[0];

			int tile_y = blockTexture / Settings::BLOCK_TEXTURES_IN_ROW;
			int tile_x = blockTexture % Settings::BLOCK_TEXTURES_IN_ROW;

			size_t r = 0, g = 0, b = 0;
			for (int i = 0; i < Settings::BLOCK_TEXTURE_SIZE * Settings::BLOCK_TEXTURE_SIZE; ++i)
			{
				int x = i % Settings::BLOCK_TEXTURE_SIZE;
				int y = i / Settings::BLOCK_TEXTURE_SIZE;

				int index = ((x + tile_x * Settings::BLOCK_TEXTURE_SIZE) + (y + tile_y * Settings::BLOCK_TEXTURE_SIZE) * Settings::BLOCK_TEXTURES_IN_ROW * Settings::BLOCK_TEXTURES_IN_ROW) * 3;

				r += bytes[index];
				g += bytes[index + 1];
				b += bytes[index + 2];
			}

			int div = Settings::BLOCK_TEXTURE_SIZE * Settings::BLOCK_TEXTURE_SIZE;
			r /= div;
			g /= div;
			b /= div;

			blockColors[block] = { (uint8_t)r, (uint8_t)g, (uint8_t)b };
		}

		stbi_image_free(bytes);
	}
	else
	{
		std::cerr << "Can not load image for block colors" << std::endl;
	}
}

WorldData World::loadWorldData() const
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

void World::saveWorldData(const WorldData& worldData) const
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

Int3::Int3() : x(0), y(0), z(0)
{
}

Int3::Int3(int x, int y, int z) : x(x), y(y), z(z)
{
}

size_t Int3::operator()(const Int3& pos) const
{
	return pos3_hash(pos.x, pos.y, pos.z);
}

size_t Int3::operator()(const glm::ivec3& pos) const
{
	return pos3_hash(pos.x, pos.y, pos.z);
}

bool Int3::operator==(const Int3& pos) const
{
	return (x == pos.x) && (y == pos.y)&& (z == pos.z);
}
