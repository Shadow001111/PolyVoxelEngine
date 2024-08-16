#pragma once
#include <ft2build.h>
#include <freetype/freetype.h>
#include <glm/glm.hpp>
#include <unordered_map>
#include "Shader.h"
#include "VAO.h"
#include "VBO.h"
#include "GraphicController.h"

struct Character
{
	unsigned int textureID;
	glm::ivec2 size;
	glm::ivec2 bearing;
	unsigned int advance;
};

class TextRenderer
{
	static FT_Library ft;
	static FT_Face face;
	static std::unordered_map<char, Character> characters;
	static VAO* textVAO;
	static VBO* textVBO;

	static size_t fontSize;
public:
	static int init();
	static void destroy();

	static void beforeTextRender(Shader* shader);
	static void afterTextRender();
	static void renderText(Shader* shader, std::string text, float x, float y, float scale, glm::vec3 color);
};

