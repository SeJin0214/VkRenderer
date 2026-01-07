#pragma once

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#define GLFW_EXPOSE_NATIVE_WIN32
#include <GLFW/glfw3native.h>

enum 
{
	WIDTH = 800,
	HEIGHT = 600
};

class Renderer
{

public:
	Renderer();
	void Run();

private:

	VkInstance mInstance;
	GLFWwindow* mWindow;
	VkDebugUtilsMessengerEXT mDebugMessenger;

	void createWindow();
	void createInstance();
	void mainLoop();
	void cleanup();
};


