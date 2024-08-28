#include "TextRenderer.h"
#include <iostream>

std::unordered_map<char, Character> TextRenderer::characters;
unsigned int TextRenderer::spaceAdvance = 0;
VAO* TextRenderer::textVAO = nullptr;
VBO* TextRenderer::textVBO = nullptr;
unsigned int TextRenderer::textureArray = 0;
glm::vec4 TextRenderer::transforms[TEXT_SSBO_SIZE] = {};
unsigned int TextRenderer::letterIndexes[TEXT_SSBO_SIZE] = {};
SSBO* TextRenderer::transformsSSBO = nullptr;
SSBO* TextRenderer::letterIndexesSSBO = nullptr;

size_t TextRenderer::fontSize = 32;

Shader* TextRenderer::textShader = nullptr;

int TextRenderer::init(Shader* textShader)
{
    TextRenderer::textShader = textShader;

    FT_Library ft;
    if (FT_Init_FreeType(&ft))
    {
        std::cerr << "ERROR::FREETYPE: Could not init FreeType Library" << std::endl;
        return -1;
    }

    const char* fontPath = "fonts/Minecraft.ttf";

    const size_t loadRangeStart = 33;
    const size_t loadRangeEnd = 127;

    FT_Face face;
    if (FT_New_Face(ft, fontPath, 0, &face))
    {
        std::cerr << "ERROR::FREETYPE: Failed to load font" << std::endl;
        FT_Done_FreeType(ft);
        return -1;
    }

    FT_Set_Pixel_Sizes(face, fontSize, fontSize);
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

    glGenTextures(1, &textureArray);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D_ARRAY, textureArray);
    glTexImage3D(GL_TEXTURE_2D_ARRAY, 0, GL_R8,
        fontSize, fontSize, loadRangeEnd - loadRangeStart,
        0, GL_RED, GL_UNSIGNED_BYTE, 0
    );
    glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    // loading characters
    for (unsigned char c = loadRangeStart; c < loadRangeEnd; c++)
    {
        if (FT_Load_Char(face, c, FT_LOAD_RENDER))
        {
            std::cerr << "ERROR::FREETYTPE: Failed to load Glyph" << std::endl;
            continue;
        }
        if (c == ' ')
        {
            spaceAdvance = ((unsigned int)face->glyph->advance.x) >> 6;
            continue;
        }

        glTexSubImage3D
        (
            GL_TEXTURE_2D_ARRAY,
            0, 0, 0, int(c) - loadRangeStart,
            face->glyph->bitmap.width, face->glyph->bitmap.rows, 1,
            GL_RED,
            GL_UNSIGNED_BYTE, 
            face->glyph->bitmap.buffer
        );

        Character character =
        {
            int(c) - loadRangeStart,
            glm::ivec2(face->glyph->bitmap.width, face->glyph->bitmap.rows),
            glm::ivec2(face->glyph->bitmap_left, face->glyph->bitmap_top),
            (unsigned int)(face->glyph->advance.x) >> 6
        };
        characters[c] = character;
    }
    glBindTexture(GL_TEXTURE_2D_ARRAY, 0);
    FT_Done_Face(face);
    FT_Done_FreeType(ft);

    //
    const glm::vec2 vertices[4] =
    {
        {0.0f, 1.0f},
        {1.0f, 1.0f},
        {1.0f, 0.0f},
        {0.0f, 0.0f}
    };

    textVBO = new VBO((const char*)vertices, sizeof(vertices), GL_STATIC_DRAW);

    textVAO = new VAO();
    textVAO->linkFloat(2, sizeof(glm::vec2));

    transformsSSBO = new SSBO(TEXT_SSBO_SIZE * sizeof(glm::vec4));
    transformsSSBO->bindBase(2);
    letterIndexesSSBO = new SSBO(TEXT_SSBO_SIZE * sizeof(unsigned int));
    letterIndexesSSBO->bindBase(3);
	return 0;
}

void TextRenderer::destroy()
{
    glDeleteTextures(1, &textureArray);
    textVAO->clean(); delete textVAO;
    textVBO->clean(); delete textVBO;
    transformsSSBO->clean(); delete transformsSSBO;
    letterIndexesSSBO->clean(); delete letterIndexesSSBO;
}

void TextRenderer::beforeTextRender()
{
    glEnable(GL_BLEND);
    glDisable(GL_DEPTH_TEST);
    glBindTexture(GL_TEXTURE_2D_ARRAY, textureArray);
    textShader->bind();
    textVAO->bind();
    textVBO->bind();
}

void TextRenderer::afterTextRender()
{
    glDisable(GL_BLEND);
    glEnable(GL_DEPTH_TEST);
}

void TextRenderer::renderText(const std::string& text, float x, float y, float scale, glm::vec3 color, AligmentX aligmentX, AligmentY aligmentY)
{
    size_t textLength = text.size();
    if (textLength == 0)
    {
        return;
    }

    textShader->setUniformFloat3("textColor", color.r, color.g, color.b);
    scale *= 2.0f / fontSize;

    // get width and height
    float textW = 0.0f;
    int textMinY = INT_MAX;
    int textMaxY = INT_MIN;
    for (size_t i = 0; i < textLength; i++)
    {
        const auto& it = characters.find(text[i]);
        if (it == characters.end())
        {
            continue;
        }
        const Character& ch = it->second;

        if (i == 0)
        {
            textW += ch.advance - ch.bearing.x;
        }
        else if (i == textLength - 1)
        {
            textW += ch.size.x;
        }
        else
        {
            textW += ch.advance;
        }

        textMinY = std::min(textMinY, ch.bearing.y - ch.size.y);
        textMaxY = std::max(textMaxY, ch.bearing.y);
    }
    textW *= scale;
    float textH = (textMaxY - textMinY) * scale;

    if (aligmentX == AligmentX::Right)
    {
        x -= textW;
    }
    else if (aligmentX == AligmentX::Center)
    {
        x -= textW * 0.5f;
    }
    if (aligmentY == AligmentY::Top)
    {
        y -= textH;
    }
    else if (aligmentY == AligmentY::Center)
    {
        y -= textH * 0.5f;
    }

    // render
    float textX = x;
    float textY = y;
    unsigned int characterIndex = 0;
    
    while (characterIndex < textLength)
    {
        unsigned int renderCharacterIndex = 0;
        while (characterIndex < textLength && renderCharacterIndex < TEXT_SSBO_SIZE)
        {
            char c = text[characterIndex];
            characterIndex++;

            if (c == ' ')
            {
                textX += spaceAdvance * scale;
                continue;
            }
            else if (c == '\n')
            {
                textX = x;
                textY -= textH * 1.2f;
                continue;
            }
            const auto& it = characters.find(c);
            if (it == characters.end())
            {
                continue;
            }
            const Character& ch = it->second;

            float posX = textX + ch.bearing.x * scale;
            float posY = textY - (fontSize - ch.bearing.y) * scale;

            transforms[renderCharacterIndex] = { posX, posY, fontSize * scale, fontSize * scale };
            letterIndexes[renderCharacterIndex] = ch.textureID;

            textX += ch.advance * scale;
            renderCharacterIndex++;
        }

        transformsSSBO->setData((const char*)transforms, renderCharacterIndex * sizeof(glm::vec4));
        letterIndexesSSBO->setData((const char*)letterIndexes, renderCharacterIndex * sizeof(unsigned int));

        glDrawArraysInstanced(GL_TRIANGLE_FAN, 0, 4, renderCharacterIndex);
    }
}
