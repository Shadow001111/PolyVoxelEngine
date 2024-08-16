#include "IBO.h"
#include <glad/glad.h>

IndirectBuffer::IndirectBuffer(size_t count) : ID(0)
{
	glGenBuffers(1, &ID);
	glBindBuffer(GL_DRAW_INDIRECT_BUFFER, ID);
	glBufferData(GL_DRAW_INDIRECT_BUFFER, count * sizeof(DrawArraysIndirectCommand), nullptr, GL_DYNAMIC_DRAW);
}

void IndirectBuffer::setData(const DrawArraysIndirectCommand* data, size_t count)
{
	glBindBuffer(GL_DRAW_INDIRECT_BUFFER, ID);
	glBufferSubData(GL_DRAW_INDIRECT_BUFFER, 0, count * sizeof(DrawArraysIndirectCommand), data);
}

void IndirectBuffer::bind() const
{
	glBindBuffer(GL_DRAW_INDIRECT_BUFFER, ID);
}

void IndirectBuffer::unbind() const
{
	glBindBuffer(GL_DRAW_INDIRECT_BUFFER, 0);
}

void IndirectBuffer::clean() const
{
	glDeleteBuffers(1, &ID);
}
