#include "GraphicController.h"
#include "settings.h"
#include <stb/stb_image.h>

int GraphicController::width = 0;
int GraphicController::height = 0;
float GraphicController::aspectRatio = 0.0f;

FBO* GraphicController::fbo = nullptr;
Shader* GraphicController::framebufferProgram = nullptr;
GLFWwindow* GraphicController::window = nullptr;
Shader* GraphicController::chunkProgram = nullptr;
Shader* GraphicController::deferredChunkProgram = nullptr;
Shader* GraphicController::textProgram = nullptr;
Shader* GraphicController::voxelGhostProgram = nullptr;
Shader* GraphicController::hotbarProgram = nullptr;
Shader* GraphicController::buttonProgram = nullptr;
Shader* GraphicController::rectangleProgram = nullptr;
int GraphicController::menuMaxFps = 0;
int GraphicController::gameMaxFps = 0;
GameSettings GraphicController::gameSettings;
bool GraphicController::zPrePass = false;

void frameBufferSizeCallback(GLFWwindow* window, int width, int height)
{
	GraphicController::resizeWindow(width, height);
}

void GraphicController::centerWindow()
{
	GLFWmonitor* primaryMonitor = glfwGetPrimaryMonitor();
	if (primaryMonitor == NULL)
	{
		std::cerr << "Failed to get primary monitor" << std::endl;
		return;
	}
	const GLFWvidmode* mode = glfwGetVideoMode(primaryMonitor);
	if (mode == NULL)
	{
		std::cerr << "Failed to get video mode" << std::endl;
		return;
	}

	int posX = (mode->width - width) / 2;
	int posY = (mode->height - height) / 2;

	glfwSetWindowPos(window, posX, posY);
}

int GraphicController::init(const GraphicSettings& graphicSettings, const GameSettings& gameSettings)
{
	GraphicController::gameSettings = gameSettings;

	width = graphicSettings.width;
	height = graphicSettings.height;
	aspectRatio = (float)width / (float)height;

	menuMaxFps = graphicSettings.menuMaxFps;
	gameMaxFps = graphicSettings.gameMaxFps;
#pragma region opengl init
	if (window != nullptr)
	{
		std::cerr << "GraphicController is already initialized" << std::endl;
		return 0;
	}

	// GLFW init
	glfwInit();
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, (graphicSettings.openglVersion / 100) % 10);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, (graphicSettings.openglVersion / 10) % 10);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

	// window
	window = glfwCreateWindow(width, height, "PolyVoxelEngine", graphicSettings.fullcreen ? glfwGetPrimaryMonitor() : NULL, NULL);
	if (window == NULL) 
	{
		std::cerr << "Failed to create window" << std::endl;
		glfwTerminate();
		return -1;
	}

	glfwMakeContextCurrent(window);
	glfwSwapInterval(graphicSettings.vsync);
	centerWindow();

	glfwSetFramebufferSizeCallback(window, frameBufferSizeCallback);

	// glad configures OpenGL
	gladLoadGL();
	glViewport(0, 0, width, height);

#pragma endregion
	glEnable(GL_DEPTH_TEST);
	glDisable(GL_CULL_FACE);
	glDisable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	if (gameSettings.rawMouseInput && glfwRawMouseMotionSupported()) 
	{
		glfwSetInputMode(window, GLFW_RAW_MOUSE_MOTION, GLFW_TRUE);
	}

	{
		int width, height, channels;
		unsigned char* pixels = stbi_load(graphicSettings.iconPath.c_str(), &width, &height, &channels, 4);
		if (pixels == NULL)
		{
			std::cerr << "Failed to load icon image" << std::endl;
		}
		else
		{
			GLFWimage icon;
			icon.width = width;
			icon.height = height;
			icon.pixels = pixels;
			glfwSetWindowIcon(window, 1, &icon);
			stbi_image_free(pixels);
		}
	}

	// programs
#if ENABLE_SMOOTH_LIGHTING
	chunkProgram = new Shader("chunk", "#define SMOOTH_LIGHTING");
#else
	chunkProgram = new Shader("chunk");
#endif
	chunkProgram->bind();
	chunkProgram->setUniformFloat3("fogColor", 0.509f, 0.623f, 1.0f);
	chunkProgram->setUniformFloat("fogDensity", Settings::fogDensity);
	chunkProgram->setUniformFloat("fogGradient", Settings::fogGradient);

#if ENABLE_SMOOTH_LIGHTING
	deferredChunkProgram = new Shader("chunk", "#define Z_PRE_PASS;#define SMOOTH_LIGHTING");
#else
	deferredChunkProgram = new Shader("chunk", "#define Z_PRE_PASS");
#endif

	framebufferProgram = new Shader("frameBuffer");
	framebufferProgram->bind();
	framebufferProgram->setUniformInt("screenTexture", 0);

	textProgram = new Shader("text");
	textProgram->bind();
	textProgram->setUniformFloat("aspectRatio", aspectRatio);

	voxelGhostProgram = new Shader("voxelGhost");

	hotbarProgram = new Shader("hotbar");

	buttonProgram = new Shader("button");
	buttonProgram->bind();
	buttonProgram->setUniformFloat("aspectRatio", aspectRatio);

	rectangleProgram = new Shader("rectangle");
	rectangleProgram->bind();
	rectangleProgram->setUniformFloat("aspectRatio", aspectRatio);
	
	// fbo
	fbo = new FBO(width, height);
	return 0;
}

void GraphicController::setWindowTitle(const char* title)
{
	glfwSetWindowTitle(window, title);
}

void GraphicController::setCursorMode(int mode)
{
	glfwSetInputMode(window, GLFW_CURSOR, mode);
}

void GraphicController::resizeWindow(int width, int height)
{
	GraphicController::width = width;
	GraphicController::height = height;

	// update aspectRatio
	aspectRatio = (float)width / (float)height;

	textProgram->bind();
	textProgram->setUniformFloat("aspectRatio", aspectRatio);

	buttonProgram->bind();
	buttonProgram->setUniformFloat("aspectRatio", aspectRatio);

	rectangleProgram->bind();
	rectangleProgram->setUniformFloat("aspectRatio", aspectRatio);

	// glfw
	glViewport(0, 0, width, height);
	fbo->resize(width, height);
}

bool GraphicController::shouldWindowClose()
{
	return glfwWindowShouldClose(window);
}

void GraphicController::closeWindow()
{
	glfwSetWindowShouldClose(window, 1);
}

bool GraphicController::isKeyPressed(int key)
{
	return glfwGetKey(window, key) == GLFW_PRESS;
}

bool GraphicController::isMouseButtonPressed(int button)
{
	return glfwGetMouseButton(window, button) == GLFW_PRESS;
}

void GraphicController::beforeRender()
{
	fbo->beforeRender();
	glClearColor(0.509f, 0.623f, 1.0f, 1.0f);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
}

void GraphicController::afterRender()
{
	framebufferProgram->bind();
	framebufferProgram->setUniformFloat("aspectRatio", aspectRatio);
	fbo->draw();
	glfwSwapBuffers(window);
}

void GraphicController::clean()
{
	fbo->clean(); delete fbo;
	framebufferProgram->clean(); delete framebufferProgram;
	chunkProgram->clean(); delete chunkProgram;
	deferredChunkProgram->clean(); delete deferredChunkProgram;
	textProgram->clean(); delete textProgram;
	voxelGhostProgram->clean(); delete voxelGhostProgram;
	hotbarProgram->clean(); delete hotbarProgram;
	buttonProgram->clean(); delete buttonProgram;

	glfwDestroyWindow(window); window = nullptr;
	glfwTerminate();
}
