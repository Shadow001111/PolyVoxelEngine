#pragma once
#include <ft2build.h>
#include <freetype/freetype.h>
#include <glm/glm.hpp>
#include <unordered_map>
#include "Shader.h"
#include "VAO.h"
#include "VBO.h"
#include "GraphicController.h"
#include "SSBO.h"

enum class AligmentX : char
{
	Left,
	Center,
	Right
};

enum class AligmentY : char
{
	Bottom,
	Center,
	Top
};

struct Character
{
	unsigned int textureID;
	glm::ivec2 size;
	glm::ivec2 bearing;
	unsigned int advance;
};

struct RenderCharacterData
{
	glm::vec4 transform;
	unsigned int textureID;
};

constexpr size_t TEXT_SSBO_SIZE = 400;

class TextRenderer
{
	static FT_Library ft;
	static FT_Face face;
	static std::unordered_map<char, Character> characters;
	static unsigned int spaceAdvance;
	static VAO* textVAO;
	static VBO* textVBO;
	static size_t fontSize;
	static unsigned int textureArray;

	static RenderCharacterData renderCharactersData[TEXT_SSBO_SIZE];
	static SSBO* textSSBO;

	static Shader* textShader;
public:

	static int init(Shader* textShader);
	static void destroy();

	static void beforeTextRender();
	static void afterTextRender();
	static void renderText(const std::string& text, float x, float y, float scale, glm::vec3 color, AligmentX aligmentX, AligmentY aligmentY);
};

