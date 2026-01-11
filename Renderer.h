#pragma once

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#define GLFW_EXPOSE_NATIVE_WIN32
#include <GLFW/glfw3native.h>
#include <vector>


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
	VkDevice mLogicalDevice;
	VkQueue mGraphicsQueue;
	VkQueue mPresentQueue;
	VkSwapchainKHR mSwapchain;
	VkExtent2D mSwapchainExtent;

	std::vector<VkImage> mImages;
	std::vector<VkImageView> mImageViews;
	
	VkRenderPass mRenderPass;
	std::vector<VkFramebuffer> mFramebuffers;

	VkPipeline mGraphicsPipeline;
	VkPipelineLayout mPipelineLayout;
	
	VkCommandPool mCommandPool;
	std::vector<VkCommandBuffer> mCommandBuffers;
	uint32_t mCurrentFrame;

	std::vector<VkFence> mFences;
	std::vector<VkFence> mImagesInFlight;
	std::vector<VkSemaphore> imageAvailableSemaphores;
	std::vector<VkSemaphore> renderFinishedSemaphores;


	void createWindow();

	void createInstance();
	void createSurface();
	void pickPhysicalDevice(uint32_t& outGraphicsFamilyIndex, uint32_t& outPresentFamilyIndex);
	bool findQueueFamilies(VkPhysicalDevice device, uint32_t& outGraphicsFamilyIndex, uint32_t& outPresentFamilyIndex) const;
	void createLogicalDevice(const uint32_t graphicsFamilyIndex, const uint32_t presentFamilyIndex);
	void createSwapchain(const uint32_t graphicsFamilyIndex, const uint32_t presentFamilyIndex);
	VkSurfaceFormatKHR pickBestFormat() const;
	VkPresentModeKHR pickBestPresentMode() const;
	void createImageViews(VkSurfaceFormatKHR format);
	void createRenderPass();
	void createFramebuffers();
	void createGraphicsPipeline();
	VkShaderModule createShaderModule(const std::vector<char>& code) const;
	void createCommandPool(const uint32_t graphicsFamilyIndex);
	void createCommandBuffers();

	// createPipeline
	// 여기까지 OpenGL은 CreateProgram 한 줄로 끝냄

	void createSyncObjects();
	void drawFrame();
	void recordCommandBuffer(VkCommandBuffer currentBuffer, uint32_t imageIndex);


	void mainLoop();
	void cleanup();
};


