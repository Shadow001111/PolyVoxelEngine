#pragma once

class SSBO
{
	unsigned int ID = 0;
public:
	SSBO(size_t size);
	void setData(const char* data, size_t size) const;

	void bindBase(size_t slot) const;
	void clean() const;
};
