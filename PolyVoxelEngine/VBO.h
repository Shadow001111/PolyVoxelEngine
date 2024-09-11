#pragma once

class VBO
{
	unsigned int ID;
public:
	VBO();
	VBO(const char* data, size_t size, unsigned int usage);
	void setData(const char* data, size_t size) const;

	void bind() const;
	static void unbind();
	void clean() const;
};

