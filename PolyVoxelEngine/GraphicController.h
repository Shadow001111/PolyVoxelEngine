#pragma once
#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"
#include "implot.h"

#include "FBO.h"
#include "Shader.h"
#include<GLFW/glfw3.h>

struct GraphicSettings
{
	int openglVersion;
	std::string iconPath;

	int width;
	int height;
	bool vsync;
	bool fullcreen;

	int menuMaxFps;
	int gameMaxFps;
};

struct GameSettings
{
	float sensitivity;
	bool rawMouseInput;
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
	static GameSettings gameSettings;

	static bool zPrePass;

	static GLFWwindow* window;
	static Shader* chunkProgram;
	static Shader* deferredChunkProgram;
	static Shader* textProgram;
	static Shader* voxelGhostProgram;
	static Shader* hotbarProgram;
	static Shader* buttonProgram;
	static Shader* rectangleProgram;

	static int init(const GraphicSettings& graphicSettings, const GameSettings& gameSettings);

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

