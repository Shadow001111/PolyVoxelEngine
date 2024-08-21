#include "Block.h"

const BlockData ALL_BLOCK_DATA[(size_t)Block::Count] =
{
	{false, true, false, {0, 0, 0, 0, 0, 0}, 0}, // Void
	{false, true, false, {0, 0, 0, 0, 0, 0}, 0}, // Air
	{true, false, true, {1, 1, 0, 2, 1, 1}, 0}, // Grass
	{true, false, true, {2, 2, 2, 2, 2, 2}, 0}, // Dirt
	{true, false, true, {4, 4, 4, 4, 4, 4}, 0}, // Stone
	{true, true, false, {11, 11, 11, 11, 11, 11}, 0}, // Water
	{true, false, true, {6, 6, 6, 6, 6, 6}, 0}, // Sand
	{true, true, true, {12, 12, 12, 12, 12, 12}, 0}, // Glass
	{true, false, true, {5, 5, 5, 5, 5, 5}, 0}, // Snow
	{true, false, true, {13, 13, 13, 13, 13, 13}, 0}, // Brick

	{true, false, true, {14, 14, 14, 14, 14, 14}, 0},    // Black color concrete
	{true, false, true, {15, 15, 15, 15, 15, 15}, 0},    // White concrete
	{true, false, true, {16, 16, 16, 16, 16, 16}, 0},    // Gray concrete
	{true, false, true, {17, 17, 17, 17, 17, 17}, 0},    // Dark gray concrete
	{true, false, true, {18, 18, 18, 18, 18, 18}, 0},    // Light gray concrete
	{true, false, true, {19, 19, 19, 19, 19, 19}, 0},    // Red concrete
	{true, false, true, {20, 20, 20, 20, 20, 20}, 0},    // Orange concrete
	{true, false, true, {21, 21, 21, 21, 21, 21}, 0},    // Yellow concrete
	{true, false, true, {22, 22, 22, 22, 22, 22}, 0},    // Green concrete
	{true, false, true, {23, 23, 23, 23, 23, 23}, 0},    // Cyan concrete
	{true, false, true, {24, 24, 24, 24, 24, 24}, 0},    // Blue concrete
	{true, false, true, {25, 25, 25, 25, 25, 25}, 0},    // Purple concrete
	{true, false, true, {26, 26, 26, 26, 26, 26}, 0},    // Pink concrete
	{true, false, true, {27, 27, 27, 27, 27, 27}, 0},    // Brown concrete
	{true, false, true, {28, 28, 28, 28, 28, 28}, 0},    // Dark green concrete
	{true, false, true, {29, 29, 29, 29, 29, 29}, 0},    // Skin color concrete

	{true, false, true, {30, 30, 30, 30, 30, 30}, 0},    // Wooden planks
	{true, false, true, {31, 31, 31, 31, 31, 31}, 15},   // Lamp
};