#include "TextRenderer.h"
#include <iostream>

std::unordered_map<char, Character> TextRenderer::characters;
unsigned int TextRenderer::spaceAdvance = 0;
VAO* TextRenderer::textVAO = nullptr;
VBO* TextRenderer::textVBO = nullptr;

size_t TextRenderer::fontSize = 52;

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

    FT_Face face;
    if (FT_New_Face(ft, fontPath, 0, &face))
    {
        std::cerr << "ERROR::FREETYPE: Failed to load font" << std::endl;
        FT_Done_FreeType(ft);
        return -1;
    }

    FT_Set_Pixel_Sizes(face, 0, fontSize);
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

    // loading characters
    for (unsigned char c = 32; c <= 126; c++)
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

        unsigned int texture;
        glGenTextures(1, &texture);
        glBindTexture(GL_TEXTURE_2D, texture);
        glTexImage2D
        (
            GL_TEXTURE_2D,
            0,
            GL_RED,
            face->glyph->bitmap.width,
            face->glyph->bitmap.rows,
            0,
            GL_RED,
            GL_UNSIGNED_BYTE,
            face->glyph->bitmap.buffer
        );
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

        Character character =
        {
            texture,
            glm::ivec2(face->glyph->bitmap.width, face->glyph->bitmap.rows),
            glm::ivec2(face->glyph->bitmap_left, face->glyph->bitmap_top),
            (unsigned int)(face->glyph->advance.x) >> 6
        };
        characters[c] = character;
    }
    glBindTexture(GL_TEXTURE_2D, 0);
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

	return 0;
}

void TextRenderer::destroy()
{
    for (const auto& pair : characters)
    {
        glDeleteTextures(1, &pair.second.textureID);
    }
    textVAO->clean();
    delete textVAO;
    textVBO->clean();
    delete textVBO;
}

void TextRenderer::beforeTextRender()
{
    glEnable(GL_BLEND);
    glDisable(GL_DEPTH_TEST);
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
    textShader->setUniformFloat3("textColor", color.r, color.g, color.b);
    scale *= 2.0f / fontSize;

    // get width and height
    float textW = 0.0f;
    int textMinY = INT_MAX;
    int textMaxY = INT_MIN;
    size_t textLength = text.size();
    for (size_t i = 0; i < textLength; i++)
    {
        const Character& ch = characters[text[i]];

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
    for (char c : text)
    {
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
        float posY = textY - (ch.size.y - ch.bearing.y) * scale;

        float w = ch.size.x * scale;

        GraphicController::textProgram->setUniformFloat4("transform", textX, textY, w, textH);

        glBindTexture(GL_TEXTURE_2D, ch.textureID);
        glDrawArraysInstanced(GL_TRIANGLE_FAN, 0, 4, 1);

        textX += ch.advance * scale;
    }
}
