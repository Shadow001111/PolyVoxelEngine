#include "Player.h"
#include <glm/gtc/matrix_inverse.hpp>
#include <glm/gtx/compatibility.hpp>

constexpr int intCeil(float x_)
{
	int x = (int)x_;
	return x < x_ ? x + 1 : x;
}

constexpr int INVENTORY_ROWS_COUNT = intCeil((float)((size_t)Block::Count - 2) / (float)Settings::INVENTORY_ROW_SIZE);

struct VoxelGhostVertex
{
	glm::vec3 position;
	glm::vec2 uv;

	VoxelGhostVertex(float x, float y, float z, float u, float v) : position(x, y, z), uv(u, v)
	{}
};

struct UIVertex
{
	glm::vec2 position;
	glm::vec2 uv;

	UIVertex(float x, float y, float u, float v) : position(x, y), uv(u, v)
	{}
};

const VoxelGhostVertex voxelGhostVertices[6 * 6] =
{
	// right
	{1.0f, 1.0f, 0.0f,   0.0f, 0.0f},
	{1.0f, 1.0f, 1.0f,   1.0f, 0.0f},
	{1.0f, 0.0f, 1.0f,   1.0f, 1.0f},

	{1.0f, 1.0f, 0.0f,   0.0f, 0.0f},
	{1.0f, 0.0f, 1.0f,   1.0f, 1.0f},
	{1.0f, 0.0f, 0.0f,   0.0f, 1.0f},

	// left
	{0.0f, 1.0f, 0.0f,   0.0f, 0.0f},
	{0.0f, 1.0f, 1.0f,   1.0f, 0.0f},
	{0.0f, 0.0f, 1.0f,   1.0f, 1.0f},

	{0.0f, 1.0f, 0.0f,   0.0f, 0.0f},
	{0.0f, 0.0f, 1.0f,   1.0f, 1.0f},
	{0.0f, 0.0f, 0.0f,   0.0f, 1.0f},

	// up
	{1.0f, 1.0f, 0.0f,   0.0f, 0.0f},
	{1.0f, 1.0f, 1.0f,   1.0f, 0.0f},
	{0.0f, 1.0f, 1.0f,   1.0f, 1.0f},

	{1.0f, 1.0f, 0.0f,   0.0f, 0.0f},
	{0.0f, 1.0f, 1.0f,   1.0f, 1.0f},
	{0.0f, 1.0f, 0.0f,   0.0f, 1.0f},

	// down
	{1.0f, 0.0f, 0.0f,   0.0f, 0.0f},
	{1.0f, 0.0f, 1.0f,   1.0f, 0.0f},
	{0.0f, 0.0f, 1.0f,   1.0f, 1.0f},

	{1.0f, 0.0f, 0.0f,   0.0f, 0.0f},
	{0.0f, 0.0f, 1.0f,   1.0f, 1.0f},
	{0.0f, 0.0f, 0.0f,   0.0f, 1.0f},

	// forward
	{0.0f, 1.0f, 1.0f,   0.0f, 0.0f},
	{1.0f, 1.0f, 1.0f,   1.0f, 0.0f},
	{1.0f, 0.0f, 1.0f,   1.0f, 1.0f},

	{0.0f, 1.0f, 1.0f,   0.0f, 0.0f},
	{1.0f, 0.0f, 1.0f,   1.0f, 1.0f},
	{0.0f, 0.0f, 1.0f,   0.0f, 1.0f},

	// back
	{0.0f, 1.0f, 0.0f,   0.0f, 0.0f},
	{1.0f, 1.0f, 0.0f,   1.0f, 0.0f},
	{1.0f, 0.0f, 0.0f,   1.0f, 1.0f},

	{0.0f, 1.0f, 0.0f,   0.0f, 0.0f},
	{1.0f, 0.0f, 0.0f,   1.0f, 1.0f},
	{0.0f, 0.0f, 0.0f,   0.0f, 1.0f},
};

