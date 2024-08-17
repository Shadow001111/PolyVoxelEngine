#include "Button.h"
#include <glad/glad.h>
#include "GraphicController.h"

struct ButtonVertex
{
	float x, y;
	ButtonVertex(float x, float y) : x(x), y(y)
	{}
};

const ButtonVertex buttonVertexes[4] =
{
	{0.0f, 0.0f}, {1.0f, 0.0f}, {1.0f, 1.0f}, {0.0f, 1.0f}
};

Button::Button()
{}

Button::Button(float x, float y, float width, float height, const std::string& label, std::function<void()> onClick, AligmentX aligmentX, AligmentY aligmentY) :
	width(width), height(height), label(label), onClick(onClick)
{
	if (aligmentX == AligmentX::Left)
	{
		this->x = x;
	}
	else if (aligmentX == AligmentX::Right)
	{
		this->x = x - width;
	}
	else
	{
		this->x = x - width * 0.5f;
	}
	if (aligmentY == AligmentY::Bottom)
	{
		this->y = y;
	}
	else if (aligmentY == AligmentY::Top)
	{
		this->y = y - height;
	}
	else
	{
		this->y = y - height * 0.5f;
	}

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
	GraphicController::buttonProgram->setUniformFloat2("position", x, y);
	GraphicController::buttonProgram->setUniformFloat2("scale", width, height);
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
