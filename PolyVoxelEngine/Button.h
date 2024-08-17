#pragma once
#include <string>
#include <functional>
#include "VAO.h"
#include "VBO.h"
#include "TextRenderer.h"

class Button
{
	bool initialized = false;
	float x, y, width, height;
	std::string label;
	std::function<void()> onClick;

	VAO vao;
	VBO vbo;
public:
	Button();
	Button(float x, float y, float width, float height, const std::string& label, std::function<void()> onClick, AligmentX aligmentX, AligmentY aligmentY);
	~Button();

	void draw() const;
	void drawText() const;

	void clean() const;

	bool click(float x, float y) const;
};

