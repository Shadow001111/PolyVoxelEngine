#pragma once
#include "settings.h"

struct FaceInstanceData
{
	int data1 = 0;
	int data2 = 0;
#if ENABLE_SMOOTH_LIGHTING
	int data3 = 0;
#endif

	FaceInstanceData();
#if ENABLE_SMOOTH_LIGHTING
	void set(int x, int y, int z, int w, int h, int normalID, char ao, unsigned int textureID, int lighting, const uint8_t* softLighting);
#else
	void set(int x, int y, int z, int w, int h, int normalID, char ao, unsigned int textureID, int lighting);
#endif
};

class FaceInstancesVBO
{
	unsigned int autolinkLayout = 0;
	unsigned int autolinkOffset = 0;
	unsigned int ID = 0;
public:
	FaceInstancesVBO(size_t instancesCount, size_t layoutOffset);
	void setData(const FaceInstanceData* instancesData, size_t offset, size_t count);

	void linkFloat(unsigned int num_components);
	void linkInt(int num_components);

	void bind() const;
	static void unbind();
	void clean() const;
};

