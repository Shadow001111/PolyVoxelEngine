#pragma once
#include "Shader.h"

class TextureArray
{
public:
	unsigned int ID;
	unsigned int unit;

	TextureArray(const char* filePath, int slot, int textureSize, int rowSize, int texturesCount, int desiredChannels, int textureWrapMode, bool createMipmaps);

	void passToShader(Shader* shader, const char* uniform, unsigned int unit) const;
	
	void bind() const;
	void unbind() const;
	void clean() const;
};
