#include "SSBO.h"
#include <glad/glad.h>

SSBO::SSBO(size_t size)
{
	glGenBuffers(1, &ID);
	glBindBuffer(GL_SHADER_STORAGE_BUFFER, ID);
	glBufferData(GL_SHADER_STORAGE_BUFFER, size, nullptr, GL_DYNAMIC_DRAW);
}

void SSBO::setData(const char* data, size_t size)
{
	glBindBuffer(GL_SHADER_STORAGE_BUFFER, ID);
	glBufferSubData(GL_SHADER_STORAGE_BUFFER, 0, size, data);
}

void SSBO::bindBase(size_t slot) const
{
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, slot, ID);
}

void SSBO::clean() const
{
	glDeleteBuffers(1, &ID);
}
