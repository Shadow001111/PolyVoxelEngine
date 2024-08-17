#pragma once
#include "FBO.h"
#include "Shader.h"

#include<GLFW/glfw3.h>

struct GraphicData
{
	int width, height;
	float aspectRatio;
};

class GraphicController
{
	static FBO* fbo;
	static Shader* framebufferProgram;

	static void centerWindow();
public:
	static int width, height;
	static float aspectRatio;

	static GLFWwindow* window;
	static Shader* chunkProgram;
	static Shader* textProgram;
	static Shader* voxelGhostProgram;
	static Shader* hotbarProgram;
	static Shader* buttonProgram;

	static int init(int width, int height, bool vsync, int openglVersion);

	static void setWindowTitle(const char* title);
	static void setCursorMode(int mode);

	static bool shouldWindowClose();
	static void closeWindow();

	static bool isKeyPressed(int key);
	static bool isMouseButtonPressed(int button);

	static void beforeRender();
	static void afterRender();

	static void clean();
};

