#include "settings.h"

int DynamicSettings::generateChunksPerTickStationary = 200;
int DynamicSettings::generateChunksPerTickMoving = 100;

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
	float fogDensity = powf(-logf(1e-8f), 1.0f / fogGradient) / distance;
	return fogDensity;
}