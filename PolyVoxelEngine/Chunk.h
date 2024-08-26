#pragma once
#include <cstdint>
#include "settings.h"
#include "FaceInstancesVBO.h"
#include "Shapes.h"
#include <unordered_map>
#include <unordered_set>
#include <queue>
#include <iostream>
#include <fstream>
#include <filesystem>
#include "Block.h"
#include "Camera.h"
#include "Vector.h"

int pos3_hash(int x, int y, int z);

struct DrawCommand
{
	unsigned int offset = 0;
	unsigned int facesCount[12]{0};

	DrawCommand();

	void resetFaces();
	bool anyFaces() const;
};

struct Face
{
	bool none = true;
	bool transparent;
	char ao;
	unsigned int textureID;
	uint8_t lighting;
#if ENABLE_SMOOTH_LIGHTING
	uint8_t smoothLighting[4];
#endif

	bool operator==(const Face& other) const;
};

class PhysicEntity;

struct PhysicEntityCollider
{
	glm::vec3& position;
	glm::vec3& size;
	glm::vec3& dpos;

	PhysicEntityCollider(glm::vec3& position, glm::vec3& size, glm::vec3& dpos);
};

struct Light
{
	glm::ivec3 pos;
	uint8_t power;
	bool blockOrSky;

	Light();
	Light(int x, int y, int z, uint8_t lightPower, bool blockOrSky);
};

struct SizeT3
{
	size_t x, y, z;

	SizeT3(size_t x, size_t y, size_t z);
};

struct LightUpdate
{
	void* chunk;
	uint8_t x, y, z;
	Block block, prevBlock;

	LightUpdate(void* chunk, uint8_t x, uint8_t y, uint8_t z, Block block, Block prevBlock);
};

struct TransparentSortFaceData
{
	uint8_t dataXY;
	uint8_t dataZ;
	uint8_t dataWH;
};

struct BlockAndLighting
{
	Block block;
	uint8_t lighting;
};

class Chunk
{
	Block blocks[Settings::CHUNK_SIZE_CUBED];
	uint8_t lightingMap[Settings::CHUNK_SIZE_CUBED]; // sky lighting in left bits, source lighting in right bits
	std::unordered_map<Block, Vector<uint16_t, Settings::CHUNK_SIZE_CUBED>> blockChanges;

	char getAO(int x, int y, int z, char side, const char* packOffsets) const;
	char getAOandSmoothLighting(int x, int y, int z, size_t side, const char* packOffsets, uint8_t* smoothLighting) const;

	void setBlockByIndexNoSave(size_t index, Block block);
	void setBlockAtNoSave(size_t x, size_t y, size_t z, Block block);
	void applyChanges();
	bool isChunkClosed() const;
	void greedyMeshing(Face* facesData);
	void updateLightingAt(size_t x, size_t y, size_t z, Block block, Block prevBlock);
	static std::string getFilepath(int X, int Y, int Z);
public:
	static Face* facesData;
	static FaceInstanceData* faceInstancesData;
	static FaceInstancesVBO* faceInstancesVBO;
	static std::unordered_map<int, Chunk*> chunkMap;

	static std::queue<Light> lightingFloodFillQueue;
	static std::queue<Light> darknessFloodFillQueue;
	static std::vector<LightUpdate> lightingUpdateVector;

	bool hasAnyFaces = false; // TODO: try to remove it
	uint16_t blocksCount = 0;
	int X, Y, Z;
	DrawCommand drawCommand;
	Chunk* neighbours[6];
	Vector<const PhysicEntityCollider*, Settings::MAX_ENTITIES_PER_CHUNK> physicEntities;

	//TransparentSortFaceData transparentSortFaceData[Settings::SINGLE_TYPE_FACE_INSTANCES_PER_CHUNK];

	Chunk(unsigned int ID);
	~Chunk();
	void init(int x, int y, int z);
	void destroy();

	void generateBlocks();
	void generateFaces();
	void updateFacesData();

	Block getBlockAtInBoundaries(size_t x, size_t y, size_t z) const;
	bool setBlockAtInBoundaries(size_t x, size_t y, size_t z, Block block);
	Block getBlockAt(int x, int y, int z) const;
	Block getBlockAtSideCheck(int x, int y, int z, size_t side) const;

	uint8_t getLightingAtInBoundaries(size_t x, size_t y, size_t z) const;
	void setLightingAtInBoundaries(size_t x, size_t y, size_t z, uint8_t lightPower, bool lightOrSky);
	uint8_t getLightingAt(int x, int y, int z) const;
	uint8_t getLightingAtSideCheck(int x, int y, int z, size_t side) const;
	void setLightingAt(int x, int y, int z, uint8_t lightPower, bool lightOrSky);

	BlockAndLighting getBlockAndLightingAtInBoundaries(size_t x, size_t y, size_t z) const;
	BlockAndLighting getBlockAndLightingAt(int x, int y, int z) const;
	BlockAndLighting getBlockAndLightingAtSideCheck(int x, int y, int z, size_t side) const;
	
	static Chunk* getChunkAt(int x, int y, int z);
	bool canSideBeSeen(const Camera& camera, size_t side) const;

	int posHash() const;
	static size_t getIndex(size_t x, size_t y, size_t z);
	static SizeT3 getCoordinatesByIndex(size_t index);
	static void loadData(std::unordered_map<Block, Vector<uint16_t, Settings::CHUNK_SIZE_CUBED>>& blockChanges, int X, int Y, int Z);
	static void saveData(std::unordered_map<Block, Vector<uint16_t, Settings::CHUNK_SIZE_CUBED>>& blockChanges, int X, int Y, int Z);
};