const UIVertex uiVertices[6] =
{
	{0.0, 1.0, 0.0, 1.0},
	{1.0, 1.0, 1.0, 1.0},
	{1.0, 0.0, 1.0, 0.0},

	{0.0, 1.0, 0.0, 1.0},
	{1.0, 0.0, 1.0, 0.0},
	{0.0, 0.0, 0.0, 0.0},
};

void Player::Accelerate(const glm::vec3& vec)
{
	physicEntity.accelerate(vec);
}

Player::Player(glm::vec3 position, float fov, float near, float far) :
	physicEntity(position, { 0.4f, 1.8f, 0.4f }, { 0.0f, -0.8f, 0.0f }), rotation(0.0f, 0.0f), previousRotation(rotation),
	camera(position, fov, near, far)
{
	voxelGhostVAO = VAO();
	voxelGhostVBO = VBO((const char*)voxelGhostVertices, sizeof(voxelGhostVertices), GL_STATIC_DRAW);
	voxelGhostVAO.linkFloat(3, sizeof(VoxelGhostVertex));
	voxelGhostVAO.linkFloat(2, sizeof(VoxelGhostVertex));

	uiVAO = VAO();
	uiVBO = VBO((const char*)uiVertices, sizeof(uiVertices), GL_STATIC_DRAW);
	uiVAO.linkFloat(2, sizeof(UIVertex));
	uiVAO.linkFloat(2, sizeof(UIVertex));
	VAO::unbind();

	GraphicController::setCursorMode(GLFW_CURSOR_DISABLED);
	//
	for (size_t i = 0; i < (size_t)Block::Count - 2; i++)
	{
		playerInventory[i] = Block(i + 2);
	}

	flyMode = gamemode == Gamemode::Creative;
}

void Player::clean() const
{
	voxelGhostVAO.clean();
	voxelGhostVBO.clean();
	uiVAO.clean();
	uiVBO.clean();
}

void Player::physicUpdate(float dt, float time)
{
	previousRotation = rotation;
	Inputs(dt, time);

	physicEntity.gravityEnabled = !flyMode;
	physicEntity.airResistanceMultiplier = flyMode ? 5.0f : 1.0f;
	physicEntity.physicUpdate(dt);
	isGrounded = physicEntity.isGrounded;
}

void Player::update(float intelpolation)
{
	// position
	camera.position = glm::lerp(physicEntity.previousPosition, physicEntity.position, intelpolation);

	// rotation
	float yaw0 = glm::radians(previousRotation.y);
	float pitch0 = glm::radians(previousRotation.x);

	float yaw1 = glm::radians(rotation.y);
	float pitch1 = glm::radians(rotation.x);

	float yaw = glm::lerp(yaw0, yaw1, intelpolation);
	float pitch = glm::lerp(pitch0, pitch1, intelpolation);

	glm::vec3 newForward
	(
		cosf(yaw) * cosf(pitch),
		-sinf(pitch),
		sinf(yaw) * cosf(pitch)
	);

	camera.Forward = newForward;
	camera.updateVectors();
}

void Player::BeforeRender()
{
	camera.updateMatrix();
	GraphicController::chunkProgram->bind();
	camera.passMatrixToShader(GraphicController::chunkProgram, "camMatrix");
	camera.passPositionToShader(GraphicController::chunkProgram, "camPos");

	GraphicController::chunkProgram->setUniformInt("debugViewMode", debugViewMode);
}

