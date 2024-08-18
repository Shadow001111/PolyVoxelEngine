#include "FBO.h"
#include <iostream>
#include <glm/glm.hpp>
#include "settings.h"

struct FBOVertex
{
	glm::vec2 pos;
	glm::vec2 uv;
};

FBOVertex FBOvertices[] = {
	{{1.0f, -1.0f}, {1.0f, 0.0f}},
	{{-1.0f, -1.0f}, {0.0f, 0.0f}},
	{{-1.0f, 1.0f}, {0.0f, 1.0f}},
	{{1.0f, 1.0f}, {1.0f, 1.0f}},
};

FBO::FBO(int width, int height) : width(width), height(height)
{
	// vao
	glGenVertexArrays(1, &rectVAO);
	glGenBuffers(1, &rectVBO);
	glBindVertexArray(rectVAO);
	glBindBuffer(GL_ARRAY_BUFFER, rectVBO);
	glBufferData(GL_ARRAY_BUFFER, sizeof(FBOvertices), &FBOvertices, GL_STATIC_DRAW);

	glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, sizeof(FBOVertex), (void*)0);
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(FBOVertex), (void*)(2 * sizeof(GLfloat)));
	glEnableVertexAttribArray(1);

	// fbo
	glGenFramebuffers(1, &fboID);
	glBindFramebuffer(GL_FRAMEBUFFER, fboID);

	glGenTextures(1, &textureID);
	glBindTexture(GL_TEXTURE_2D, textureID);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, 0);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, textureID, 0);

	glGenRenderbuffers(1, &rboID);
	glBindRenderbuffer(GL_RENDERBUFFER, rboID);
	glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, width, height);
	glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, rboID);
	
	auto fboStatus = glCheckFramebufferStatus(GL_FRAMEBUFFER);
	if (fboStatus != GL_FRAMEBUFFER_COMPLETE)
	{
		std::cerr << "PP Framebuffer error: " << fboStatus << std::endl;
	}
}

void FBO::resize(int width, int height) const
{
	// texture
	glBindTexture(GL_TEXTURE_2D, textureID);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, 0);

	// rbo
	glBindRenderbuffer(GL_RENDERBUFFER, rboID);
	glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, width, height);

}

void FBO::draw() const
{
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	glBindVertexArray(rectVAO);
	glDisable(GL_DEPTH_TEST);
	glDisable(GL_CULL_FACE);
	glBindTexture(GL_TEXTURE_2D, textureID);
	glDrawArrays(GL_TRIANGLE_FAN, 0, 4);
}

void FBO::beforeRender() const
{
	glBindFramebuffer(GL_FRAMEBUFFER, fboID);
}

void FBO::clean() const
{
	glDeleteFramebuffers(1, &fboID);
	glDeleteVertexArrays(1, &rectVAO);
	glDeleteBuffers(1, &rectVBO);
	glDeleteTextures(1, &textureID);
	glDeleteRenderbuffers(1, &rboID);
}
