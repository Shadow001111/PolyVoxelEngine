#pragma once
#include "glad/glad.h"

struct DrawArraysIndirectCommand
{
	GLuint count; // verticesCount
	GLuint instancesCount;
	GLuint first;	// vertex index
	GLuint baseInstance;
};

class IndirectBuffer
{
	unsigned int ID;
public:
	IndirectBuffer(size_t count);
	void setData(const DrawArraysIndirectCommand* data, size_t count);

	void bind() const;
	void unbind() const;
	void clean() const;
};