void Player::draw() const
{
	GraphicController::voxelGhostProgram->bind();
	camera.passMatrixToShader(GraphicController::voxelGhostProgram, "camMatrix");

	voxelGhostVAO.bind();
	glDisable(GL_CULL_FACE);
	glEnable(GL_BLEND);

	if (drawVoxelMarker)
	{
		GraphicController::voxelGhostProgram->setUniformFloat3("scale", 1.0f, 1.0f, 1.0f);
		GraphicController::voxelGhostProgram->setUniformFloat("borderScale", 1.0f);

		GraphicController::voxelGhostProgram->setUniformFloat3("position", voxelMarkerPos.x, voxelMarkerPos.y, voxelMarkerPos.z);
		GraphicController::voxelGhostProgram->setUniformFloat3("color", 0.0f, 0.0f, 0.0f);
		glDrawArrays(GL_TRIANGLES, 0, 36);
	}

	// debug chunk
	if (debugChunk)
	{
		GraphicController::voxelGhostProgram->setUniformFloat3("scale", Settings::CHUNK_SIZE, Settings::CHUNK_SIZE, Settings::CHUNK_SIZE);
		GraphicController::voxelGhostProgram->setUniformFloat("borderScale", 2.0f / Settings::CHUNK_SIZE);
		GraphicController::voxelGhostProgram->setUniformFloat3("position",
			debugChunk->X * Settings::CHUNK_SIZE,
			debugChunk->Y * Settings::CHUNK_SIZE,
			debugChunk->Z * Settings::CHUNK_SIZE
		);
		GraphicController::voxelGhostProgram->setUniformFloat3("color", 0.0f, 0.0f, 0.0f);

		glDrawArrays(GL_TRIANGLES, 0, 36);
	}

	// draw hotbar
	{
		const float hotbarLeft = -0.6f;
		const float hotbarRight = fabsf(hotbarLeft);

		const float hotbarCellWidth = (hotbarRight - hotbarLeft) / 9.0f;
		const float hotbarCellHeight = hotbarCellWidth * GraphicController::aspectRatio;

		GraphicController::hotbarProgram->bind();
		GraphicController::hotbarProgram->setUniformFloat2("scale", hotbarCellWidth, hotbarCellHeight);
		GraphicController::hotbarProgram->setUniformInt("drawSlot", 1);

		uiVAO.bind();
		glDisable(GL_DEPTH_TEST);

		physicEntity.world->blockTextures.bind();

		for (int i = 0; i < 9; i++)
		{
			GraphicController::hotbarProgram->setUniformFloat("brightness", i == selectedHotbatSlot ? 1.0f : 0.5f);

			float x = hotbarLeft + i * hotbarCellWidth;
			GraphicController::hotbarProgram->setUniformFloat2("position", x, -1.0f);
			glDrawArrays(GL_TRIANGLES, 0, 6);
		}

		// draw hotbar blocks
		const float hotbarBlockScale = 0.7f;
		GraphicController::hotbarProgram->setUniformFloat2("scale", hotbarCellWidth * hotbarBlockScale, hotbarCellHeight * hotbarBlockScale);
		GraphicController::hotbarProgram->setUniformInt("drawSlot", 0);

		const float y = hotbarCellHeight * (1.0f - hotbarBlockScale) * 0.5f - 1.0f;
		for (int i = 0; i < 9; i++)
		{
			Block block = hotbar[i];
			if (block == Block::Void)
			{
				continue;
			}

			float x = hotbarLeft + i * hotbarCellWidth + hotbarCellWidth * (1.0f - hotbarBlockScale) * 0.5f;
			GraphicController::hotbarProgram->setUniformFloat2("position", x, y);

			GraphicController::hotbarProgram->setUniformInt("textureID", ALL_BLOCK_DATA[(size_t)block].textures[0]);
			glDrawArrays(GL_TRIANGLES, 0, 6);
		}
	}

	// draw inventory
	if (inventoryOpened)
	{
		float inventoryLeft = -0.6f;
		float inventoryTop = 0.9f;
		float inventoryBlockScale = 0.7f;

		float inventoryRight = fabsf(inventoryLeft);

		float inventoryCellWidth = (inventoryRight - inventoryLeft) / Settings::INVENTORY_ROW_SIZE;
		float inventoryCellHeight = inventoryCellWidth * GraphicController::aspectRatio;

		GraphicController::hotbarProgram->setUniformFloat2("scale", inventoryCellWidth, inventoryCellHeight);
		GraphicController::hotbarProgram->setUniformInt("drawSlot", 1);

		for (int y = 0; y < INVENTORY_ROWS_COUNT; y++)
		{
			for (int x = 0; x < Settings::INVENTORY_ROW_SIZE; x++)
			{
				GraphicController::hotbarProgram->setUniformFloat("brightness", (x == inventorySelectedPos.x && y == inventorySelectedPos.y) ? 1.0f : 0.5f);
				GraphicController::hotbarProgram->setUniformFloat2("position",
					inventoryLeft + x * inventoryCellWidth,
					inventoryTop - (y + 1) * inventoryCellHeight);

				glDrawArrays(GL_TRIANGLES, 0, 6);
			}
		}

		//
		GraphicController::hotbarProgram->setUniformFloat2("scale", inventoryCellWidth * inventoryBlockScale, inventoryCellHeight * inventoryBlockScale);
		GraphicController::hotbarProgram->setUniformInt("drawSlot", 0);

		size_t index = 0;
		for (int y = 0; y < INVENTORY_ROWS_COUNT; y++)
		{
			for (int x = 0; x < Settings::INVENTORY_ROW_SIZE; x++)
			{
				if (index >= (size_t)Block::Count - 2)
				{
					break;
				}

				GraphicController::hotbarProgram->setUniformFloat("brightness", 0.5f);
				GraphicController::hotbarProgram->setUniformFloat2("position",
					inventoryLeft + x * inventoryCellWidth + inventoryCellWidth * (1.0f - inventoryBlockScale) * 0.5f,
					inventoryTop - (y + 1) * inventoryCellHeight + inventoryCellHeight * (1.0f - inventoryBlockScale) * 0.5f
				);
				GraphicController::hotbarProgram->setUniformInt("textureID", ALL_BLOCK_DATA[(size_t)playerInventory[index]].textures[0]);

				glDrawArrays(GL_TRIANGLES, 0, 6);

				index++;
			}
			if (index >= (size_t)Block::Count - 2)
			{
				break;
			}
		}
	}
}

