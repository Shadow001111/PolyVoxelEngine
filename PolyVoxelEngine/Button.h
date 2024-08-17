#pragma once
#include <string>
#include <functional>
#include "VAO.h"
#include "VBO.h"
#include "TextRenderer.h"

class Button
{
	float x, y, width, height;
	std::string label;
	std::function<void()> onClick;

	VAO* vao = nullptr;
	VBO* vbo = nullptr;
public:
	Button();
	Button(float x, float y, float width, float height, const std::string& label, std::function<void()> onClick, AligmentX aligmentX, AligmentY aligmentY);
	Button(Button&& other) noexcept;
	~Button();

	void draw() const;
	void drawText() const;

	bool click(float x, float y) const;
};

