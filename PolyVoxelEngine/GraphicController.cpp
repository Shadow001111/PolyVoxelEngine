#include "GraphicController.h"
#include "settings.h"

int GraphicController::width = 0;
int GraphicController::height = 0;
float GraphicController::aspectRatio = 0.0f;

FBO* GraphicController::fbo = nullptr;
Shader* GraphicController::framebufferProgram = nullptr;
GLFWwindow* GraphicController::window = nullptr;
Shader* GraphicController::chunkProgram = nullptr;
Shader* GraphicController::textProgram = nullptr;
Shader* GraphicController::voxelGhostProgram = nullptr;
Shader* GraphicController::hotbarProgram = nullptr;

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

int GraphicController::init(int width, int height, bool vsync, int openglVersion)
{
	GraphicController::width = width;
	GraphicController::height = height;
	GraphicController::aspectRatio = (float)width / (float)height;
#pragma region opengl init
	if (window != nullptr)
	{
		std::cerr << "GraphicController is already initialized" << std::endl;
		return 0;
	}

	// GLFW init
	glfwInit();
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, (openglVersion / 100) % 10);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, (openglVersion / 10) % 10);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

	// window
	window = glfwCreateWindow(width, height, "PolyVoxelEngine", NULL, NULL);
	if (window == NULL) 
	{
		std::cerr << "Failed to create window" << std::endl;
		glfwTerminate();
		return -1;
	}

	glfwMakeContextCurrent(window);
	glfwSwapInterval(vsync);
	centerWindow();

	// glad configures OpenGL
	gladLoadGL();
	glViewport(0, 0, width, height);

#pragma endregion
	glEnable(GL_DEPTH_TEST);
	glDisable(GL_CULL_FACE);
	glDisable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	// programs
#if ENABLE_SMOOTH_LIGHTING
	chunkProgram = new Shader("chunkSmooth");
#else
	chunkProgram = new Shader("chunk");
#endif
	chunkProgram->bind();
	chunkProgram->setUniformFloat3("fogColor", 0.509f, 0.623f, 1.0f);
	chunkProgram->setUniformFloat("fogDensity", Settings::fogDensity);
	chunkProgram->setUniformFloat("fogGradient", Settings::fogGradient);

	framebufferProgram = new Shader("frameBuffer");
	framebufferProgram->bind();
	framebufferProgram->setUniformInt("screenTexture", 0);

	textProgram = new Shader("text");

	voxelGhostProgram = new Shader("voxelGhost");

	hotbarProgram = new Shader("hotbar");
	
	// fbo
	fbo = new FBO(width, height);
	return 0;
}

void GraphicController::setWindowTitle(const char* title)
{
	glfwSetWindowTitle(window, title);
}

bool GraphicController::shouldWindowClose()
{
	return glfwWindowShouldClose(window);
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
	textProgram->clean(); delete textProgram;
	voxelGhostProgram->clean(); delete voxelGhostProgram;
	hotbarProgram->clean(); delete hotbarProgram;

	glfwDestroyWindow(window);
	glfwTerminate();
}