void Player::keyCallback(int key, int scancode, int action, int mods)
{
	if (action != GLFW_PRESS)
	{
		return;
	}

	// NUMBER BUTTONS
	for (int i = 0; i < 9; i++)
	{
		if (key == GLFW_KEY_1 + i)
		{	
			selectedHotbatSlot = i;
			selectedHotbarBlock = hotbar[i];
			return;
		}
	}

	//
	if (key == GLFW_KEY_O)
	{
		glm::ivec3 pos = glm::floor(physicEntity.position / (float)Settings::CHUNK_SIZE);
		debugChunk = Chunk::getChunkAt(pos.x, pos.y, pos.z);
	}
	else if (key == GLFW_KEY_I)
	{
		inventoryOpened = !inventoryOpened;
		if (inventoryOpened)
		{
			GraphicController::setCursorMode(GLFW_CURSOR_NORMAL);
		}
		else
		{
			glfwSetCursorPos(GraphicController::window, GraphicController::width / 2, GraphicController::height / 2);
			GraphicController::setCursorMode(GLFW_CURSOR_DISABLED);
		}
	}
	else if (key == GLFW_KEY_U)
	{
		int div = 10;
		int w = 1365 / div;
		int h = 2048 / div;
		physicEntity.world->buildImage(0, 100, 0, w, h, 0, 1, "PlayerContent/negr.png");
	}

	// NUMPAD
	for (int i = 0; i < 3; i++)
	{
		if (key == GLFW_KEY_KP_0 + i)
		{
			debugViewMode = i;
			return;
		}
	}
}

