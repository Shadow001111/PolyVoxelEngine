#pragma once
#include <string>
#include <cmath>
#include <iostream>

struct DynamicSettings
{
	static int generateChunksPerTickStationary;
	static int generateChunksPerTickMoving;
};

int calcArea(int radius);

int calcVolume(int radius);

float calculateFogDensity(float distance, float fogGradient);

namespace Settings
{
	//
	constexpr int MENU_MAX_FPS = 20;
	constexpr int GAME_MAX_FPS = 30;

	constexpr bool FULLSCREEN = false;

	// World
	const std::string worldPath = "Worlds/Test";
	
	// Chunk
	constexpr int CHUNK_LOAD_RADIUS = 8;
	constexpr int CHUNK_SIZE = 16;
	constexpr size_t MAX_ENTITIES_PER_CHUNK = 256;

	// Physic
	constexpr float GRAVITY = -40.0f;
	constexpr float MAX_Y_SPEED = 200.0f;
	constexpr float AIR_RESISTANCE = 0.5f;

	// Biomes
	constexpr int BIOME_BORDER_INTERPOLATION_SIZE = 8;

	//
	constexpr size_t NOISE2_ARRAY_WIDTH = 16;

	//
	const float fogGradient = 10.0f;

	// Player
	constexpr float PLAYER_INTERACTION_DISTANCE = 16.0f;
	constexpr int INVENTORY_ROW_SIZE = 9;
	const std::string playerDataPath = worldPath + "/player.bin";

	// Textures
	constexpr int BLOCK_TEXTURE_SIZE = 16;
	constexpr int BLOCK_TEXTURES_IN_ROW = 16;
	constexpr int BLOCK_TEXTURES_COUNT = 32;
	constexpr int BLOCK_TEXTURES_NUM_CHANNELS = 4;

#pragma region ...
	constexpr int HALF_CHUNK_SIZE = CHUNK_SIZE / 2;
	constexpr int CHUNK_SIZE_SQUARED = CHUNK_SIZE * CHUNK_SIZE;
	constexpr int CHUNK_SIZE_CUBED = CHUNK_SIZE_SQUARED * CHUNK_SIZE;
	//constexpr size_t SINGLE_TYPE_FACE_INSTANCES_PER_CHUNK = (CHUNK_SIZE_CUBED / 2 * 6);
	constexpr size_t FACE_INSTANCES_PER_CHUNK = (CHUNK_SIZE_CUBED / 2 * 6) + (CHUNK_SIZE_SQUARED / 2 * 6); // solid + additionalTransparent
	const size_t MAX_RENDERED_CHUNKS_COUNT = calcVolume(CHUNK_LOAD_RADIUS);
	const size_t MAX_CHUNK_DRAW_COMMANDS_COUNT = MAX_RENDERED_CHUNKS_COUNT * 6;

	const std::string chunkSavesPath = worldPath + "/Chunks/";

	const float MAX_RENDER_DISTANCE = CHUNK_LOAD_RADIUS * CHUNK_SIZE - HALF_CHUNK_SIZE;
	const float fogDensity = calculateFogDensity(MAX_RENDER_DISTANCE, fogGradient);

	constexpr size_t BLOCK_TEXTURE_SIZE_IN_BYTES = BLOCK_TEXTURE_SIZE * BLOCK_TEXTURE_SIZE * BLOCK_TEXTURES_NUM_CHANNELS;
#pragma endregion
#define ENABLE_SMOOTH_LIGHTING true
}