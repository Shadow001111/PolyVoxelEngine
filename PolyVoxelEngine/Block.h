#pragma once
#include <cstdint>

enum class Block : uint8_t
{
	Void,
	Air,
	Grass,
	Dirt,
	Stone,
	Water,
	Sand,
	Glass,
	Snow,
	Brick,
	BlackConcrete,
	WhiteConcrete,
	GrayConcrete,
	DarkGrayConcrete,
	LightGrayConcrete,
	RedConcrete,
	OrangeConcrete,
	YellowConcrete,
	GreenConcrete,
	CyanConcrete,
	BlueConcrete,
	PurpleConcrete,
	PinkConcrete,
	BrownConcrete,
	DarkGreenConcrete,
	SkinColorConcrete,
	WoodenPlanks,
	Lamp,
	Count
};

struct BlockData
{
	bool createFaces = false;
	bool transparent = false;
	bool colliding = false;
	unsigned int textures[6] = { 0, 0, 0, 0, 0, 0 };
	uint8_t lightPower = 0;
};

extern const BlockData ALL_BLOCK_DATA[(size_t)Block::Count];