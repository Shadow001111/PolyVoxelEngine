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

	static int menuMaxFps;
	static int gameMaxFps;

	static GLFWwindow* window;
	static Shader* chunkProgram;
	static Shader* textProgram;
	static Shader* voxelGhostProgram;
	static Shader* hotbarProgram;
	static Shader* buttonProgram;

	static int init(int openglVersion, int width, int height, bool vsync, bool fullcreen, int menuMaxFps, int gameMaxFps);

	static void setWindowTitle(const char* title);
	static void setCursorMode(int mode);
	static void resizeWindow(int width, int height);

	static bool shouldWindowClose();
	static void closeWindow();

	static bool isKeyPressed(int key);
	static bool isMouseButtonPressed(int button);

	static void beforeRender();
	static void afterRender();

	static void clean();
};

