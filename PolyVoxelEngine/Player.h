#pragma once
#include "PhysicEntity.h"

enum class Gamemode : char
{
	Survival,
	Creative,
	Spectator
};

class Player
{
	glm::vec2 rotation, previousRotation;

	bool in_window = true;
	bool isGrounded = false;

	Gamemode gamemode = Gamemode::Creative;

	float sensitivity = 50.0f;
	float moveSpeed = 20.0f, fastMoveSpeed = 40.0f,
		airMoveSpeed = 5.0f,
		flySpeed = 100.0f, fastFlySpeed = 200.0f;
	bool flyMode = true;

	float worldEditNextTime = 0.0f;

	glm::ivec3 voxelMarkerPos;
	bool drawVoxelMarker = false;

	VAO voxelGhostVAO, uiVAO;
	VBO voxelGhostVBO, uiVBO;

	Block hotbar[9] =
	{
		Block::Grass,
		Block::Dirt,
		Block::Stone,
		Block::Glass,
		Block::Sand,
		Block::Snow,
		Block::Water,
		Block::Brick,
		Block::Lamp
	};
	uint8_t selectedHotbatSlot = 0;
	Block selectedHotbarBlock = hotbar[selectedHotbatSlot];
	bool inventoryOpened = false;
	glm::ivec2 inventorySelectedPos = { 0, 0 };

	Block playerInventory[(int)Block::Count - 2];

	RaycastHit lastRaycastHit;

	Chunk* debugChunk = nullptr;
	int debugViewMode = 0;

	void Inputs(float dt, float time);
	void Accelerate(const glm::vec3& vec);
public:
	Camera camera;
	PhysicEntity physicEntity;

	Player(glm::vec3 position, float fov, float near, float far);
	void clean() const;

	void physicUpdate(float dt, float time);
	void update(float intelpolation);
	void BeforeRender();
	void draw() const;

	void keyCallback(int key, int scancode, int action, int mods);

	void mouseButtonCallback(int button, int action);

	void scrollCallback(int yoffset);
};

