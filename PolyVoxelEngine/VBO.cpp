#include "VBO.h"
#include <glad/glad.h>

VBO::VBO() : ID(0)
{}

VBO::VBO(const char* data, size_t size, unsigned int usage) : ID(0)
{
	glGenBuffers(1, &ID);
	bind();
	glBufferData(GL_ARRAY_BUFFER, size, data, usage);
}

void VBO::setData(const char* data, size_t size) const
{
	glBufferSubData(GL_ARRAY_BUFFER, 0, size, data);
}

void VBO::bind() const
{
	glBindBuffer(GL_ARRAY_BUFFER, ID);
}

void VBO::unbind()
{
	glBindBuffer(GL_ARRAY_BUFFER, 0);
}

void VBO::clean() const
{
	glDeleteBuffers(1, &ID);
}
