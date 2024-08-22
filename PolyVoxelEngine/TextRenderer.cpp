#include "TextRenderer.h"
#include <iostream>

std::unordered_map<char, Character> TextRenderer::characters;
VAO* TextRenderer::textVAO = nullptr;
VBO* TextRenderer::textVBO = nullptr;

size_t TextRenderer::fontSize = 52;

Shader* TextRenderer::textShader = nullptr;

struct TextVertex
{
    glm::vec2 pos;
    glm::vec2 uv;

    TextVertex(float x, float y, float u, float v) : pos(x, y), uv(u, v)
    {}
};

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
    for (unsigned char c = 0; c < 128; c++)
    {
        if (FT_Load_Char(face, c, FT_LOAD_RENDER))
        {
            std::cerr << "ERROR::FREETYTPE: Failed to load Glyph" << std::endl;
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
            static_cast<unsigned int>(face->glyph->advance.x)
        };
        characters[c] = character;
    }
    glBindTexture(GL_TEXTURE_2D, 0);
    FT_Done_Face(face);
    FT_Done_FreeType(ft);

    //
    textVBO = new VBO(nullptr, sizeof(TextVertex) * 6, GL_DYNAMIC_DRAW);

    textVAO = new VAO();
    textVAO->linkFloat(2, sizeof(TextVertex));
    textVAO->linkFloat(2, sizeof(TextVertex));

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

void TextRenderer::renderText(const std::string& text, float x_, float y_, float scale, glm::vec3 color, AligmentX aligmentX, AligmentY aligmentY)
{
    textShader->setUniformFloat3("textColor", color.r, color.g, color.b);
    scale *= 2.0f / fontSize;

    // get width and height
    float textW = 0.0f;
    float textH = 0.0f;
    for (char c : text)
    {
        const Character& ch = characters[c];
        textW += ch.size.x;
        textH = fmaxf(textH, ch.size.y);
    }
    textW *= scale;
    textH *= scale;

    if (aligmentX == AligmentX::Right)
    {
        x_ -= textW;
    }
    else if (aligmentX == AligmentX::Center)
    {
        x_ -= textW * 0.5f;
    }
    if (aligmentY == AligmentY::Bottom)
    {
        y_ += textH;
    }
    else if (aligmentY == AligmentY::Center)
    {
        y_ += textH * 0.5f;
    }

    // render
    {
        float x = x_;
        for (char c : text)
        {
            const Character& ch = characters[c];

            float posX = x + ch.bearing.x * scale;
            float posY = y_ - (ch.size.y - ch.bearing.y) * scale;

            float w = ch.size.x * scale;
            float h = ch.size.y * scale;

            TextVertex vertices[4] =
            {
                {posX, posY - textH, 0.0f, 1.0f},
                {posX + w, posY - textH, 1.0f, 1.0f},
                {posX + w, posY, 1.0f, 0.0f},
                {posX, posY, 0.0f, 0.0f}
            };

            glBindTexture(GL_TEXTURE_2D, ch.textureID);
            textVBO->setData((const char*)vertices, sizeof(vertices));

            glDrawArrays(GL_TRIANGLE_FAN, 0, 4);

            x += (ch.advance >> 6) * scale;
        }
    }
}
