#pragma once

struct DrawArraysIndirectCommand
{
	unsigned int count; // verticesCount
	unsigned int instancesCount;
	unsigned int first;	// vertex index
	unsigned int baseInstance;
};

class IndirectBuffer
{
	unsigned int ID;
public:
	IndirectBuffer(size_t count);
	void setData(const DrawArraysIndirectCommand* data, size_t count) const;

	void bind() const;
	void unbind() const;
	void clean() const;
};


