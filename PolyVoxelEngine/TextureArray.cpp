#include "TextureArray.h"
#include <iostream>
#include <stb/stb_image.h>

TextureArray::TextureArray(const char* filePath, GLint slot, int textureSize, int rowSize, int texturesCount, int desiredChannels, int textureWrapMode, bool createMipmaps) : ID(0), unit(slot)
{
	int width = 0, height = 0, numChannels = desiredChannels;

	if (numChannels < 1 || numChannels > 4)
	{
		std::cerr << "Wrong texture channels count" << std::endl;
		return;
	}

	stbi_set_flip_vertically_on_load(true);
	unsigned char* bytes = stbi_load(filePath, &width, &height, nullptr, desiredChannels);
	
	if (!bytes)
	{
		std::cerr << "Failed to load texture: " << filePath << std::endl;
		return;
	}

	constexpr GLint numChannelsToFormat[4][2] = { {GL_RED, GL_R8}, {GL_RG, GL_RG8}, {GL_RGB, GL_RGB8}, {GL_RGBA, GL_RGBA8} };

	GLint format = numChannelsToFormat[numChannels - 1][0];
	GLint formatByte = numChannelsToFormat[numChannels - 1][1];
	int mipmapLevels = 1 + (createMipmaps ? ceilf(log2f(textureSize)) : 0);

	glGenTextures(1, &ID);
	glBindTexture(GL_TEXTURE_2D_ARRAY, ID);
	glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
	glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_S, textureWrapMode);
	glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_T, textureWrapMode);
	if (textureWrapMode == GL_CLAMP_TO_BORDER)
	{
		float borderColor[] = { 0.0f, 0.0f, 0.0f, 0.0f };
		glTexParameterfv(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_BORDER_COLOR, borderColor);
	}
	glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MAX_LEVEL, mipmapLevels);

	glTexStorage3D(GL_TEXTURE_2D_ARRAY, mipmapLevels, formatByte, textureSize, textureSize, texturesCount);

	unsigned char* tileBytes = new unsigned char[textureSize * textureSize * numChannels];

	for (int layer = 0; layer < texturesCount; layer++)
	{
		int tile_y = layer / rowSize;
		int tile_x = layer % rowSize;

		for (int i = 0; i < textureSize * textureSize; ++i)
		{
			int x = i % textureSize;
			int y = i / textureSize;

			int index1 = i * numChannels;
			int index2 = ((x + tile_x * textureSize) + (y + tile_y * textureSize) * textureSize * rowSize) * numChannels;

			memcpy(tileBytes + index1, bytes + index2, numChannels);
		}

		glTexSubImage3D
		(
			GL_TEXTURE_2D_ARRAY,	// type
			0,						// mipmap level
			0, 0, layer,			// x, y, z offsets
			textureSize, textureSize, 1,	// width, height, depth
			format,					// format
			GL_UNSIGNED_BYTE,		// channel type
			tileBytes
		);
	}

	glGenerateMipmap(GL_TEXTURE_2D_ARRAY);

	{
		GLfloat value, max_anisotropy = 8.0f;
		glGetFloatv(GL_MAX_TEXTURE_MAX_ANISOTROPY, &value);
		value = (value > max_anisotropy) ? max_anisotropy : value;
		glTexParameterf(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MAX_ANISOTROPY, value);
	}

	delete[] tileBytes;
	stbi_image_free(bytes);

	unbind();
}

void TextureArray::passToShader(Shader* shader, const char* uniform, unsigned int unit) const
{
	shader->setUniformInt(uniform, unit);
}

void TextureArray::bind() const
{
	glActiveTexture(GL_TEXTURE0 + unit);
	glBindTexture(GL_TEXTURE_2D_ARRAY, ID);
}

void TextureArray::unbind() const
{
	glBindTexture(GL_TEXTURE_2D_ARRAY, 0);
}

void TextureArray::clean() const
{
	glDeleteTextures(1, &ID);
}
