#include "VAO.h"
#include <glad/glad.h>

VAO::VAO() : ID(0)
{
	glGenVertexArrays(1, &ID);
	bind();
}

void VAO::linkFloat(unsigned int num_components, size_t sizeOfVertex)
{
	unsigned int layout = autolinkLayout++;
	glVertexAttribPointer(layout, num_components, GL_FLOAT, GL_FALSE, sizeOfVertex, (void*)autolinkOffset);
	glEnableVertexAttribArray(layout);
	autolinkOffset += num_components * sizeof(GLfloat);
}

void VAO::linkDouble(unsigned int num_components, size_t sizeOfVertex)
{
	unsigned int layout = autolinkLayout++;
	glVertexAttribLPointer(layout, num_components, GL_DOUBLE, sizeOfVertex, (void*)autolinkOffset);
	glEnableVertexAttribArray(layout);
	autolinkOffset += num_components * sizeof(GLdouble);
}

void VAO::linkInt(unsigned int num_components, size_t sizeOfVertex)
{
	unsigned int layout = autolinkLayout++;
	glVertexAttribIPointer(layout, num_components, GL_INT, sizeOfVertex, (void*)autolinkOffset);
	glEnableVertexAttribArray(layout);
	autolinkOffset += num_components * sizeof(GLint);
}

void VAO::bind() const
{
	glBindVertexArray(ID);
}

void VAO::unbind()
{
	glBindVertexArray(0);
}

void VAO::clean() const
{
	glDeleteVertexArrays(1, &ID);
}

unsigned int VAO::getLayout() const
{
	return autolinkLayout;
}
