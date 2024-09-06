#pragma once
#include "Chunk.h"
#include <vector>
#include <queue>
#include "Camera.h"
#include "SSBO.h"
#include <thread>
#include <mutex>
#include <condition_variable>
#include "TextureArray.h"

#include "IBO.h"
#include "VBO.h"
#include "VAO.h"

struct RaycastHit
{
	bool hit = false;
	glm::ivec3 globalPos{0, 0, 0};
	glm::ivec3 normal{0, 0, 0};
	Block block = Block::Air;
};

struct ChunkDistance
{
	Chunk* chunk = nullptr;
	float distance = 0.0f;

	ChunkDistance(Chunk* chunk, float distance);
};

struct Int3
{
	int x = 0, y = 0, z = 0;

	Int3();
	Int3(int x, int y, int z);
	size_t operator()(const Int3& pos) const noexcept;
	size_t operator()(const glm::ivec3& pos) const noexcept;
	bool operator==(const Int3& pos) const noexcept;
};

struct WorldData
{
	glm::vec3 playerPosition = {0, INT_MIN, 0};
	glm::vec2 playerRotation = { 0, 0 };

	unsigned int seed = 0;
	uint16_t worldTime = 12000;
};

class World
{
	Chunk** chunkPool = nullptr;
	size_t chunkPoolIndex = 0;
	glm::ivec3 lastPlayerLoadChunkPos;
	uint8_t sortGenerateChunksQueueTick = 9999999;

	std::queue<Chunk*> chunkGenerateQueue;
	std::unordered_set<Chunk*> generateFacesSet;

	std::unordered_map<int, std::unordered_map<Block, Vector<uint16_t, Settings::CHUNK_SIZE_CUBED>>> temporalChunkBlockChanges;
	std::unordered_set<Int3, Int3> temporalSaveDataChunks;

	bool shutdown = false;

	VAO quadInstanceVAO;
	VBO quadInstanceVBO;
	IndirectBuffer indirectBuffer;
	SSBO chunkPositionSSBO;
	SSBO chunkPositionIndexSSBO;

	DrawArraysIndirectCommand* drawCommands = nullptr;
	glm::vec3* chunkPositions = nullptr;
	unsigned int* chunkPositionIndexes = nullptr;

	uint8_t dataShrinkingTick = 0;


	Chunk* getChunk(int x, int y, int z);
	void releaseChunk(Chunk* chunk);

	void getRenderChunks(std::vector<ChunkDistance>& renderChunks, const Camera& camera) const;
	void getDrawCommands(const std::vector<ChunkDistance>& renderChunks, const Camera& camera, size_t& commandsCount, size_t& positionsCount, bool transparent);

	inline void addChunkToGenerateFaces(Chunk* chunk);
	void addSurroundingChunksToGenerateFaces(const Chunk* chunk);

	uint8_t getLightingAt(int x, int y, int z) const;
	void setLightingAt(int x, int y, int z, uint8_t power, bool lightOrSky);
	void lightingFloodFill();
	void darknessFloodFill();
	void updateLighting();
	void updateBlockLighting(const LightUpdate& lightUpdate);
	void updateSkyLighting(const LightUpdate& lightUpdate);
public:
	uint16_t time = 0;

	TextureArray blockTextures;
	TextureArray numberTextures;

	uint32_t drawCommandsCount = 0;

	World(const WorldData& worldData);
	~World();

	void update(const glm::vec3& pos, bool isMoving);
	void generateChunksBlocks(const glm::vec3& pos, bool isMoving);
	void generateChunksFaces();
	RaycastHit raycast(const glm::vec3& startPos, const glm::vec3& dir, float length);
	void setBlockAt(int x, int y, int z, Block block);
	Block getBlockAt(int x, int y, int z) const;

	bool loadChunks(int x, int y, int z, bool forced);

	static WorldData loadWorldData();
	static void saveWorldData(const WorldData& worldData);

	void draw(const Camera& camera);

	//
	void regenerateChunks();
};

