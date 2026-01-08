#pragma once

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#define GLFW_EXPOSE_NATIVE_WIN32
#include <GLFW/glfw3native.h>
#include <optional>


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
	VkSurfaceKHR mSurface;
	VkPhysicalDevice mPhysicalDevice;

	std::optional<uint32_t> mGraphicsFamilyIndex;
	std::optional<uint32_t> mPresentFamilyIndex;

	void createWindow();
	void createInstance();
	void createSurface();
	void pickPhysicalDevice();
	void FindQueueFamilies(VkPhysicalDevice device, std::optional<uint32_t>& outGraphicsFamilyIndex, std::optional<uint32_t>& outPresentFamilyIndex) const;


	void mainLoop();
	void cleanup();
};


