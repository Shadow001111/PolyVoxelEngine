#include "Button.h"
#include <glm/vec3.hpp>

struct ButtonVertex
{
	float x, y;
	ButtonVertex(float x, float y) : x(x), y(y)
	{}
};

Button::Button() :
	x(0.0f), y(0.0f), width(0.0f), height(0.0f), label("")
{}

Button::Button(float x_, float y_, float width, float height, const std::string& label, std::function<void()> onClick, AligmentX aligmentX, AligmentY aligmentY) :
	width(width), height(height), label(label), onClick(onClick)
{
	if (aligmentX == AligmentX::Left)
	{
		x = x_;
	}
	else if (aligmentX == AligmentX::Right)
	{
		x = x_ - width;
	}
	else
	{
		x = x_ - width * 0.5f;
	}
	if (aligmentY == AligmentY::Bottom)
	{
		y = y_;
	}
	else if (aligmentY == AligmentY::Top)
	{
		y = y_ - height;
	}
	else
	{
		y = y_ - height * 0.5f;
	}

	const ButtonVertex buttonVertexes[4] =
	{
		{x, y},
		{x + width, y},
		{x + width, y + height},
		{x, y + height}
	};

	vao = new VAO();
	vbo = new VBO((const char*)buttonVertexes, sizeof(buttonVertexes), GL_STATIC_DRAW);

	vao->linkFloat(2, sizeof(ButtonVertex));
	VAO::unbind();
}

Button::Button(Button&& other) noexcept
{
	x = other.x;
	y = other.y;
	width = other.width;
	height = other.height;
	label = std::move(other.label);
	onClick = other.onClick;
	vao = other.vao;
	vbo = other.vbo;

	other.vao = nullptr;
	other.vbo = nullptr;
}

Button::~Button()
{
	if (vao)
	{
		vao->clean(); delete vao;
		vbo->clean(); delete vbo;
	}
}

void Button::draw() const
{
	vao->bind();
	glDrawArrays(GL_TRIANGLE_FAN, 0, 4);
}

void Button::drawText() const
{
	float centerX = x + width * 0.5f;
	float centerY = y + height * 0.5f;
	TextRenderer::renderText(label, centerX, centerY, 0.05f, glm::vec3(1.0f, 0.0f, 0.0f), AligmentX::Center, AligmentY::Center);
}

bool Button::click(float x, float y) const
{
	if (x > this->x &&
		x < this->x + width &&
		y > this->y &&
		y < this->y + height)
	{
		onClick();
		return true;
	}
	return false;
}
