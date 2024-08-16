#pragma once
#include <glad/glad.h>
#include <stb/stb_image.h>
#include "Shader.h"

class TextureArray
{
public:
	unsigned int ID;
	GLuint unit;

	TextureArray(const char* filePath, GLint slot, int textureSize, int rowSize, int texturesCount, int desiredChannels, int textureWrapMode);

	void passToShader(Shader* shader, const char* uniform, unsigned int unit) const;
	
	void bind() const;
	void unbind() const;
	void clean() const;
};
