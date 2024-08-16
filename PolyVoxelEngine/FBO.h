#pragma once
#include <glad/glad.h>

class FBO
{
	int width = 0, height = 0;

	GLuint rectVAO = 0;
	GLuint rectVBO = 0;

	GLuint fboID = 0;
	GLuint textureID = 0;
	GLuint rboID = 0;
public:
	FBO(int width, int height);

	void draw() const;

	void beforeRender() const;

	void clean() const;
};

