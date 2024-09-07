#include "Block.h"

const BlockData ALL_BLOCK_DATA[(size_t)Block::Count] =
{
	{false, true, false, 0, {0, 0, 0, 0, 0, 0}},  // Void
	{false, true, false, 0, {0, 0, 0, 0, 0, 0}},  // Air
	{true, false, true,  0, {1, 1, 0, 2, 1, 1}},  // Grass
	{true, false, true,  0, {2, 2, 2, 2, 2, 2}},  // Dirt
	{true, false, true,  0, {4, 4, 4, 4, 4, 4}},  // Stone
	{true, true,  false, 0, {11, 11, 11, 11, 11, 11}}, // Water
	{true, false, true,  0, {6, 6, 6, 6, 6, 6}},  // Sand
	{true, true,  true,  0, {12, 12, 12, 12, 12, 12}}, // Glass
	{true, false, true,  0, {5, 5, 5, 5, 5, 5}},  // Snow
	{true, false, true,  0, {13, 13, 13, 13, 13, 13}}, // Brick

	{true, false, true,  0, {14, 14, 14, 14, 14, 14}}, // Black color concrete
	{true, false, true,  0, {15, 15, 15, 15, 15, 15}}, // White concrete
	{true, false, true,  0, {16, 16, 16, 16, 16, 16}}, // Gray concrete
	{true, false, true,  0, {17, 17, 17, 17, 17, 17}}, // Dark gray concrete
	{true, false, true,  0, {18, 18, 18, 18, 18, 18}}, // Light gray concrete
	{true, false, true,  0, {19, 19, 19, 19, 19, 19}}, // Red concrete
	{true, false, true,  0, {20, 20, 20, 20, 20, 20}}, // Orange concrete
	{true, false, true,  0, {21, 21, 21, 21, 21, 21}}, // Yellow concrete
	{true, false, true,  0, {22, 22, 22, 22, 22, 22}}, // Green concrete
	{true, false, true,  0, {23, 23, 23, 23, 23, 23}}, // Cyan concrete
	{true, false, true,  0, {24, 24, 24, 24, 24, 24}}, // Blue concrete
	{true, false, true,  0, {25, 25, 25, 25, 25, 25}}, // Purple concrete
	{true, false, true,  0, {26, 26, 26, 26, 26, 26}}, // Pink concrete
	{true, false, true,  0, {27, 27, 27, 27, 27, 27}}, // Brown concrete
	{true, false, true,  0, {28, 28, 28, 28, 28, 28}}, // Dark green concrete
	{true, false, true,  0, {29, 29, 29, 29, 29, 29}}, // Skin color concrete

	{true, false, true,  0, {30, 30, 30, 30, 30, 30}}, // Wooden planks
	{true, false, true, 15, {31, 31, 31, 31, 31, 31}}, // Lamp
};