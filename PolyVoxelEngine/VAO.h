#pragma once

class VAO
{
	unsigned int autolinkLayout = 0;
	unsigned int autolinkOffset = 0;
	unsigned int ID;
public:
	VAO();

	void linkFloat(unsigned int num_components, size_t sizeOfVertex);
	void linkInt(unsigned int num_components, size_t sizeOfVertex);
	void linkDouble(unsigned int num_components, size_t sizeOfVertex);

	void bind() const;
	static void unbind();
	void clean() const;

	unsigned int getLayout() const;
};
