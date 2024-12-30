#include "settings.h"

int calcArea(int radius)
{
	int P = 0;
	int rsq = radius * radius;
	for (int x = 1; x < radius; x++)
	{
		P += (int)sqrtf(rsq - x * x);
	}
	return (P + radius) * 4 + 1;
}

int calcVolume(int radius)
{
	int P = 0;
	int rsq = radius * radius;
	for (int x = 1; x < radius; x++)
	{
		int D1 = rsq - x * x;
		int maxY = (int)sqrt(D1);
		for (int y = 1; y <= maxY; y++)
		{
			P += (int)sqrtf(D1 - y * y);
		}
	}
	return P * 8 + (calcArea(radius) - radius * 2) * 3 - 2;
}

float calculateFogDensity(float distance, float fogGradient)
{
	return powf(-logf(1e-8f), 1.0f / fogGradient) / distance;
}

namespace Settings
{
	int DynamicSettings::generateChunksPerTickStationary = 200;
	int DynamicSettings::generateChunksPerTickMoving = 100;

	int CHUNK_LOAD_RADIUS = 5;
	size_t MAX_RENDERED_CHUNKS_COUNT = calcVolume(CHUNK_LOAD_RADIUS);
	size_t MAX_CHUNK_DRAW_COMMANDS_COUNT = MAX_RENDERED_CHUNKS_COUNT * 6;
}