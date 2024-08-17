#pragma once
#include <string>
#include <functional>
#include "VAO.h"
#include "VBO.h"

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

class Button
{
	float x, y, width, height;
	std::string label;
	std::function<void()> onClick;

	VAO vao;
	VBO vbo;
public:
	Button(float x, float y, float width, float height, const std::string& label, std::function<void()> onClick, AligmentX aligmentX, AligmentY aligmentY);

	void draw() const;
	void drawText() const;
};