void Player::mouseButtonCallback(int button, int action)
{
	if (button == 0 && action == GLFW_PRESS && inventoryOpened)
	{
		float inventoryLeft = -0.6f;
		float inventoryTop = 0.9f;

		float inventoryRight = fabsf(inventoryLeft);
		float inventoryCellHeight = (inventoryRight - inventoryLeft) / Settings::INVENTORY_ROW_SIZE * GraphicController::aspectRatio;
		float inventoryBottom = inventoryTop - INVENTORY_ROWS_COUNT * inventoryCellHeight;

		double mouseX, mouseY;
		glfwGetCursorPos(GraphicController::window, &mouseX, &mouseY);

		mouseX = (mouseX / GraphicController::width) * 2.0f - 1.0f;
		mouseY = -((mouseY / GraphicController::height) * 2.0f - 1.0f);

		float xAxis = (mouseX - inventoryLeft) / (inventoryRight - inventoryLeft);
		if (xAxis < 0.0f || xAxis >= 1.0f)
		{
			return;
		}

		float yAxis = 1.0f - (mouseY - inventoryBottom) / (inventoryTop - inventoryBottom);
		if (yAxis < 0.0f || yAxis >= 1.0f)
		{
			return;
		}

		inventorySelectedPos.x = fminf(floorf(xAxis * Settings::INVENTORY_ROW_SIZE), Settings::INVENTORY_ROW_SIZE - 1);
		inventorySelectedPos.y = fminf(floorf(yAxis * INVENTORY_ROWS_COUNT), INVENTORY_ROWS_COUNT - 1);

		hotbar[selectedHotbatSlot] = playerInventory[inventorySelectedPos.x + inventorySelectedPos.y * Settings::INVENTORY_ROW_SIZE];
		selectedHotbarBlock = hotbar[selectedHotbatSlot];
	}
	else if (button == 2 && action == GLFW_PRESS && lastRaycastHit.hit)
	{
		bool hotbarAlreadyContainsBlock = false;
		for (size_t i = 0; i < 9; i++)
		{
			if (hotbar[i] == lastRaycastHit.block)
			{
				selectedHotbatSlot = i;
				selectedHotbarBlock = hotbar[selectedHotbatSlot];
				hotbarAlreadyContainsBlock = true;
				break;
			}
		}
		if (!hotbarAlreadyContainsBlock)
		{
			bool emptySlotIsFound = false;
			for (size_t i = 0; i < 9; i++)
			{
				if (hotbar[i] == Block::Void)
				{
					selectedHotbatSlot = i;
					emptySlotIsFound = true;
					break;
				}
			}
			hotbar[selectedHotbatSlot] = lastRaycastHit.block;
			selectedHotbarBlock = hotbar[selectedHotbatSlot];
		}
	}
}

void Player::scrollCallback(int yoffset)
{
	selectedHotbatSlot = ((selectedHotbatSlot - yoffset) % 9 + 9) % 9;
	selectedHotbarBlock = hotbar[selectedHotbatSlot];
}

void Player::Inputs(float dt, float time)
{
	// EXIT
	if (GraphicController::isKeyPressed(GLFW_KEY_ESCAPE))
	{
		//glfwSetWindowShouldClose(GraphicController::window, GL_TRUE);

		in_window = false;
		glfwSetInputMode(GraphicController::window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
	}

	// MOVEMENT
	glm::vec3 forward = camera.Forward;
	glm::vec3 right = camera.Right;
	glm::vec3 up = camera.Up;

	float speed = 0.0f;

	if (flyMode)
	{
		if (GraphicController::isKeyPressed(GLFW_KEY_LEFT_SHIFT))
		{
			speed = fastFlySpeed;
		}
		else
		{
			speed = flySpeed;
		}

		if (GraphicController::isKeyPressed(GLFW_KEY_W))
		{
			Accelerate((speed * dt) * forward);
		}
		if (GraphicController::isKeyPressed(GLFW_KEY_A))
		{
			Accelerate(-(speed * dt) * right);
		}
		if (GraphicController::isKeyPressed(GLFW_KEY_S))
		{
			Accelerate(-(speed * dt) * forward);
		}
		if (GraphicController::isKeyPressed(GLFW_KEY_D))
		{
			Accelerate((speed * dt) * right);
		}
		if (GraphicController::isKeyPressed(GLFW_KEY_SPACE))
		{
			Accelerate((speed * dt) * GLOBAL_UP);
		}
		if (GraphicController::isKeyPressed(GLFW_KEY_LEFT_CONTROL))
		{
			Accelerate(-(speed * dt) * GLOBAL_UP);
		}
	}
	else
	{
		if (isGrounded)
		{
			if (GraphicController::isKeyPressed(GLFW_KEY_LEFT_SHIFT))
			{
				speed = fastMoveSpeed;
			}
			else
			{
				speed = moveSpeed;
			}
		}
		else
		{
			speed = airMoveSpeed;
		}

		glm::vec3 flatForward = glm::normalize(glm::vec3(forward.x, 0.0f, forward.z));
		glm::vec3 flatRight = glm::vec3(-flatForward.z, 0.0f, flatForward.x);
		
		if (GraphicController::isKeyPressed(GLFW_KEY_W))
		{
			Accelerate((speed * dt) * flatForward);
		}
		if (GraphicController::isKeyPressed(GLFW_KEY_A))
		{
			Accelerate(-(speed * dt) * flatRight);
		}
		if (GraphicController::isKeyPressed(GLFW_KEY_S))
		{
			Accelerate(-(speed * dt) * flatForward);
		}
		if (GraphicController::isKeyPressed(GLFW_KEY_D))
		{
			Accelerate((speed * dt) * flatRight);
		}
		if (GraphicController::isKeyPressed(GLFW_KEY_SPACE) && isGrounded)
		{
			Accelerate(12.0f * GLOBAL_UP);
		}
	}

	// ROTATION
	if (in_window && !inventoryOpened)
	{
		double mouseX, mouseY;
		glfwGetCursorPos(GraphicController::window, &mouseX, &mouseY);

		float rotX = sensitivity * ((float)mouseY / (float)GraphicController::height - 0.5f);
		float rotY = sensitivity * ((float)mouseX / (float)GraphicController::width - 0.5f);

		rotation.x += rotX;
		rotation.y += rotY;

		float lim = 89.9999f;
		if (rotation.x > lim)
		{
			rotation.x = lim;
		}
		else if (rotation.x < -lim)
		{
			rotation.x = -lim;
		}

		glfwSetCursorPos(GraphicController::window, GraphicController::width / 2, GraphicController::height / 2);
	}

	// MOUSE BUTTON
	auto hit = physicEntity.world->raycast(physicEntity.position, camera.Forward, Settings::PLAYER_INTERACTION_DISTANCE);
	lastRaycastHit = hit;
	drawVoxelMarker = hit.hit;
	if (hit.hit)
	{
		voxelMarkerPos = hit.globalPos;
	}

	if (!inventoryOpened && time > worldEditNextTime)
	{
		if (GraphicController::isMouseButtonPressed(GLFW_MOUSE_BUTTON_LEFT))
		{
			if (in_window)
			{
				if (selectedHotbarBlock != Block::Void)
				{
					worldEditNextTime = time + 0.1f;
					if (hit.hit)
					{
						physicEntity.world->setBlockAt(
							hit.globalPos.x + hit.normal.x,
							hit.globalPos.y + hit.normal.y,
							hit.globalPos.z + hit.normal.z,
							selectedHotbarBlock
						);
					}
				}
			}
			else
			{
				in_window = true;
				glfwSetInputMode(GraphicController::window, GLFW_CURSOR, GLFW_CURSOR_HIDDEN);
			}
		}

		if (GraphicController::isMouseButtonPressed(GLFW_MOUSE_BUTTON_RIGHT))
		{
			if (in_window)
			{
				worldEditNextTime = time + 0.1f;
				if (hit.hit)
				{
					physicEntity.world->setBlockAt(
						hit.globalPos.x,
						hit.globalPos.y,
						hit.globalPos.z,
						Block::Air
					);
				}
			}
			else
			{
				in_window = true;
				glfwSetInputMode(GraphicController::window, GLFW_CURSOR, GLFW_CURSOR_HIDDEN);
			}
		}
	}
}